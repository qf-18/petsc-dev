#!/usr/bin/env python

if __name__ == '__main__':
    import configure

    # build on harley
    configure_options = [
        '--with-cc=icc',
	'--with-fc=ifc',
	'--with-cxx=icc',
        '--with-mpi-include=/home/petsc/soft/linux-rh73-intel/mpich-1.2.5/include',
        '--with-mpi-lib=[/home/petsc/soft/linux-rh73-intel/mpich-1.2.5/lib/libmpich.a,libpmpich.a]',
        '--with-mpirun=mpirun',
	'--with-blas-lapck-dir=/home/petsc/soft/linux-rh73-intel/mkl-52',
        '-PETSC_ARCH='+configure.getarch()
        ]

    configure.petsc_configure(configure_options)
