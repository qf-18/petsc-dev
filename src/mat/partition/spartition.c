/*$Id: spartition.c,v 1.23 2001/03/23 23:22:49 balay Exp $*/
 
#include "petsc.h"
#include "petscmat.h"

EXTERN_C_BEGIN
EXTERN int MatPartitioningCreate_Current(MatPartitioning);
EXTERN int MatPartitioningCreate_Square(MatPartitioning);
EXTERN int MatPartitioningCreate_Parmetis(MatPartitioning);
EXTERN int MatPartitioningCreate_Chaco(MatPartitioning);
EXTERN int MatPartitioningCreate_Jostle(MatPartitioning);
EXTERN int MatPartitioningCreate_Party(MatPartitioning);
EXTERN int MatPartitioningCreate_Scotch(MatPartitioning);
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "MatPartitioningRegisterAll" 
/*@C
  MatPartitioningRegisterAll - Registers all of the matrix Partitioning routines in PETSc.

  Not Collective

  Level: developer

  Adding new methods:
  To add a new method to the registry. Copy this routine and 
  modify it to incorporate a call to MatPartitioningRegisterDynamic() for 
  the new method, after the current list.

  Restricting the choices: To prevent all of the methods from being
  registered and thus save memory, copy this routine and modify it to
  register a zero, instead of the function name, for those methods you
  do not wish to register.  Make sure that the replacement routine is
  linked before libpetscmat.a.

.keywords: matrix, Partitioning, register, all

.seealso: MatPartitioningRegisterDynamic(), MatPartitioningRegisterDestroy()
@*/
int MatPartitioningRegisterAll(const char path[])
{
  int         ierr;

  PetscFunctionBegin;
  ierr = MatPartitioningRegisterDynamic(MAT_PARTITIONING_CURRENT,path,"MatPartitioningCreate_Current",MatPartitioningCreate_Current);CHKERRQ(ierr);
  ierr = MatPartitioningRegisterDynamic("square",path,"MatPartitioningCreate_Square",MatPartitioningCreate_Square);CHKERRQ(ierr);
#if defined(PETSC_HAVE_PARMETIS)
  ierr = MatPartitioningRegisterDynamic(MAT_PARTITIONING_PARMETIS,path,"MatPartitioningCreate_Parmetis",MatPartitioningCreate_Parmetis);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_CHACO)
  ierr = MatPartitioningRegisterDynamic(MAT_PARTITIONING_CHACO,path,"MatPartitioningCreate_Chaco",MatPartitioningCreate_Chaco);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_JOSTLE)
  ierr = MatPartitioningRegisterDynamic(MAT_PARTITIONING_JOSTLE,path,"MatPartitioningCreate_Jostle",MatPartitioningCreate_Jostle);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_PARTY)
  ierr = MatPartitioningRegisterDynamic(MAT_PARTITIONING_PARTY,path,"MatPartitioningCreate_Party",MatPartitioningCreate_Party);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_SCOTCH)
  ierr = MatPartitioningRegisterDynamic(MAT_PARTITIONING_SCOTCH,path,"MatPartitioningCreate_Scotch",MatPartitioningCreate_Scotch);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}



