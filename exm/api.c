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

/* exm_fname_template is initialized in exm.c:exm_init() */
char exm_fname_template[EXM_MAX_PATH_LEN];
size_t exm_threshold = 2000000000;

/* The next functions allow applications to inspect and change default
 * settings. The application must dynamically locate them with dlsym after
 * the library is loaded. They represent the exm API, such as it is.
 *
 * API functions defined below include:
 * double exm_version()
 * size_t exm_set_threshold(size_t j)
 * int exm_set_path(char *path)
 * int exm_madvise(void *addr, int advice)
 * char * exm_lookup(void *addr)
 * char * exm_get_template()
 */

/* Return the exm library version
 * OUTPUT
 * (return value): double version major.minor
 */
double
exm_version (char *v)
{
  return EXM_VERSION;
}

/* Set and get threshold size.
 * INPUT
 * j: proposed new exm_threshold size
 * OUTPUT
 * (return value): exm_threshold size on exit
 */
size_t
exm_set_threshold (size_t j)
{
  if (j > 0)
    {
      omp_set_nest_lock (&lock);
      exm_threshold = j;
      omp_unset_nest_lock (&lock);
    }
  return exm_threshold;
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
printf("%p %d \n",addr,advice);
  HASH_FIND_PTR (flexmap, &addr, x);
  if (x)
    j = madvise (x->addr, x->length, advice);
printf("%p %d \n",x,advice);
  omp_unset_nest_lock (&lock);
  return j;
}


/* Set the file directory path character string
 * INPUT p, a proposed new path string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
exm_set_path (char *p)
{
  omp_set_nest_lock (&lock);
  memset (exm_fname_template, 0, EXM_MAX_PATH_LEN);
  snprintf (exm_fname_template, EXM_MAX_PATH_LEN, "%s/exm%ld_XXXXXX",
            p, (long int) getpid ());
  omp_unset_nest_lock (&lock);
  return 0;
}

/* Return a copy of the exm template (allocated internally...
 * it is up to the caller to free the returned copy!!)
 */
char *
exm_get_template ()
{
  char *s;
  omp_set_nest_lock (&lock);
  s = strndup(exm_fname_template, EXM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return s;
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

// XXX Also add a list all mappings function??
