#!/usr/bin/env python
# See bin/maint/iphonebuilder.py for instructions
# Note that this "cheats" and runs all the ./configure tests on the Mac NOT on the iPhone
#     but this is ok because the answers are the same.
# Force a 32 bit compile because that is what ARM supports.
# No need to provide BLAS/LAPACK because Mac and iPhone support the same Accelerate framework
#
if __name__ == '__main__':
  import sys
  import os
  sys.path.insert(0, os.path.abspath('config'))
  import configure
  configure_options = [
    '--with-cc=gcc -m32',
    '--with-mpi=0',
    '--with-iphone=1',
    '--with-valgrind=0',
    '--with-x=0',
    '--with-fc=0',
    'PETSC_ARCH=arch-iphone',
  ]
  configure.petsc_configure(configure_options)
