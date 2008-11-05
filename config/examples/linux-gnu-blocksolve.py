#!/usr/bin/env python

# ******* Currently not tested **********

configure_options = [
  '--with-mpi-include=/home/petsc/soft/linux-rh73/mpich-1.2.4/include',
  '--with-mpi-lib=[/home/petsc/soft/linux-rh73/mpich-1.2.4/lib/libmpich.a,/home/petsc/soft/linux-rh73/mpich-1.2.4/lib/libpmpich.a]',
  '--with-mpiexec=mpiexec -all-local',
  '--with-blocksolve95-lib=/home/petsc/software/BlockSolve95/lib/libO/linux/libBS95.a',
  '--with-cc=gcc'
  ]

if __name__ == '__main__':
  import sys,os
  sys.path.insert(0,os.path.abspath('config'))
  import configure
  configure.petsc_configure(configure_options)
