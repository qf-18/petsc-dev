#!/usr/bin/env python
import os
import sys

if __name__ == '__main__':
    import configure

    configure_options = [
    '--with-mpi=0',
    '-PETSC_ARCH='+configure.getarch()
    ]

    configure.petsc_configure(configure_options)


