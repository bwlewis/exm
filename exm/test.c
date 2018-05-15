#include <fcntl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>

void testfunc (void);

int
main (int argc, void **argv)
{
  int j;
  void *x;
  char *derror;
  const char *y = "Cazart!";
  size_t SIZE = 1000000;

  size_t (*set_threshold) (size_t);
  int (*_madvise) (void *, int);
  char * (*get_template)(void);
  int (*set_path)(char *);
  void *handle;
  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) return 2;
  dlerror ();
  set_threshold = (size_t (*)(size_t ))dlsym(handle, "exm_set_threshold");
  _madvise = (int (*)(void *, int))dlsym(handle, "exm_madvise");
  set_path = (int (*)(char *))dlsym(handle, "exm_set_path");
  get_template = (char *(*)(void))dlsym(handle, "exm_get_template");
  if ((derror = dlerror ()) == NULL)  (*set_threshold) (SIZE);

  printf("> get_template() %s\n", (*get_template) ());
  (*set_path)("/tmp");
  printf("> get_template() %s\n", (*get_template) ());

  printf ("> malloc below threshold\n");
  x = malloc (SIZE - 1);
  memcpy (x, (const void *) y, strlen (y));
  free (x);

  printf ("> malloc above threshold\n");
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> exm_madvise(x, 1) = %d\n", _madvise((void *)x, 1));
  free (x);


  printf ("> calloc below threshold\n");
  x = calloc (SIZE - 1, 1);
  memcpy (x, (const void *) y, strlen (y));
  free (x);

  printf ("> calloc above threshold\n");
  x = calloc (SIZE + 1, 1);
  memcpy (x, (const void *) y, strlen (y));
  free (x);


  printf ("> malloc + realloc below threshold\n");
  x = malloc (SIZE - 1);
  x = realloc (x, SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  free (x);

  printf ("> malloc + realloc above threshold\n");
  x = malloc (SIZE + 1);
  x = realloc (x, SIZE + 10);
  memcpy (x, (const void *) y, strlen (y));
  free (x);

  dlclose (handle);
  printf("> test OK\n");
  return 0;
}
