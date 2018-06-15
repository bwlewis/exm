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
#include <signal.h>

void
check_error ()
{
  char *derror;
  if ((derror = dlerror ()) == NULL)
    return;
  fprintf (stderr, "%s\n", derror);
  _exit (1);
}

int
main (int argc, void **argv)
{
  int j;
  void *x;
  const char *y = "parent";
  char *path;
  size_t SIZE = 1000000;
  void *x1, *x2, *x3;

  size_t (*set_threshold) (size_t);
  int (*exm_madvise) (void *, int);
  int (*exm_cow) (int);
  char *(*exm_path) (char *);
  void (*exm_debug_list) (void);
  void *handle;
  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle)
    return 2;
  set_threshold = (size_t (*)(size_t)) dlsym (handle, "exm_threshold");
  check_error ();
  exm_madvise = (int (*)(void *, int)) dlsym (handle, "exm_madvise");
  check_error ();
  exm_cow = (int (*)(int)) dlsym (handle, "exm_cow");
  check_error ();
  exm_path = (char *(*)(char *)) dlsym (handle, "exm_path");
  check_error ();
  exm_debug_list = (void (*)(void)) dlsym (handle, "exm_debug_list");
  check_error ();
  dlclose (handle);

  printf ("> initial threshold %lu\n", (*set_threshold) (0));
  printf ("> set_threshold() %lu\n", (*set_threshold) (SIZE));

  path = (*exm_path) (NULL);
  printf ("> exm_path(NULL) %s\n", path);
  free (path);
  path = (*exm_path) ("/tmp");
  printf ("> exm_path(\"/tmp\") %s\n", path);

  printf ("> malloc below threshold\n");
  x = malloc (SIZE - 1);
  memcpy (x, (const void *) y, strlen (y));
  free (x);

  printf ("> malloc above threshold\n");
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> exm_madvise(x, 1) = %d\n", (*exm_madvise) ((void *) x, 1));
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


  printf ("> exm_debug_list()\n");
  x1 = malloc (SIZE + 1);
  x2 = malloc (SIZE + 1);
  x3 = malloc (SIZE + 1);
  exm_debug_list();
  free (x1);
  free (x2);
  free (x3);


  printf ("> malloc above threshold + copy on write fork\n");
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  pid_t p = fork ();
  if (p == 0)                   // child
    {
      sprintf (x, "child");
      printf ("> hello from %s process address %p\n", (char *) x, x);
      free (x); // unmaps in child, but can't unlink because parent
      x = malloc (SIZE + 1); // XXX A LEAK
      free(x); // avoids the leak, but would be nice to get working destructor
      sleep (2);
      exit (0);
    }
  sleep (1);
  kill (p, SIGTERM);
// If you comment out free(x) above, then the allocation in the child leaks
// above because finalize not run when process is terminated by a signal with
// no registered handler.  Also illustrate that child writes are COW.
  printf ("> hello from parent process address %p, value: %s\n", x, (char *) x);
  free (x);
  wait (0);

// This test is just as above but using a shared writable map between
// parent and child (note that the value printed out in the parent
// is written by the child).
  printf ("> malloc above threshold + shared writable map fork (%d)\n", exm_cow(0));
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  p = fork ();
  if (p == 0)                   // child
    {
      sprintf (x, "child");
      printf ("> hello from %s process address %p\n", (char *) x, x);
      free (x); // unmaps in child, but can't unlink because parent
      sleep (2);
      exit (0);
    }
  sleep (1);
  kill (p, SIGTERM);
  printf ("> hello from parent process address %p, value: %s\n", x, (char *) x);
  free (x);
  wait (0);

// This test is just as above but using sendfile to copy backing file
// in the child.
  printf ("> malloc above threshold + full copy map fork (%d)\n", exm_cow(2));
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  p = fork ();
  if (p == 0)                   // child
    {
      sprintf (x, "child");
      printf ("> hello from %s process address %p\n", (char *) x, x);
      free (x); // unmaps in child, but can't unlink because parent
      sleep (2);
      exit (0);
    }
  sleep (1);
  kill (p, SIGTERM);
  printf ("> hello from parent process address %p, value: %s\n", x, (char *) x);
  free (x);
  wait (0);

  printf ("> test complete\n");
  return 0;
}
