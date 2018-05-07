
.onLoad <- function(libname, pkgname) {
  library.dynam("exm", pkgname, libname)
  .Call("Rexm_set_path", as.character(tempdir()), PACKAGE="exm")
}

.onUnload <- function(libpath) {
  library.dynam.unload("exm", libpath)
}
