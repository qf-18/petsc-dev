#include "private/fortranimpl.h"
#include "petscsnes.h"

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matmffdsetfunction_            MATMFFDSETFUNCTION
#define matmffdsettype_                MATMFFDSETTYPE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matmffdsetfunction_            matmffdsetfunction
#define matmffdsettype_                matmffdsettype
#endif

static PetscErrorCode ourmatmffdfunction(void *ctx,Vec x,Vec f)
{
  PetscErrorCode ierr = 0;
  Mat            mat = (Mat) ctx;
  (*(void (PETSC_STDCALL *)(void*,Vec*,Vec*,PetscErrorCode*))(((PetscObject)mat)->fortran_func_pointers[0]))(((PetscObject)mat)->fortran_func_pointers[1],&x,&f,&ierr);CHKERRQ(ierr);
  return 0;
}

EXTERN_C_BEGIN
void PETSC_STDCALL matmffdsetfunction_(Mat *mat,void (PETSC_STDCALL *func)(void*,Vec*,Vec*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECT(ctx);
  PetscObjectAllocateFortranPointers(*mat,2);
  ((PetscObject)*mat)->fortran_func_pointers[0] = (PetscVoidFunction)func;
  ((PetscObject)*mat)->fortran_func_pointers[1] = (PetscVoidFunction)ctx;
  *ierr = MatMFFDSetFunction(*mat,ourmatmffdfunction,*mat);
}

void PETSC_STDCALL matmffdsettype_(Mat *mat,CHAR ftype PETSC_MIXED_LEN(len),PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;
  FIXCHAR(ftype,len,t);
  *ierr = MatMFFDSetType(*mat,t);
  FREECHAR(ftype,t);
}

EXTERN_C_END
