/*
  ___  _  ______ ___ 
 / _ \| |/_/ __ `__ \
/  __/>  </ / / / / /
\___/_/|_/_/ /_/ /_/ 
                     
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <omp.h>

#include "uthash.h"
#include "exm.h"

/* exm_path is initialized in exm.c:exm_init() */
char exm_data_path[EXM_MAX_PATH_LEN];
size_t exm_alloc_threshold = 2000000000;
int exm_child_cow = 1;

/* The next functions allow applications to inspect and change default
 * settings. The application must dynamically locate them with dlsym after
 * the library is loaded. They represent the exm API, such as it is.
 *
 * API functions defined below include:
 * double exm_version()
 * size_t exm_threshold(size_t j)
 * char * exm_path(char *path)
 * char * exm_lookup(void *addr)
 * int exm_madvise(void *addr, int advice)
 * int exm_child_cow(int j)
 */

/* Return the exm library version
 * OUTPUT (return value): double version major.minor
 */
double
exm_version (char *v)
{
  return EXM_VERSION;
}

/* Set exm_child_cow control.
 * INPUT j: proposed new exm_child_cow value
 * OUTPUT (return value): exm_child_cow value
 * exm_child_cow <= 0   MAP_SHARED parent/child shared writable map
 * exm_child_cow  = 1   MAP_PRIVATE (in-core COW populated as written to, default)
 * exm_child_cow  = 2   map copy of backing file (MAP_SHARED, but on a private copy)
 * exm_child_cow  = 3   reserved for proposed future out of core COW
 * exm_child_cow  > 3   reserved
 *
 * exm_child_cow = 1 is the deafult. This is an _in-core_ copy on write image.
 * Data written in the child populate RAM one page at a time as written to
 * (and stay in RAM).
 *
 * exm_child_cow = 2 first fully copies the backing file in the child, then sets
 * up an out of core mapping to that. The copy uses sendfile for speed.
 */ 
int
exm_cow (int j)
{
  omp_set_nest_lock (&lock);
  exm_child_cow = j;
  omp_unset_nest_lock (&lock);
  return exm_child_cow;
}

/* Set and get threshold size.
 * INPUT j: proposed new exm_threshold size
 * OUTPUT (return value): exm_threshold size
 */
size_t
exm_threshold (size_t j)
{
  if (j > 0)
    {
      omp_set_nest_lock (&lock);
      exm_alloc_threshold = j;
      omp_unset_nest_lock (&lock);
    }
  return exm_alloc_threshold;
}

/* Set madvise option for an exm-allocated region
 * INPUT
 * addr: exm-allocated pointer address
 * advice: one of the madvise options MADV_NORMAL, MADV_RANDOM, MADV_SEQUENTIAL
 * OUTPUT
 * (return value): zero on success, -1 on error (see man madvise for errors)
 */
int
exm_madvise (void *addr, int advice)
{
  int j = -1;
  struct map *x;
  omp_set_nest_lock (&lock);
  HASH_FIND_PTR (flexmap, &addr, x);
  if (x)
    j = madvise (x->addr, x->length, advice);
  omp_unset_nest_lock (&lock);
  return j;
}

/* Set/retrieve the file directory path character string
 * INPUT p, a proposed new path string or NULL
 * Returns string with path set. When input is NULL, allocates output
 * and it is up to the caller to free the returned copy!!
 */
char *
exm_path (char *p)
{
  omp_set_nest_lock (&lock);
  if (p == NULL)
    {
      p = strndup (exm_data_path, EXM_MAX_PATH_LEN);
    }
  else
    {
      memset (exm_data_path, 0, EXM_MAX_PATH_LEN);
      snprintf (exm_data_path, EXM_MAX_PATH_LEN, "%s", p);
    }
  omp_unset_nest_lock (&lock);
  return p;
}

/* Lookup an address, returning NULL if the address is not found or a strdup
 * locally-allocated copy of the backing file path for the address. No guarantee
 * is made that the address or backing file will be valid after this call, so
 * it's really up to the caller to make sure free is not called on the address
 * simultaneously with this call. CALLER'S RESPONSIBILITY TO FREE RESULT!
 */
char *
exm_lookup (void *addr)
{
  char *f = NULL;
  struct map *x;
  omp_set_nest_lock (&lock);
  HASH_FIND_PTR (flexmap, &addr, x);
  if (x)
    f = strndup (x->path, EXM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return f;
}

/* Debugging function that iterates over hash table, printing entries
 * to stderr in order.
 */
void
exm_debug_list ()
{
  struct map *m, *tmp;
  omp_set_nest_lock (&lock);
  HASH_ITER (hh, flexmap, m, tmp)
  {
    fprintf(stderr, "%p, %lu, %s\n", m->addr, m->length, m->path);
  }
  omp_unset_nest_lock (&lock);
}
