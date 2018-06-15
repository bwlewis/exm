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
