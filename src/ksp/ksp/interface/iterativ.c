
/*
   This file contains some simple default routines.  
   These routines should be SHORT, since they will be included in every
   executable image that uses the iterative routines (note that, through
   the registry system, we provide a way to load only the truely necessary
   files) 
 */
#include "src/ksp/ksp/kspimpl.h"   /*I "petscksp.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultFreeWork"
/*
  KSPDefaultFreeWork - Free work vectors

  Input Parameters:
. ksp  - iterative context
 */
PetscErrorCode KSPDefaultFreeWork(KSP ksp)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (ksp->work)  {
    ierr = VecDestroyVecs(ksp->work,ksp->nwork);CHKERRQ(ierr);
    ksp->work = PETSC_NULL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetResidualNorm"
/*@C
   KSPGetResidualNorm - Gets the last (approximate preconditioned)
   residual norm that has been computed.
 
   Not Collective

   Input Parameters:
.  ksp - the iterative context

   Output Parameters:
.  rnorm - residual norm

   Level: intermediate

.keywords: KSP, get, residual norm

.seealso: KSPComputeResidual()
@*/
PetscErrorCode KSPGetResidualNorm(KSP ksp,PetscReal *rnorm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidDoublePointer(rnorm,2);
  *rnorm = ksp->rnorm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetIterationNumber"
/*@
   KSPGetIterationNumber - Gets the current iteration number; if the 
         KSPSolve() is complete, returns the number of iterations
         used.
 
   Not Collective

   Input Parameters:
.  ksp - the iterative context

   Output Parameters:
.  its - number of iterations

   Level: intermediate

   Notes:
      During the ith iteration this returns i-1
.keywords: KSP, get, residual norm

.seealso: KSPComputeResidual(), KSPGetResidualNorm()
@*/
PetscErrorCode KSPGetIterationNumber(KSP ksp,int *its)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidIntPointer(its,2);
  *its = ksp->its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPSingularValueMonitor"
/*@C
    KSPSingularValueMonitor - Prints the two norm of the true residual and
    estimation of the extreme singular values of the preconditioned problem
    at each iteration.
 
    Collective on KSP

    Input Parameters:
+   ksp - the iterative context
.   n  - the iteration
-   rnorm - the two norm of the residual

    Options Database Key:
.   -ksp_singmonitor - Activates KSPSingularValueMonitor()

    Notes:
    The CG solver uses the Lanczos technique for eigenvalue computation, 
    while GMRES uses the Arnoldi technique; other iterative methods do
    not currently compute singular values.

    Level: intermediate

.keywords: KSP, CG, default, monitor, extreme, singular values, Lanczos, Arnoldi

.seealso: KSPComputeExtremeSingularValues()
@*/
PetscErrorCode KSPSingularValueMonitor(KSP ksp,int n,PetscReal rnorm,void *dummy)
{
  PetscReal emin,emax,c;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (!ksp->calc_sings) {
    ierr = PetscPrintf(ksp->comm,"%3d KSP Residual norm %14.12e \n",n,rnorm);CHKERRQ(ierr);
  } else {
    ierr = KSPComputeExtremeSingularValues(ksp,&emax,&emin);CHKERRQ(ierr);
    c = emax/emin;
    ierr = PetscPrintf(ksp->comm,"%3d KSP Residual norm %14.12e %% max %g min %g max/min %g\n",n,rnorm,emax,emin,c);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPVecViewMonitor"
/*@C
   KSPVecViewMonitor - Monitors progress of the KSP solvers by calling 
   VecView() for the approximate solution at each iteration.

   Collective on KSP

   Input Parameters:
+  ksp - the KSP context
.  its - iteration number
.  fgnorm - 2-norm of residual (or gradient)
-  dummy - either a viewer or PETSC_NULL

   Level: intermediate

   Notes:
    For some Krylov methods such as GMRES constructing the solution at
  each iteration is expensive, hence using this will slow the code.

.keywords: KSP, nonlinear, vector, monitor, view

.seealso: KSPSetMonitor(), KSPDefaultMonitor(), VecView()
@*/
PetscErrorCode KSPVecViewMonitor(KSP ksp,int its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  Vec         x;
  PetscViewer viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  ierr = KSPBuildSolution(ksp,PETSC_NULL,&x);CHKERRQ(ierr);
  if (!viewer) {
    MPI_Comm comm;
    ierr   = PetscObjectGetComm((PetscObject)ksp,&comm);CHKERRQ(ierr);
    viewer = PETSC_VIEWER_DRAW_(comm);
  }
  ierr = VecView(x,viewer);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultMonitor"
/*@C
   KSPDefaultMonitor - Print the residual norm at each iteration of an
   iterative solver.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm (preconditioned) residual value (may be estimated).  
-  dummy - unused monitor context 

   Level: intermediate

.keywords: KSP, default, monitor, residual

.seealso: KSPSetMonitor(), KSPTrueMonitor(), KSPLGMonitorCreate()
@*/
PetscErrorCode KSPDefaultMonitor(KSP ksp,int n,PetscReal rnorm,void *dummy)
{
  PetscErrorCode ierr;
  PetscViewer viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_(ksp->comm);
  ierr = PetscViewerASCIIPrintf(viewer,"%3d KSP Residual norm %14.12e \n",n,rnorm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPTrueMonitor"
/*@C
   KSPTrueMonitor - Prints the true residual norm as well as the preconditioned
   residual norm at each iteration of an iterative solver.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm (preconditioned) residual value (may be estimated).  
-  dummy - unused monitor context 

   Options Database Key:
.  -ksp_truemonitor - Activates KSPTrueMonitor()

   Notes:
   When using right preconditioning, these values are equivalent.

   When using either ICC or ILU preconditioners in BlockSolve95 
   (via MATMPIROWBS matrix format), then use this monitor will
   print both the residual norm associated with the original
   (unscaled) matrix.

   Level: intermediate

.keywords: KSP, default, monitor, residual

.seealso: KSPSetMonitor(), KSPDefaultMonitor(), KSPLGMonitorCreate()
@*/
PetscErrorCode KSPTrueMonitor(KSP ksp,int n,PetscReal rnorm,void *dummy)
{
  PetscErrorCode ierr;
  Vec          resid,work;
  PetscReal    scnorm;
  PC           pc;
  Mat          A,B;
  PetscViewer  viewer = (PetscViewer) dummy;
  
  PetscFunctionBegin;
  ierr = VecDuplicate(ksp->vec_rhs,&work);CHKERRQ(ierr);
  ierr = KSPBuildResidual(ksp,0,work,&resid);CHKERRQ(ierr);

  /*
     Unscale the residual if the matrix is, for example, a BlockSolve matrix
    but only if both matrices are the same matrix, since only then would 
    they be scaled.
  */
  ierr = VecCopy(resid,work);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCGetOperators(pc,&A,&B,PETSC_NULL);CHKERRQ(ierr);
  if (A == B) {
    ierr = MatUnScaleSystem(A,PETSC_NULL,work);CHKERRQ(ierr);
  }
  ierr = VecNorm(work,NORM_2,&scnorm);CHKERRQ(ierr);
  ierr = VecDestroy(work);CHKERRQ(ierr);
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_(ksp->comm);
  ierr = PetscViewerASCIIPrintf(viewer,"%3d KSP preconditioned resid norm %14.12e true resid norm %14.12e\n",n,rnorm,scnorm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultSMonitor"
/*
  Default (short) KSP Monitor, same as KSPDefaultMonitor() except
  it prints fewer digits of the residual as the residual gets smaller.
  This is because the later digits are meaningless and are often 
  different on different machines; by using this routine different 
  machines will usually generate the same output.
*/
PetscErrorCode KSPDefaultSMonitor(KSP ksp,int its,PetscReal fnorm,void *dummy)
{
  PetscErrorCode ierr;
  PetscViewer viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_(ksp->comm);

  if (fnorm > 1.e-9) {
    ierr = PetscViewerASCIIPrintf(viewer,"%3d KSP Residual norm %g \n",its,fnorm);CHKERRQ(ierr);
  } else if (fnorm > 1.e-11){
    ierr = PetscViewerASCIIPrintf(viewer,"%3d KSP Residual norm %5.3e \n",its,fnorm);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerASCIIPrintf(viewer,"%3d KSP Residual norm < 1.e-11\n",its);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPSkipConverged"
/*@C
   KSPSkipConverged - Convergence test that NEVER returns as converged.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm residual value (may be estimated)
-  dummy - unused convergence context 

   Returns:
.  0 - always

   Notes:
   This is used as the convergence test with the option KSPSetNormType(ksp,KSP_NO_NORM),
   since norms of the residual are not computed. Convergence is then declared 
   after a fixed number of iterations have been used. Useful when one is 
   using CG or Bi-CG-stab as a smoother.
                    
   Level: advanced

.keywords: KSP, default, convergence, residual

.seealso: KSPSetConvergenceTest(), KSPSetTolerances(), KSPSetNormType()
@*/
PetscErrorCode KSPSkipConverged(KSP ksp,int n,PetscReal rnorm,KSPConvergedReason *reason,void *dummy)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultConverged"
/*@C
   KSPDefaultConverged - Determines convergence of
   the iterative solvers (default code).

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm residual value (may be estimated)
-  dummy - unused convergence context 

   Returns:
+   positive - if the iteration has converged;
.   negative - if residual norm exceeds divergence threshold;
-   0 - otherwise.

   Notes:
   KSPDefaultConverged() reaches convergence when
$      rnorm < MAX (rtol * rnorm_0, atol);
   Divergence is detected if
$      rnorm > dtol * rnorm_0,

   where 
+     rtol = relative tolerance,
.     atol = absolute tolerance.
.     dtol = divergence tolerance,
-     rnorm_0 = initial residual norm

   Use KSPSetTolerances() to alter the defaults for rtol, atol, dtol.

   The precise values of reason are macros such as KSP_CONVERGED_RTOL, which
   are defined in petscksp.h.

   Level: intermediate

.keywords: KSP, default, convergence, residual

.seealso: KSPSetConvergenceTest(), KSPSetTolerances(), KSPSkipConverged()
@*/
PetscErrorCode KSPDefaultConverged(KSP ksp,int n,PetscReal rnorm,KSPConvergedReason *reason,void *dummy)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidPointer(reason,4);
  *reason = KSP_CONVERGED_ITERATING;

  if (!n) {
    ksp->ttol   = PetscMax(ksp->rtol*rnorm,ksp->atol);
    ksp->rnorm0 = rnorm;
  }
  if (rnorm <= ksp->ttol) {
    if (rnorm < ksp->atol) {
      PetscLogInfo(ksp,"Linear solver has converged. Residual norm %g is less than absolute tolerance %g at iteration %d\n",rnorm,ksp->atol,n);
      *reason = KSP_CONVERGED_ATOL;
    } else {
      PetscLogInfo(ksp,"Linear solver has converged. Residual norm %g is less than relative tolerance %g times initial residual norm %g at iteration %d\n",rnorm,ksp->rtol,ksp->rnorm0,n);
      *reason = KSP_CONVERGED_RTOL;
    }
  } else if (rnorm >= ksp->divtol*ksp->rnorm0) {
    PetscLogInfo(ksp,"Linear solver is diverging. Initial residual norm %g, current residual norm %g at iteration %d\n",ksp->rnorm0,rnorm,n);
    *reason = KSP_DIVERGED_DTOL;
  } else if (rnorm != rnorm) {
    PetscLogInfo(ksp,"Linear solver has created a not a number (NaN) as the residual norm, declaring divergence\n");
    *reason = KSP_DIVERGED_DTOL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultBuildSolution"
/*
   KSPDefaultBuildSolution - Default code to create/move the solution.

   Input Parameters:
+  ksp - iterative context
-  v   - pointer to the user's vector  

   Output Parameter:
.  V - pointer to a vector containing the solution

   Level: advanced

.keywords:  KSP, build, solution, default

.seealso: KSPGetSolution(), KSPDefaultBuildResidual()
*/
PetscErrorCode KSPDefaultBuildSolution(KSP ksp,Vec v,Vec *V)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (ksp->pc_side == PC_RIGHT) {
    if (ksp->pc) {
      if (v) {ierr = KSP_PCApply(ksp,ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else {SETERRQ(PETSC_ERR_SUP,"Not working with right preconditioner");}
    } else {
      if (v) {ierr = VecCopy(ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else { *V = ksp->vec_sol;}
    }
  } else if (ksp->pc_side == PC_SYMMETRIC) {
    if (ksp->pc) {
      if (ksp->transpose_solve) SETERRQ(PETSC_ERR_SUP,"Not working with symmetric preconditioner and transpose solve");
      if (v) {ierr = PCApplySymmetricRight(ksp->pc,ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else {SETERRQ(PETSC_ERR_SUP,"Not working with symmetric preconditioner");}
    } else  {
      if (v) {ierr = VecCopy(ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else { *V = ksp->vec_sol;}
    }
  } else {
    if (v) {ierr = VecCopy(ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
    else { *V = ksp->vec_sol; }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultBuildResidual"
/*
   KSPDefaultBuildResidual - Default code to compute the residual.

   Input Parameters:
.  ksp - iterative context
.  t   - pointer to temporary vector
.  v   - pointer to user vector  

   Output Parameter:
.  V - pointer to a vector containing the residual

   Level: advanced

.keywords:  KSP, build, residual, default

.seealso: KSPDefaultBuildSolution()
*/
PetscErrorCode KSPDefaultBuildResidual(KSP ksp,Vec t,Vec v,Vec *V)
{
  PetscErrorCode ierr;
  MatStructure pflag;
  Vec          T;
  PetscScalar  mone = -1.0;
  Mat          Amat,Pmat;

  PetscFunctionBegin;
  PCGetOperators(ksp->pc,&Amat,&Pmat,&pflag);
  ierr = KSPBuildSolution(ksp,t,&T);CHKERRQ(ierr);
  ierr = KSP_MatMult(ksp,Amat,t,v);CHKERRQ(ierr);
  ierr = VecAYPX(&mone,ksp->vec_rhs,v);CHKERRQ(ierr);
  *V = v;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetVecs"
/*
  KSPDefaultGetWork - Gets a number of work vectors.

  Input Parameters:
. ksp  - iterative context
. nw   - number of work vectors to allocate

  Output Parameter:
.  work - the array of vectors created

 */
PetscErrorCode  KSPGetVecs(KSP ksp,int nw,Vec **work)
{
  PetscErrorCode ierr;
  Vec vec;

  PetscFunctionBegin;
  if (ksp->vec_rhs) vec = ksp->vec_rhs;
  else {
    Mat pmat;
    ierr = PCGetOperators(ksp->pc,0,&pmat,0);CHKERRQ(ierr);
    ierr = MatGetVecs(pmat,&vec,0);CHKERRQ(ierr);
  }
  ierr = VecDuplicateVecs(vec,nw,work);CHKERRQ(ierr);
  if (!ksp->vec_rhs) {
    ierr = VecDestroy(vec);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultGetWork"
/*
  KSPDefaultGetWork - Gets a number of work vectors.

  Input Parameters:
. ksp  - iterative context
. nw   - number of work vectors to allocate

  Notes:
  Call this only if no work vectors have been allocated 
 */
PetscErrorCode  KSPDefaultGetWork(KSP ksp,int nw)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (ksp->work) {ierr = KSPDefaultFreeWork(ksp);CHKERRQ(ierr);}
  ksp->nwork = nw;
  ierr = KSPGetVecs(ksp,nw,&ksp->work);CHKERRQ(ierr);
  PetscLogObjectParents(ksp,nw,ksp->work);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultDestroy"
/*
  KSPDefaultDestroy - Destroys a iterative context variable for methods with
  no separate context.  Preferred calling sequence KSPDestroy().

  Input Parameter: 
. ksp - the iterative context
*/
PetscErrorCode KSPDefaultDestroy(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (ksp->data) {
    ierr      = PetscFree(ksp->data);CHKERRQ(ierr);
    ksp->data = 0;
  }

  /* free work vectors */
  ierr = KSPDefaultFreeWork(ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetConvergedReason"
/*@C
   KSPGetConvergedReason - Gets the reason the KSP iteration was stopped.

   Not Collective

   Input Parameter:
.  ksp - the KSP context

   Output Parameter:
.  reason - negative value indicates diverged, positive value converged, see KSPConvergedReason

   Possible values for reason:
+  KSP_CONVERGED_RTOL (residual norm decreased by a factor of rtol)
.  KSP_CONVERGED_ATOL (residual norm less than atol)
.  KSP_CONVERGED_ITS (used by the preonly preconditioner that always uses ONE iteration) 
.  KSP_CONVERGED_QCG_NEG_CURVE
.  KSP_CONVERGED_QCG_CONSTRAINED
.  KSP_CONVERGED_STEP_LENGTH
.  KSP_DIVERGED_ITS  (required more than its to reach convergence)
.  KSP_DIVERGED_DTOL (residual norm increased by a factor of divtol)
.  KSP_DIVERGED_BREAKDOWN (generic breakdown in method)
-  KSP_DIVERGED_BREAKDOWN_BICG (Initial residual is orthogonal to preconditioned initial
                                residual. Try a different preconditioner, or a different initial guess.)
 

   Level: beginner

   Notes: Can only be called after the call the KSPSolve() is complete.

.keywords: KSP, nonlinear, set, convergence, test

.seealso: KSPSetConvergenceTest(), KSPDefaultConverged(), KSPSetTolerances(), KSPConvergedReason
@*/
PetscErrorCode KSPGetConvergedReason(KSP ksp,KSPConvergedReason *reason)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidPointer(reason,2);
  *reason = ksp->reason;
  PetscFunctionReturn(0);
}
