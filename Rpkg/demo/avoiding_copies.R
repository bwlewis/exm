# Avoiding Copies in Forked Processes
#
# 1. Define a vector in a parent process
# 2. Fork two child processes
# 3. Modify the vector slightly in each child and sum
#
# The example avoids copying the entire vector x in the child processes by
# evaluating expressions in the same environment where x lives in each child
# process. Instead of copying the whole vector, only modified memory pages
# (typically 4k each) are copied in each child process.
#
# This can be important when the vector is huge yet the modifications are
# small.
library(parallel)
f = function()
{
  x = rep(0.0, 1e6)
  sum(x)
  tracemem(x)

  expr = expression({x[1]=1;sum(x)})
  c(sum(x), unlist(mclapply(1:2, function(i)
    {
      eval(expr, envir=parent.env(environment()))
    })))
}
f()

# cf. This variation which copies the _whole_ vector in each child
# (note the tracemem reports).
f = function()
{
  x = rep(0.0, 1e6)
  sum(x)
  tracemem(x)

  c(sum(x), unlist(mclapply(1:2, function(i)
    {
      x[1] = 1
      sum(x)
    })))
}
f()
