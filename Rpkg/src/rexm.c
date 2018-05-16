#include <dlfcn.h>

#include <R.h>
#define USE_RINTERNALS
#include <Rinternals.h>

/*
 * exm_threshold
 * INPUT J SEXP   Threshold value or R_NilValue
 * OUTPUT  SEXP   Threshold set point or R_NilValue on error
 */
SEXP
Rexm_threshold (SEXP J)
{
  SEXP VAL;
  void *handle;
  size_t size;
  size_t (*set_threshold)(size_t );
  char *derror;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlerror ();
  set_threshold = (size_t (*)(size_t ))dlsym(handle, "exm_set_threshold");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlclose (handle);

  PROTECT (VAL = allocVector(REALSXP, 1));
  size = (size_t) *(REAL (J));
  REAL(VAL)[0] = (double) (*set_threshold)(size);
  UNPROTECT (1);
  return VAL;
}

/*
 * exm_set_path
 *
 * INPUT S SEXP   A path to map files in
 * OUTPUT An SEXP integer, 0 for success otherwise error
 *
 */
SEXP
Rexm_set_path (SEXP S)
{
  void *handle;
  char * (*exm_path)(char *);
  char *s;
  char *derror;
  char *temp = (char *)CHAR (STRING_ELT (S, 0));

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlerror ();
  exm_path = (char *(*)(char *))dlsym(handle, "exm_path");
  if ((derror = dlerror ()) != NULL)  {
      return R_NilValue;
  }
  dlclose (handle);
  s = (*exm_path)(temp);
  if(!s) return R_NilValue;
  return S;
}


SEXP
Rexm_get_path ()
{
  SEXP VAL;
  void *handle;
  char * (*exm_path)(char *);
  char *derror;
  char *s;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlerror ();
  exm_path = (char *(*)(char *))dlsym(handle, "exm_path");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlclose (handle);
  s = (*exm_path)(NULL);
  if(!s) return R_NilValue;
  PROTECT (VAL = mkString(s));
  UNPROTECT (1);
  free (s);
  return VAL;
}

SEXP
Rexm_lookup (SEXP OBJECT)
{
  SEXP VAL;
  void *handle;
  char * (*flookup)(void *);
  char *derror, *s;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlerror ();
  flookup = (char *(*)(void *))dlsym(handle, "exm_lookup");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlclose (handle);
  s = (*flookup)((void *)OBJECT);
  if (!s) return R_NilValue;
  PROTECT (VAL = mkString(s));
  UNPROTECT (1);
  free (s);
  return VAL;
}


SEXP
Rexm_madvise (SEXP OBJECT, SEXP ADVICE)
{
  SEXP VAL;
  void *handle;
  int (*advise)(void *, int);
  char *derror;
  int j = *(INTEGER(ADVICE));

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlerror ();
  advise = (int (*)(void *, int))dlsym(handle, "exm_madvise");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlclose (handle);
  PROTECT (VAL = allocVector(INTSXP, 1));
  INTEGER (VAL)[0] = (*advise)((void *)OBJECT, j);
  UNPROTECT (1);
  return VAL;
}

SEXP
Rexm_version ()
{
  SEXP VAL;
  void *handle;
  char *derror;
  double (*ver)();

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlerror ();
  ver = (double (*)())dlsym(handle, "exm_version");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n", dlerror ());
      return R_NilValue;
  }
  dlclose (handle);
  PROTECT (VAL = allocVector(REALSXP, 1));
  REAL (VAL)[0] = (*ver)();
  UNPROTECT (1);
  return VAL;
}
