#' Retrieve and/or change the threshold beyond which allocations are
#' file-backed.
#'
#' @param nbytes the new threshold (in bytes)
#' @seealso \code{\link{exm_template}}
#' @examples
#' # Add another 1000 bytes to the threshold.
#' \dontrun{
#' exm_threshold(exm_threshold()+1000)
#' }
#' @export
exm_threshold <- function(nbytes=0)
{
  .Call("Rexm_threshold", as.numeric(nbytes), PACKAGE="exm")
}

#' Set and return the exm fork copy on write behavior.
#'
#' Control how forked child processes see memory defined by the parent process.
#' The default value is 'copy on write' which behaves similarly to the usual
#' (non-exm) fork behavior--writes to memory regions by child processes are
#' stored in RAM by each child process page by page using a copy on write
#' method. Alternatively, use 'duplicate' if you expect a forked child process
#' to make lots of changes that could exceed available system RAM. The
#' 'duplicate' option copies out of core backing files and sets up new
#' memory mappings in the child process--it incurs start up overhead. The
#' 'shared' option gives shared read/write, out of core access to parent and child
#' processes. However, it should be avoided as it can lead to memory corruption
#' between processes due to unexpected modification of R objects.
#'
#' @param value copy on write setting
#' @examples
#' # Make children duplicate memory
#' \dontrun{
#' exm_cow("duplicate")
#' }
#' @export
exm_cow <- function(value=c("copy on write", "shared", "duplicate"))
{
  value <- match.arg(value)
  api <- c("shared"=0, "copy on write"=1, "duplicate"=2)
  api[.Call("Rexm_cow", as.integer(api[value]), PACKAGE="exm") + 1]
}


#' Set or retrieve the exm data file path
#' @param path If missing, return the current data path. Otherwise set the
#'   data path to the specified value.
#' @export
exm_path <- function(path)
{
  if(missing(path)) .Call("Rexm_get_path", PACKAGE="exm")
  else .Call("Rexm_set_path", as.character(path), PACKAGE="exm")
}

#' Lookup the exm backing file for an object
#' @param object Any R object
#' @export
exm_lookup <- function(object)
{
  .Call("Rexm_lookup", object, PACKAGE="exm")
}

#' Lookup the exm backing file for an object
#' @param object Any R object
#' @param advice Mapping type, one of "normal", "random", or "sequential"
#' @return Integer return code from the OS-level madvise call, zero means success
#' @export
exm_madvise <- function(object, advice=c("normal", "random", "sequential"))
{
  advice <- as.integer(c(normal=0, random=1, sequential=2)[match.arg(advice)])
  .Call("Rexm_madvise", object, advice, PACKAGE="exm")
}

#' @export
exm_version <- function()
{
  .Call("Rexm_version", PACKAGE="exm")
}
