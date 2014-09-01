staunch
=======

staunch is the secure tizen launcher used for native
applications.

This current version is setting supplementary groups.
It is not using the standard component `security-manager`
that will do it (soon).

What is provided ?
------------------

When typing `make` in the subdirectory `src` it produces
the executables `staunch-manage` and `staunch-launch`.

`staunch-manage` prepares the link to the target for the launcher.

`staunch-launch` sets the supplementary groups and launches the target
program.


Installation
------------

```
# install and compile
git clone https://github.com/jobol/staunch
cd staunch/src
make
```



