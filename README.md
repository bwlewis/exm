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

Description
---

Exm is a simple tool for out-of-core (OOC) computing.  It is launched as a
command line utility, taking an application as an argument. All memory
allocations larger than a specified threshold are memory-mapped to a binary
file. When data are not needed, they are stored on disk. It aims to be both
process- and thread-safe. It includes a simple API that can be used by programs
to dynamically fine tune out of core allocation details.

Requirements
---

OpenMP

Installing exm
---

The exm package can be installed by navigating to the 
`exm` directory using the following commands:

```bash
make all
sudo make install
```

Please note that you may get a warning message concerning `__malloc_hook`.
This is due to a recent change in gcc and is currently being worked through.
Also note that install will put the executable into /usr/local/bin and the
required shared object file inot /usr/local/lib.

General usage
---

```
exm <program> [program arguments]
```

Installing the R exm package
---

After installing exm the R exm package can be installed by
the following commands in a shell:

```bash
R CMD build Rpkg
R CMD INSTALL exm_0.1.tar.gz
```

The package documentation provides more information about how to use the 
R exm package.

Using exm
---

Start an application in exm by specifying the application as an argument

```r
exm R
```

The R package memory-mapped files are stored in the session R temporary
directory by default. This parameter along with others can be set using the
package functions.
