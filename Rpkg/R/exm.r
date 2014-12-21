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

#' Set the exm temp file pattern.
#' @param pattern A valid tmp file pattern containing the string "XXXXXX"
#' @export
exm_set_pattern <- function(pattern="fm_XXXXXX")
{
  .Call("Rexm_set_pattern", as.character(pattern), PACKAGE="exm")
}

#' Set the exm temp file path
#' @param pattern A valid path
#' @export
exm_set_path <- function(path="/tmp")
{
# XXX validate path first.
  .Call("Rexm_set_path", as.character(path), PACKAGE="exm")
}

#' Return the exm temp directory/pattern name template
#' @export
exm_get_template <- function()
{
  .Call("Rexm_get_template", PACKAGE="exm")
}

#' Lookup the exm backing file for an object
#' @param object Any R object
#' @export
exm_lookup <- function(object)
{
  .Call("Rexm_lookup", object, PACKAGE="exm")
}
