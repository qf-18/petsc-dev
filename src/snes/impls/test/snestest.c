#define PETSCSNES_DLL

#include "include/private/snesimpl.h"

typedef struct {
  PetscTruth complete_print;
} SNES_Test;

/*
     SNESSolve_Test - Tests whether a hand computed Jacobian 
     matches one compute via finite differences.
*/
#undef __FUNCT__  
#define __FUNCT__ "SNESSolve_Test"
PetscErrorCode SNESSolve_Test(SNES snes)
{
  Mat            A = snes->jacobian,B;
  Vec            x = snes->vec_sol;
  PetscErrorCode ierr;
  PetscInt       i;
  MatStructure   flg;
  PetscReal      nrm,gnorm;
  SNES_Test      *neP = (SNES_Test*)snes->data;

  PetscFunctionBegin;

  if (A != snes->jacobian_pre) {
    SETERRQ(PETSC_ERR_ARG_WRONG,"Cannot test with alternative preconditioner");
  }

  ierr = PetscPrintf(snes->comm,"Testing hand-coded Jacobian, if the ratio is\n");CHKERRQ(ierr);
  ierr = PetscPrintf(snes->comm,"O(1.e-8), the hand-coded Jacobian is probably correct.\n");CHKERRQ(ierr);
  if (!neP->complete_print) {
    ierr = PetscPrintf(snes->comm,"Run with -snes_test_display to show difference\n");CHKERRQ(ierr);
    ierr = PetscPrintf(snes->comm,"of hand-coded and finite difference Jacobian.\n");CHKERRQ(ierr);
  }

  for (i=0; i<3; i++) {
    if (i == 1) {ierr = VecSet(x,-1.0);CHKERRQ(ierr);}
    else if (i == 2) {ierr = VecSet(x,1.0);CHKERRQ(ierr);}
 
    /* compute both versions of Jacobian */
    ierr = SNESComputeJacobian(snes,x,&A,&A,&flg);CHKERRQ(ierr);
    if (!i) {ierr = MatConvert(A,MATSAME,MAT_INITIAL_MATRIX,&B);CHKERRQ(ierr);}
    ierr = SNESDefaultComputeJacobian(snes,x,&B,&B,&flg,snes->funP);CHKERRQ(ierr);
    if (neP->complete_print) {
      MPI_Comm    comm;
      PetscViewer viewer;
      ierr = PetscPrintf(snes->comm,"Finite difference Jacobian\n");CHKERRQ(ierr);
      ierr = PetscObjectGetComm((PetscObject)B,&comm);CHKERRQ(ierr);
      ierr = PetscViewerASCIIGetStdout(comm,&viewer);CHKERRQ(ierr);
      ierr = MatView(B,viewer);CHKERRQ(ierr);
    }
    /* compare */
    ierr = MatAXPY(B,-1.0,A,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
    ierr = MatNorm(B,NORM_FROBENIUS,&nrm);CHKERRQ(ierr);
    ierr = MatNorm(A,NORM_FROBENIUS,&gnorm);CHKERRQ(ierr);
    if (neP->complete_print) {
      MPI_Comm    comm;
      PetscViewer viewer;
      ierr = PetscPrintf(snes->comm,"Hand-coded Jacobian\n");CHKERRQ(ierr);
      ierr = PetscObjectGetComm((PetscObject)B,&comm);CHKERRQ(ierr);
      ierr = PetscViewerASCIIGetStdout(comm,&viewer);CHKERRQ(ierr);
      ierr = MatView(A,viewer);CHKERRQ(ierr);
    }
    ierr = PetscPrintf(snes->comm,"Norm of matrix ratio %G difference %G\n",nrm/gnorm,nrm);CHKERRQ(ierr);
  }
  ierr = MatDestroy(B);CHKERRQ(ierr);
  /*
         Return error code cause Jacobian not good
  */
  PetscFunctionReturn(PETSC_ERR_ARG_WRONGSTATE);
}
/* ------------------------------------------------------------ */
#undef __FUNCT__  
#define __FUNCT__ "SNESDestroy_Test"
PetscErrorCode SNESDestroy_Test(SNES snes)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = PetscFree(snes->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SNESSetFromOptions_Test"
static PetscErrorCode SNESSetFromOptions_Test(SNES snes)
{
  SNES_Test      *ls = (SNES_Test *)snes->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;

  ierr = PetscOptionsHead("Hand-coded Jacobian tester options");CHKERRQ(ierr);
    ierr = PetscOptionsName("-snes_test_display","Display difference between approximate and handcoded Jacobian","None",&ls->complete_print);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------ */
EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "SNESCreate_Test"
PetscErrorCode PETSCSNES_DLLEXPORT SNESCreate_Test(SNES  snes)
{
  SNES_Test      *neP;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  snes->ops->setup	     = 0;
  snes->ops->solve	     = SNESSolve_Test;
  snes->ops->destroy	     = SNESDestroy_Test;
  snes->ops->setfromoptions  = SNESSetFromOptions_Test;
  snes->ops->converged	     = 0;

  ierr			= PetscNew(SNES_Test,&neP);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(snes,sizeof(SNES_Test));CHKERRQ(ierr);
  snes->data    	= (void*)neP;
  neP->complete_print   = PETSC_FALSE;
  PetscFunctionReturn(0);
}
EXTERN_C_END




