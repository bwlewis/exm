#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>

void testfunc (void);

void
check_error ()
{
  char *derror;
  if ((derror = dlerror ()) == NULL) return;
  fprintf(stderr, "%s\n", derror);
  _exit (1);
}

int
main (int argc, void **argv)
{
  int j;
  void *x;
  const char *y = "Cazart!";
  char *path;
  size_t SIZE = 1000000;

  size_t (*set_threshold) (size_t);
  int (*exm_madvise) (void *, int);
  char * (*exm_path)(char *);
  void *handle;
  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) return 2;
  set_threshold = (size_t (*)(size_t ))dlsym(handle, "exm_set_threshold");
  check_error ();
  exm_madvise = (int (*)(void *, int))dlsym(handle, "exm_madvise");
  check_error ();
  exm_path = (char *(*)(char *))dlsym(handle, "exm_path");
  check_error ();
  dlclose (handle);

  printf("> set_threshold() %lu\n", (*set_threshold) (SIZE));

  path = (*exm_path)(NULL);
  printf("> exm_path(NULL) %s\n", path);
  free(path);
  path = (*exm_path)("/tmp");
  printf("> exm_path(\"/tmp\") %s\n", path);

  printf ("> malloc below threshold\n");
  x = malloc (SIZE - 1);
  memcpy (x, (const void *) y, strlen (y));
  free (x);

  printf ("> malloc above threshold\n");
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> exm_madvise(x, 1) = %d\n", (*exm_madvise)((void *)x, 1));
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

  printf ("> malloc above threshold + fork\n");
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  pid_t p = fork();
  if(p == 0) // child
  { 
    sprintf(x, "child");
    printf("> hello from %s process\n", (char *)x);
    exit(0);
  }
  wait(0);
  sprintf(x, "parent");
  printf("> hello from %s process\n", (char *)x);
  free (x);

  printf("> test OK\n");
  return 0;
}
