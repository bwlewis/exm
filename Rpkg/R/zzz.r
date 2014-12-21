
.onLoad <- function(libname, pkgname) {
  library.dynam("exm", pkgname, libname)
}

.onUnload <- function(libpath) {
  library.dynam.unload("exm", libpath)
}
