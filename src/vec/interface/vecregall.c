
#include "vecimpl.h"     /*I  "vec.h"  I*/
EXTERN_C_BEGIN
EXTERN PetscErrorCode VecCreate_Seq(Vec);
EXTERN PetscErrorCode VecCreate_MPI(Vec);
EXTERN PetscErrorCode VecCreate_Shared(Vec);
EXTERN PetscErrorCode VecCreate_FETI(Vec);
EXTERN PetscErrorCode VecCreate_ESI(Vec);
EXTERN PetscErrorCode VecCreate_PetscESI(Vec);
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "VecRegisterAll"
/*@C
  VecRegisterAll - Registers all of the vector components in the Vec package.

  Not Collective

  Input parameter:
. path - The dynamic library path

  Level: advanced

.keywords: Vec, register, all
.seealso:  VecRegister(), VecRegisterDestroy(), VecRegisterDynamic()
@*/
PetscErrorCode VecRegisterAll(const char path[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  VecRegisterAllCalled = PETSC_TRUE;

  ierr = VecRegisterDynamic(VECSEQ,      path, "VecCreate_Seq",      VecCreate_Seq);CHKERRQ(ierr);
  ierr = VecRegisterDynamic(VECMPI,      path, "VecCreate_MPI",      VecCreate_MPI);CHKERRQ(ierr);
  ierr = VecRegisterDynamic(VECSHARED,   path, "VecCreate_Shared",   VecCreate_Shared);CHKERRQ(ierr);
  ierr = VecRegisterDynamic(VECFETI,     path, "VecCreate_FETI",     VecCreate_FETI);CHKERRQ(ierr);
#if defined(__cplusplus) && !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_SINGLE) && defined(PETSC_HAVE_CXX_NAMESPACE)
  ierr = VecRegisterDynamic(VECESI,      path, "VecCreate_ESI",      VecCreate_ESI);CHKERRQ(ierr);
  ierr = VecRegisterDynamic(VECPETSCESI, path, "VecCreate_PetscESI", VecCreate_PetscESI);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

