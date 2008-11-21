!
!
!  Include file for Fortran use of the PC (preconditioner) package in PETSc
!
#if !defined (__PETSCPCDEF_H)
#define __PETSCPCDEF_H

#include "finclude/petscmatdef.h"
#include "finclude/petscdadef.h"

#if !defined(PETSC_USE_FORTRAN_TYPES)
#define PC PetscFortranAddr
#endif
#define PCSide PetscEnum
#define PCASMType PetscEnum
#define PCCompositeType PetscEnum
#define PCRichardsonConvergedReason PetscEnum 
#define PCType character*(80)
!
!  Various preconditioners
!
#define PCNONE 'none'
#define PCJACOBI 'jacobi'
#define PCSOR 'sor'
#define PCLU 'lu'
#define PCSHELL 'shell'
#define PCBJACOBI 'bjacobi'
#define PCMG 'mg'
#define PCEISENSTAT 'eisenstat'
#define PCILU 'ilu'
#define PCICC 'icc'
#define PCASM 'asm'
#define PCKSP 'ksp'
#define PCCOMPOSITE 'composite'
#define PCREDUNDANT 'redundant'
#define PCSPAI 'spai'
#define PCMILU 'milu'
#define PCNN 'nn'
#define PCCHOLESKY 'cholesky'
#define PCSAMG 'samg'
#define PCPBJACOBI 'pbjacobi'
#define PCMAT 'mat'
#define PCHYPRE 'hypre'
#define PCFIELDSPLIT 'fieldsplit'
#define PCML 'ml'

#endif
