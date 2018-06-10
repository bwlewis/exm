```
  ___  _  ______ ___ 
 / _ \| |/_/ __ `__ \
/  __/>  </ / / / / /
\___/_/|_/_/ /_/ /_/ 
                     
```

Easy eXtended Memory

Exm provides utilities to selectively override the memory allocator,
allowing users to create out-of-core data structures that may be much
larger than available RAM.


## Description

Exm is a simple tool for out-of-core (OOC) computing.  It is launched as a
command line utility, taking an application as an argument. All memory
allocations larger than a specified threshold are memory-mapped to a binary
file. When data are not needed, they are stored on disk. It aims to be both
process- and thread-safe. It includes an optional, simple API that can be used
by programs to dynamically fine tune out of core allocation details.

Use exm with, for instance, a fast NVME flash device. Exm complements Linux
system swap (also on a fast storage device) and works best together with swap.
Linux swap efficiently manages situations in which lots of smaller allocations
exceed available memory, while exm handles large allocations.


## Requirements

OpenMP

## Installing exm

The exm package can be installed by navigating to the 
`src` directory using the following commands:

```bash
sudo make install
```

Default install locations are executables in `/usr/local/bin` and libraries in
`/usr/local/lib`. You can change the `/usr/local` part by setting the
environment variable `PREFIX`. Uninstall with
```bash
sudo make uninstall
```

## General use

```bash
exm <program> [program arguments]
```
Optionally set a default threshold size, beyond which memory allocations
are disk-based in TMPDIR:
```bash
EXM_THRESHOLD=<bytes> exm <program> [program arguments]
```

# The R exm package

The R exm package provides access from R to the exm API for setting
data paths and memory allocation thresholds. It can be installed from
the command line with:

```bash
R CMD build Rpkg
R CMD INSTALL exm_0.1.tar.gz
```

The package documentation provides more information about how to use the 
R exm package.

## Using exm with R

Start an application in exm by specifying the application as an argument

```r
exm R
```

The R package memory-mapped files are stored in the session R temporary
directory by default. This parameter along with others can be set using the
package functions.
