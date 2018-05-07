#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <omp.h>

#include "uthash.h"
#include "exm.h"

static char exm_fname_pattern[EXM_MAX_PATH_LEN] = "XXXXXX";
static char exm_fname_path[EXM_MAX_PATH_LEN] = "/tmp";

char exm_fname_template[EXM_MAX_PATH_LEN] = "/tmp/exm_XXXXXX";
size_t exm_threshold = 2000000000;

int exm_advise = MADV_SEQUENTIAL;
int exm_offset = 0;

/* The next functions allow applications to inspect and change default
 * settings. The application must dynamically locate them with dlsym after
 * the library is loaded. They represent the exm API, such as it is.
 *
 * API functions defined below include:
 * size_t exm_set_threshold (size_t j)
 * int exm_set_template (char *template)
 * int exm_set_pattern (char *pattern)
 * int exm_set_path (char *path)
 * int exm_madvise (int j)
 * int exm_memcpy_offset (int j)
 * char * exm_lookup(void *addr)
 * char * exm_get_template()
 */

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

/* Set madvise option */
int
exm_madvise (int j)
{
  if(j> -1)
  {
    omp_set_nest_lock (&lock);
    exm_advise = j;
    omp_unset_nest_lock (&lock);
  }
  return exm_advise;
}

/* Set memcpy offset option */
int
exm_memcpy_offset (int j)
{
  if(j> -1)
  {
    omp_set_nest_lock (&lock);
    exm_offset = j;
    omp_unset_nest_lock (&lock);
  }
  return exm_offset;
}

/* Set the file template character string
 * INPUT name, a proposed new exm_fname_template string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
exm_set_template (char *name)
{
  char *s;
  int n = strlen(name);
  if(n<6) return -1;
/* Validate mkostemp file name */
  s = name + (n-6);
  if(strncmp(s, "XXXXXX", 6)!=0) return -2;
  omp_set_nest_lock (&lock);
  memset(exm_fname_template, 0, EXM_MAX_PATH_LEN);
  strncpy(exm_fname_template, name, EXM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Set the file pattern character string
 * INPUT name, a proposed new exm_fname_pattern string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
exm_set_pattern (char *name)
{
  char *s;
  int n = strlen(name);
  if(n<6) return -1;
/* Validate mkostemp file name */
  s = name + (n-6);
  if(strncmp(s, "XXXXXX", 6)!=0) return -2;
  omp_set_nest_lock (&lock);
  memset(exm_fname_template, 0, EXM_MAX_PATH_LEN);
  memset(exm_fname_pattern, 0, EXM_MAX_PATH_LEN);
  strncpy (exm_fname_pattern, name, EXM_MAX_PATH_LEN);
  snprintf(exm_fname_template, EXM_MAX_PATH_LEN, "%s/%s",
           exm_fname_path, exm_fname_pattern);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Set the file directory path character string
 * INPUT name, a proposed new exm_fname_path string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
exm_set_path (char *p)
{
  int n = strlen(p);
  if(n<6) return -1;
  omp_set_nest_lock (&lock);
  memset(exm_fname_template, 0, EXM_MAX_PATH_LEN);
  memset(exm_fname_path, 0, EXM_MAX_PATH_LEN);
  strncpy (exm_fname_path, p, EXM_MAX_PATH_LEN);
  snprintf(exm_fname_template, EXM_MAX_PATH_LEN, "%s/%s",
           exm_fname_path, exm_fname_pattern);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Return a copy of the exm_fname_template (allocated internally...
 * it is up to the caller to free the returned copy!! Yikes! My rationale
 * is this--how could I trust a buffer provided by the caller?)
 */
char *
exm_get_template()
{
  char *s;
  omp_set_nest_lock (&lock);
  s = strndup(exm_fname_template,EXM_MAX_PATH_LEN);
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
exm_lookup(void *addr)
{
  char *f = NULL;
  struct map *x;
  omp_set_nest_lock (&lock);
  HASH_FIND_PTR (flexmap, &addr, x);
  if(x) f = strndup(x->path,EXM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return f;
}
// XXX Also add a list all mappings function??
