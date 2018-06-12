#include <omp.h>
#include "uthash.h"

#define EXM_VERSION 0.1
#define EXM_MAX_PATH_LEN 4096
#define EXM_DEFAULT_ADVISE MADV_SEQUENTIAL

/* The map structure tracks the file mappings.  */
struct map
{
  void *addr;                   /* Memory address, hash key */
  char *path;                   /* File path */
  size_t length;                /* Mapping length */
  pid_t pid;                    /* Process ID of owner (for fork) */
  UT_hash_handle hh;            /* Make this thing uthash-hashable */
};

/* These global values can be changed using the basic API defined in api.c. */
extern char exm_data_path[];
extern size_t exm_alloc_threshold;
extern int exm_child_cow;

/* The global variable flexmap is a key-value list of addresses (keys) and file
 * paths (values). The recursive OpenMP lock is used widely in the library and
 * API functions.
 */
struct map *flexmap;
omp_nest_lock_t lock;
