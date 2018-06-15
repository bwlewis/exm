/*
  ___  _  ______ ___ 
 / _ \| |/_/ __ `__ \
/  __/>  </ / / / / /
\___/_/|_/_/ /_/ /_/ 

*/
#define _GNU_SOURCE
#include <syslog.h>
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>
#include <sys/sendfile.h>

#define uthash_malloc(sz) uthash_malloc_(sz)
#define uthash_free(ptr, sz) uthash_free_(ptr)
#include "uthash.h"
#include "exm.h"

#ifndef TMPDIR
#define TMPDIR "/tmp"
#endif

extern void *__libc_malloc (size_t size);
static void exm_init (void) __attribute__ ((constructor));
static void exm_finalize (void) __attribute__ ((destructor));
static void *(*exm_hook) (size_t);
static void *(*exm_default_free) (void *);
static void *(*exm_default_malloc) (size_t);
static void *(*exm_default_valloc) (size_t);
static void *(*exm_default_realloc) (void *, size_t);
static void *(*exm_default_memcpy) (void *dest, const void *src, size_t n);

static pid_t (*exm_default_fork) (void);

static void *uthash_malloc_ (size_t);
static void uthash_free_ (void *);
void freemap (struct map *);

/* READY has three states:
 * -1 at startup, prior to initialization of anything
 *  1 After initialization finished, ready to go.
 *  0 After finalization has been called, don't mmap anymore.
 */
static int READY = -1;

/* NOTES
 *
 * Exm uses well-known methods to overload various memory allocation functions
 * such that allocations above a threshold use memory mapped files. The library
 * maintains a mapping of allocated addresses and corresponding backing files
 * and cleans up files as they are de-allocated.  The idea is to help programs
 * allocate large objects out of core while explicitly avoiding the system swap
 * space. The library provides a basic API that lets programs change the
 * mapping file path and threshold value.
 *
 * Be cautious using any library function in this code, they often end up
 * re-entering one of our functions. The general rule here is to keep things
 * as minimal as possible in here.
 */

/* Exm initialization
 *
 * Initializes synchronizing lock variable and address/file key/value map.
 * This function may be called multiple times, be aware of that and keep
 * this as tiny/simple as possible and thread-safe.
 *
 * Oddly, exm_init is invoked lazily by calloc, which will be triggered
 * either directly by a program calloc call, or on first use of any of the
 * intercepted functions because all of them except for calloc call dlsym,
 * which itself calls calloc.
 *
 */
static void
exm_init ()
{
  if (READY < 0)
    {
      snprintf (exm_data_path, EXM_MAX_PATH_LEN, "%s", TMPDIR);
      omp_init_nest_lock (&lock);
      READY = 1;
      openlog ("exm", LOG_PERROR | LOG_PID, LOG_USER);
      char *endptr, *EXM_THRESHOLD = getenv ("EXM_THRESHOLD");
      if (EXM_THRESHOLD != NULL)
        {
          errno = 0;
          unsigned long _threshold = strtoul (EXM_THRESHOLD, &endptr, 0);
          if (errno == 0)
            exm_alloc_threshold = (size_t) _threshold;
        }
      char *EXM_CHILD_COW = getenv ("EXM_CHILD_COW");
      if (EXM_CHILD_COW != NULL)
        {
          errno = 0;
          long _child_cow = strtol (EXM_CHILD_COW, &endptr, 10);
          if (errno == 0)
            exm_child_cow = (int) _child_cow;
        }
    }
  if (!exm_hook)
    exm_hook = __libc_malloc;
  if (!exm_default_free)
    exm_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
}

/* Exm finalization
 * Remove any left over allocations, but we don't destroy the lock--XXX
 */
static void
exm_finalize ()
{
#if defined(DEBUG) || defined(DEBUG1)
  syslog (LOG_DEBUG, "finalize READY=%d\n", READY);
#endif
  struct map *m, *tmp;
  pid_t pid;
  omp_set_nest_lock (&lock);
  READY = 0;
  HASH_ITER (hh, flexmap, m, tmp)
  {
#if defined(DEBUG) || defined(DEBUG1)
    syslog (LOG_DEBUG, "finalize unmap address %p of size %lu\n", m->addr,
            (unsigned long int) m->length);
#endif
    munmap (m->addr, m->length);
    pid = getpid ();
    if (pid == m->pid)
      {
#if defined(DEBUG) || defined(DEBUG1)
        syslog (LOG_DEBUG, "finalize unlink %p:%s\n", m->addr, m->path);
#endif
        unlink (m->path);
        HASH_DEL (flexmap, m);
        freemap (m);
      }
  }
  omp_unset_nest_lock (&lock);
#if defined(DEBUG) || defined(DEBUG1)
  syslog (LOG_DEBUG, "finalized\n");
#endif
}

int
addr_sort (struct map *a, struct map *b)
{
  int j = 0;
  if (a->addr == NULL || b->addr == NULL) return j;
  if (a->addr < b->addr)
    j = -1;
  else if (a->addr > b->addr)
    j = 1;
  return j;
}

/* freemap is a utility function that deallocates the supplied map structure */
void
freemap (struct map *m)
{
#if defined(DEBUG) || defined(DEBUG1)
  syslog (LOG_DEBUG, "freemap\n");
#endif
  if (m)
    {
      if (m->path)
        (*exm_default_free) (m->path);
      (*exm_default_free) (m);
    }
}

/* Make sure uthash uses the default malloc and free functions. */
void *
uthash_malloc_ (size_t size)
{
  if (!exm_default_malloc)
    exm_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");
  return (*exm_default_malloc) (size);
}

void
uthash_free_ (void *ptr)
{
  if (!exm_default_free)
    exm_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  (*exm_default_free) (ptr);
}

void *
malloc (size_t size)
{
  struct map *m, *y;
  void *x;
  int j;
  int fd;

  if (!exm_default_malloc)
    exm_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");

  if (size < exm_alloc_threshold || READY < 1)
    {
      x = (*exm_default_malloc) (size);
#ifdef DEBUG1
      syslog (LOG_DEBUG, "malloc %p\n", x);
#endif
      if (x)
        return x;               // The usual malloc
      if (READY < 1)
        return NULL;            // malloc failed, not ready
    }

/* If either size >= the threshold value and READY >= 1, or
 * we failed to malloc any size and READY >= 1, then try mmap.
 */
  m = (struct map *) ((*exm_default_malloc) (sizeof (struct map)));
  m->path = (char *) ((*exm_default_malloc) (EXM_MAX_PATH_LEN));
  memset (m->path, 0, EXM_MAX_PATH_LEN);
  omp_set_nest_lock (&lock);
  snprintf (m->path, EXM_MAX_PATH_LEN, "%s/exm%ld_XXXXXX",
            exm_data_path, (long int) getpid ());
  m->length = size;
  fd = mkostemp (m->path, O_RDWR | O_CREAT);
  j = ftruncate (fd, m->length);
  if (j < 0)
    {
      close (fd);
      unlink (m->path);
      freemap (m);
      omp_unset_nest_lock (&lock);
      return NULL;
    }
  m->addr = mmap (NULL, m->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (m->addr == MAP_FAILED)
    {
      syslog (LOG_CRIT, "exm mmap failure\n");
      close (fd);
      unlink (m->path);
      freemap (m);
      omp_unset_nest_lock (&lock);
      return NULL;
    }
  madvise (m->addr, m->length, EXM_DEFAULT_ADVISE);
  m->pid = getpid ();
  x = m->addr;
  close (fd);
#if defined(DEBUG) || defined(DEBUG1)
  syslog (LOG_DEBUG, "malloc address %p, size %lu, file  %s\n",
          m->addr, (unsigned long int) m->length, m->path);
#endif
/* Check to make sure that this address is not already in the hash. If it is,
 * then something is terribly wrong and we must bail.
 */
  HASH_FIND_PTR (flexmap, m->addr, y);
  if (y)
    {
      munmap (m->addr, m->length);
      unlink (m->path);
      freemap (m);
      x = NULL;
    }
  else
    {
//      HASH_ADD_PTR (flexmap, addr, m);
      HASH_ADD_INORDER (hh, flexmap, addr, sizeof (void *), m, addr_sort);
    }
#if defined(DEBUG) || defined(DEBUG1)
  syslog (LOG_DEBUG, "hash count = %u\n", HASH_COUNT (flexmap));
#endif
  omp_unset_nest_lock (&lock);
  return x;
}

void
free (void *ptr)
{
  struct map *m;
  pid_t pid;
  if (!ptr)
    return;
  if (READY > 0)
    {
#ifdef DEBUG1
      syslog (LOG_DEBUG, "free %p\n", ptr);
#endif
      omp_set_nest_lock (&lock);
      HASH_FIND_PTR (flexmap, &ptr, m);
/* Make sure a child process does not delete a parent mapping */
      pid = getpid ();
      if (m)
        {
#if defined(DEBUG) || defined(DEBUG1)
          syslog (LOG_DEBUG,
                  "free unmap address %p of size %lu %ld\n", ptr,
                  (unsigned long int) m->length, (long int) m->pid);
#endif
          munmap (ptr, m->length);
          if (pid == m->pid)
            {
#if defined(DEBUG) || defined(DEBUG1)
              syslog (LOG_DEBUG, "free unlink %p:%s\n", ptr, m->path);
#endif
              unlink (m->path);
              HASH_DEL (flexmap, m);
              freemap (m);
            }
          omp_unset_nest_lock (&lock);
          return;
        }
      omp_unset_nest_lock (&lock);
    }
  if (!exm_default_free)
    exm_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  (*exm_default_free) (ptr);
}

/* valloc returns memory aligned to a page boundary.  Memory mapped flies are
 * aligned to page boundaries, so we simply return our modified malloc when
 * over the threshold. Otherwise, fall back to default valloc.
 */
void *
valloc (size_t size)
{
  if (READY > 0 && size > exm_alloc_threshold)
    {
#if defined(DEBUG) || defined(DEBUG1)
      syslog (LOG_DEBUG, "valloc...handing off to exm malloc\n");
#endif
      return malloc (size);
    }
  if (!exm_default_valloc)
    exm_default_valloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "valloc");
  return exm_default_valloc (size);
}

/* Realloc is complicated in the case of fork. We have to protect parents from
 * wayward children and also maintain expected realloc behavior. See comments
 * below...
 */
void *
realloc (void *ptr, size_t size)
{
  struct map *m, *y;
  int j, fd, child;
  void *x;
  pid_t pid;
  size_t copylen;
#ifdef DEBUG1
  syslog (LOG_DEBUG, "realloc\n");
#endif

/* Handle two special realloc cases: */
  if (ptr == NULL)
    return malloc (size);
  if (size == 0)
    {
      free (ptr);
      return NULL;
    }

  x = NULL;
  if (!exm_default_realloc)
    exm_default_realloc =
      (void *(*)(void *, size_t)) dlsym (RTLD_NEXT, "realloc");
  if (READY > 0)
    {
      omp_set_nest_lock (&lock);
      HASH_FIND_PTR (flexmap, &ptr, m);
      if (m)
        {
/* Remove the current file mapping, truncate the file, and return a new
 * file mapping to the truncated file. But don't allow a child process
 * to screw with the parent's mapping.
 */
          munmap (ptr, m->length);
          pid = getpid ();
          child = 0;
          if (pid == m->pid)
            {
              HASH_DEL (flexmap, m);
              m->length = size;
              fd = open (m->path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            }
          else
            {
/* Uh oh. We're in a child process. We need to copy this mapping and create a
 * new map entry unique to the child.  Also  need to copy old data up to min
 * (size, m->length). This can only happen if exm_child_cow = 0, see fork
 * below.
 */
              y = m;
              child = 1;
              m =
                (struct map *) ((*exm_default_malloc) (sizeof (struct map)));
              m->path = (char *) ((*exm_default_malloc) (EXM_MAX_PATH_LEN));
              memset (m->path, 0, EXM_MAX_PATH_LEN);
              snprintf (m->path, EXM_MAX_PATH_LEN, "%s/exm%ld_XXXXXX",
                        exm_data_path, (long int) getpid ());
              m->length = size;
              copylen = m->length;
              if (y->length < copylen)
                copylen = y->length;
              fd = mkostemp (m->path, O_RDWR | O_CREAT);
            }
          if (fd < 0)
            {
              omp_unset_nest_lock (&lock);
              goto bail;
            }
          j = ftruncate (fd, m->length);
          if (j < 0)
            {
              omp_unset_nest_lock (&lock);
              goto bail;
            }
          m->addr =
            mmap (NULL, m->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
/* Here is a rather unfortunate child copy... XXX use some kind of cow map? */
          if (child)
            exm_default_memcpy (m->addr, y->addr, copylen);
          m->pid = getpid ();
/* Check for existence of the address in the hash. It must not already exist,
 * (after all we just removed it and we hold the lock)--if it does something
 * is terribly wrong and we bail.
 */
          HASH_FIND_PTR (flexmap, m->addr, y);
          if (y)
            {
              omp_unset_nest_lock (&lock);
              munmap (m->addr, m->length);
              goto bail;
            }
//          HASH_ADD_PTR (flexmap, addr, m);
          HASH_ADD_INORDER (hh, flexmap, addr, sizeof (void *), m, addr_sort);
          x = m->addr;
          close (fd);
#if defined(DEBUG) || defined(DEBUG1)
          syslog (LOG_DEBUG, "realloc address %p size %lu\n", ptr,
                  (unsigned long int) m->length);
#endif
          omp_unset_nest_lock (&lock);
          return x;
        }
      omp_unset_nest_lock (&lock);
    }
  x = (*exm_default_realloc) (ptr, size);
  return x;

bail:
  close (fd);
  unlink (m->path);
  freemap (m);
  return NULL;
}

#ifdef OSX
// XXX reallocf is a Mac OSX/BSD-specific function.
void *
reallocf (void *ptr, size_t size)
{
// XXX WRITE ME!
  return reallocf (ptr, size);
}
#else
// XXX memalign, posix_memalign are not in OSX/BSD, but are in Linux
// and POSIX. Define them here.
// XXX WRITE ME!
#endif


/* calloc is a special case.  dlsym ultimately calls calloc, thus a direct
 * interposition here results in an infinite loop...We created a fake calloc
 * that relies on malloc, and we avoid use of dlsym by using the special glibc
 * __libc_malloc. This works, but requires glibc!  We should not need to do
 * this on BSD/OS X.
 */
void *
calloc (size_t count, size_t size)
{
  void *x;
  size_t n = count * size;
  if (READY > 0 && n > exm_alloc_threshold)
    {
#if defined(DEBUG) || defined(DEBUG1)
      syslog (LOG_DEBUG, "calloc...handing off to exm malloc\n");
#endif
      return malloc (n);
    }
  if (!exm_hook)
    exm_init ();
  x = exm_hook (n);             //, NULL);
  memset (x, 0, n);
  return x;
}





/* exm-aware memcpy.
 *
 * It turns out, at least on Linux, that memcpy on memory-mapped files is much
 * slower than simply copying the data with read and write--and much, much
 * slower than sendfile (which performs the copy with kernel routines only).
 *
 * This is a placeholder for a future memcpy that detects when copies are
 * made between exm-allocated regions. At the moment, this only handles
 * the easy case of full region copies.
 *
 * XXX Once this is finished, then WRITE ME:
 * memccpy, memmove, strncpy, wmemcpy
 *
 * XXX See below for a proposed fork COW API. Follow the same rules for
 * XXX memcpy? Or perhaps similar rules?
 */
void *
memcpy (void *dest, const void *src, size_t n)
{
  struct map *SRC, *DEST;
  int src_fd, dest_fd;
  if (!exm_default_memcpy)
    exm_default_memcpy =
      (void *(*)(void *, const void *, size_t)) dlsym (RTLD_NEXT, "memcpy");
  omp_set_nest_lock (&lock);
/* XXX here we need to see if the src and dest lie within exm allocations.
 * right now, this only catches the niche/easy case of copying the whole
 * region.
 */
  HASH_FIND_PTR (flexmap, &src, SRC);
  HASH_FIND_PTR (flexmap, &dest, DEST);
  if (!SRC || !DEST)
    {
      omp_unset_nest_lock (&lock);
      return (*exm_default_memcpy) (dest, src, n);
    }
  if (SRC->length != n || DEST->length < n)
    {
      omp_unset_nest_lock (&lock);
      return (*exm_default_memcpy) (dest, src, n);
    }
#if defined(DEBUG) || defined(DEBUG1)
  syslog (LOG_DEBUG, "memcopy address %p src_addr %p of size %lu\n",
          SRC->addr, src, (unsigned long int) SRC->length);
#endif
/*  Consider replacing with a copy on write approach?  XXX */
  src_fd = open (SRC->path, O_RDONLY);
  dest_fd = open (DEST->path, O_RDWR);
  omp_unset_nest_lock (&lock);
  sendfile (dest_fd, src_fd, 0, n);     // XXX how to handle error?
  close (dest_fd);
  close (src_fd);
  return dest;
}



/* Optionally convert child process mappings to copy on write MAP_PRIVATE */
// XXX XXX fork/cow API not stable yet
// Propose:
// exm_child_cow <= 0   MAP_SHARED
// exm_child_cow  = 1   MAP_PRIVATE (in-core COW populated as written to, default)
// exm_child_cow  = 2   map copy of original (MAP_SHARED, but copied)
// exm_child_cow  = 3   reserved for proposed future out of core COW
// exm_child_cow  > 3   reserved
// XXX
pid_t
fork (void)
{
  pid_t p;
  if (!exm_default_fork)
    exm_default_fork = (pid_t (*)(void)) dlsym (RTLD_NEXT, "fork");
  p = exm_default_fork ();
  if (exm_child_cow <= 0 || p > 0)
    return p;

  /* following run only by forked child ... */
  struct map *m, *tmp, *remap, *x;
  int fd;
  pid_t q = getpid ();
  omp_set_nest_lock (&lock);
  HASH_ITER (hh, flexmap, m, tmp)
  {
    if (q != m->pid)
      {
        remap =
          (struct map *) ((*exm_default_malloc) (sizeof (struct map)));
        remap->path = (char *) ((*exm_default_malloc) (EXM_MAX_PATH_LEN));
        memset (remap->path, 0, EXM_MAX_PATH_LEN);
        fd = open (m->path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd >= 0)
          {
            remap->addr =
              mmap (m->addr, m->length, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_PRIVATE, fd, 0);
            if (remap->addr == MAP_FAILED)
              {
                syslog (LOG_CRIT, "fork (child) remap failure %p\n", m->addr);
              }
            else
              {
                snprintf (remap->path, EXM_MAX_PATH_LEN, "%s", m->path);
                remap->length = m->length;
                remap->pid = q;
                HASH_REPLACE_INORDER (hh, flexmap, addr, sizeof (void *),
                                      remap, x, addr_sort);
                syslog (LOG_DEBUG,
                        "child remapping address %p as copy on write",
                        x->addr);
                if (x != NULL) freemap (x);
              }
              close (fd);
          }
        else
          syslog (LOG_DEBUG, "warning: child unable to remap address %p",
                  m->addr);
        freemap (remap);
      }
  }
  omp_unset_nest_lock (&lock);
  return p;
}
