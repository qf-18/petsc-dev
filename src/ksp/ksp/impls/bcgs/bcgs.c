
#include "src/ksp/ksp/kspimpl.h"

#undef __FUNCT__  
#define __FUNCT__ "KSPSetUp_BCGS"
static int KSPSetUp_BCGS(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (ksp->pc_side == PC_SYMMETRIC) {
    SETERRQ(PETSC_ERR_SUP,"no symmetric preconditioning for KSPBCGS");
  }
  ierr = KSPDefaultGetWork(ksp,6);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPSolve_BCGS"
static int  KSPSolve_BCGS(KSP ksp)
{
  int         i,ierr;
  PetscScalar rho,rhoold,alpha,beta,omega,omegaold,d1,d2,zero = 0.0,tmp;
  Vec         X,B,V,P,R,RP,T,S;
  PetscReal   dp = 0.0;

  PetscFunctionBegin;

  X       = ksp->vec_sol;
  B       = ksp->vec_rhs;
  R       = ksp->work[0];
  RP      = ksp->work[1];
  V       = ksp->work[2];
  T       = ksp->work[3];
  S       = ksp->work[4];
  P       = ksp->work[5];

  /* Compute initial preconditioned residual */
  ierr = KSPInitialResidual(ksp,X,V,T,R,B);CHKERRQ(ierr);

  /* Test for nothing to do */
  if (ksp->normtype != KSP_NO_NORM) {
    ierr = VecNorm(R,NORM_2,&dp);CHKERRQ(ierr);
  }
  ierr = PetscObjectTakeAccess(ksp);CHKERRQ(ierr);
  ksp->its   = 0;
  ksp->rnorm = dp;
  ierr = PetscObjectGrantAccess(ksp);CHKERRQ(ierr);
  KSPLogResidualHistory(ksp,dp);
  KSPMonitor(ksp,0,dp);
  ierr = (*ksp->converged)(ksp,0,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
  if (ksp->reason) PetscFunctionReturn(0);

  /* Make the initial Rp == R */
  ierr = VecCopy(R,RP);CHKERRQ(ierr);

  rhoold   = 1.0;
  alpha    = 1.0;
  omegaold = 1.0;
  ierr = VecSet(&zero,P);CHKERRQ(ierr);
  ierr = VecSet(&zero,V);CHKERRQ(ierr);

  i=0;
  do {
    ierr = VecDot(R,RP,&rho);CHKERRQ(ierr);       /*   rho <- (r,rp)      */
    if (rho == 0.0) {
      ksp->reason = KSP_DIVERGED_BREAKDOWN;
      break;
    }
    beta = (rho/rhoold) * (alpha/omegaold);
    tmp = -omegaold; VecAXPY(&tmp,V,P);            /*   p <- p - w v       */
    ierr = VecAYPX(&beta,R,P);CHKERRQ(ierr);      /*   p <- r + p beta    */
    ierr = KSP_PCApplyBAorAB(ksp,P,V,T);CHKERRQ(ierr);  /*   v <- K p           */
    ierr = VecDot(V,RP,&d1);CHKERRQ(ierr);
    alpha = rho / d1; tmp = -alpha;                /*   a <- rho / (v,rp)  */
    ierr = VecWAXPY(&tmp,V,R,S);CHKERRQ(ierr);    /*   s <- r - a v       */
    ierr = KSP_PCApplyBAorAB(ksp,S,T,R);CHKERRQ(ierr);/*   t <- K s    */
    ierr = VecDot(S,T,&d1);CHKERRQ(ierr);
    ierr = VecDot(T,T,&d2);CHKERRQ(ierr);
    if (d2 == 0.0) {
      /* t is 0.  if s is 0, then alpha v == r, and hence alpha p
	 may be our solution.  Give it a try? */
      ierr = VecDot(S,S,&d1);CHKERRQ(ierr);
      if (d1 == 0.0) {
        ksp->reason = KSP_DIVERGED_BREAKDOWN;
        break;
      }
      ierr = VecAXPY(&alpha,P,X);CHKERRQ(ierr);   /*   x <- x + a p       */
      ierr = PetscObjectTakeAccess(ksp);CHKERRQ(ierr);
      ksp->its++;
      ksp->rnorm  = 0.0;
      ksp->reason = KSP_CONVERGED_RTOL;
      ierr = PetscObjectGrantAccess(ksp);CHKERRQ(ierr);
      KSPLogResidualHistory(ksp,dp);
      KSPMonitor(ksp,i+1,0.0);
      break;
    }
    omega = d1 / d2;                               /*   w <- (t's) / (t't) */
    ierr  = VecAXPY(&alpha,P,X);CHKERRQ(ierr);     /*   x <- x + a p       */
    ierr  = VecAXPY(&omega,S,X);CHKERRQ(ierr);     /*   x <- x + w s       */
    tmp   = -omega; 
    ierr  = VecWAXPY(&tmp,T,S,R);CHKERRQ(ierr);    /*   r <- s - w t       */
    if (ksp->normtype != KSP_NO_NORM) {
      ierr = VecNorm(R,NORM_2,&dp);CHKERRQ(ierr);
    }

    rhoold   = rho;
    omegaold = omega;

    ierr = PetscObjectTakeAccess(ksp);CHKERRQ(ierr);
    ksp->its++;
    ksp->rnorm = dp;
    ierr = PetscObjectGrantAccess(ksp);CHKERRQ(ierr);
    KSPLogResidualHistory(ksp,dp);
    KSPMonitor(ksp,i+1,dp);
    ierr = (*ksp->converged)(ksp,i+1,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
    if (ksp->reason) break;    
    i++;
  } while (i<ksp->max_it);

  if (i == ksp->max_it) {
    ksp->reason = KSP_DIVERGED_ITS;
  }

  ierr = KSPUnwindPreconditioner(ksp,X,T);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
     KSPBCGS -     Implements the BiCGStab (Stabilized version of BiConjugate
         Gradient Squared) method.  Reference: van der Vorst, SIAM J. Sci. Stat. Comput., 1992.


   Options Database Keys:
.   see KSPSolve()

   Level: beginner

.seealso:  KSPCreate(), KSPSetType(), KSPType (for list of available types), KSP, KSPBICG

M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "KSPCreate_BCGS"
PetscErrorCode KSPCreate_BCGS(KSP ksp)
{
  PetscFunctionBegin;
  ksp->data                 = (void*)0;
  ksp->pc_side              = PC_LEFT;
  ksp->ops->setup           = KSPSetUp_BCGS;
  ksp->ops->solve           = KSPSolve_BCGS;
  ksp->ops->destroy         = KSPDefaultDestroy;
  ksp->ops->buildsolution   = KSPDefaultBuildSolution;
  ksp->ops->buildresidual   = KSPDefaultBuildResidual;
  ksp->ops->setfromoptions  = 0;
  ksp->ops->view            = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END
