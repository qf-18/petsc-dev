/* 
   Default and runtime options used by seq and MPI Spooles' interface for both aij and sbaij mat objects
*/

#include "src/mat/impls/aij/seq/spooles/spooles.h"

/* Set Spooles' default and runtime options */
#undef __FUNCT__  
#define __FUNCT__ "SetSpoolesOptions"
PetscErrorCode SetSpoolesOptions(Mat A, Spooles_options *options)
{
  PetscErrorCode ierr;
  int          indx;
  const char   *ordertype[]={"BestOfNDandMS","MMD","MS","ND"};
  PetscTruth   flg;

  PetscFunctionBegin;	
  /* set default input parameters */ 
#if defined(PETSC_USE_COMPLEX)
  options->typeflag       = SPOOLES_COMPLEX;
#else
  options->typeflag       = SPOOLES_REAL;
#endif
  options->msglvl         = 0;
  options->msgFile        = 0;
  options->tau            = 100.; 
  options->seed           = 10101;  
  options->ordering       = 0;     /* BestOfNDandMS */
  options->maxdomainsize  = 500;
  options->maxzeros       = 1000;
  options->maxsize        = 96;   
  options->FrontMtxInfo   = PETSC_FALSE; 
  if ( options->symflag == SPOOLES_SYMMETRIC ) { /* || SPOOLES_HERMITIAN */
    options->patchAndGoFlag = 0;  /* no patch */
    options->storeids       = 1; 
    options->storevalues    = 1;
    options->toosmall       = 1.e-9;
    options->fudge          = 1.e-9;
    
  } 

  /* get runtime input parameters */
  ierr = PetscOptionsBegin(A->comm,A->prefix,"Spooles Options","Mat");CHKERRQ(ierr); 

    ierr = PetscOptionsReal("-mat_spooles_tau","tau (used for pivoting; \n\
           all entries in L and U have magnitude no more than tau)","None",
                            options->tau,&options->tau,PETSC_NULL);CHKERRQ(ierr);

    ierr = PetscOptionsInt("-mat_spooles_seed","random number seed, used for ordering","None",
                           options->seed,&options->seed,PETSC_NULL);CHKERRQ(ierr);

    if (PetscLogPrintInfo) options->msglvl = 1;
    ierr = PetscOptionsInt("-mat_spooles_msglvl","msglvl","None",
                           options->msglvl,&options->msglvl,0);CHKERRQ(ierr); 
    if (options->msglvl > 0) {
        options->msgFile = fopen("spooles.msgFile", "a");
        PetscPrintf(PETSC_COMM_SELF,"\n Spooles' output is written into the file 'spooles.msgFile' \n\n");
    } 

    ierr = PetscOptionsEList("-mat_spooles_ordering","ordering type","None",ordertype,4,ordertype[0],&indx,&flg);CHKERRQ(ierr);
    if (flg) {options->ordering = indx;}
   
    ierr = PetscOptionsInt("-mat_spooles_maxdomainsize","maxdomainsize","None",\
                           options->maxdomainsize,&options->maxdomainsize,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsInt("-mat_spooles_maxzeros ","maxzeros","None",\
                           options->maxzeros,&options->maxzeros,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsInt("-mat_spooles_maxsize","maxsize","None",\
                           options->maxsize,&options->maxsize,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsLogical("-mat_spooles_FrontMtxInfo","FrontMtxInfo","None",PETSC_FALSE,&flg,0);CHKERRQ(ierr);
    if (flg) options->FrontMtxInfo = PETSC_TRUE; 

    if ( options->symflag == SPOOLES_SYMMETRIC ) {
      ierr = PetscOptionsInt("-mat_spooles_symmetryflag","matrix type","None", \
                           options->symflag,&options->symflag,PETSC_NULL);CHKERRQ(ierr);

      ierr = PetscOptionsInt("-mat_spooles_patchAndGoFlag","patchAndGoFlag","None", \
                           options->patchAndGoFlag,&options->patchAndGoFlag,PETSC_NULL);CHKERRQ(ierr);
      
      ierr = PetscOptionsReal("-mat_spooles_fudge","fudge","None", \
                           options->fudge,&options->fudge,PETSC_NULL);CHKERRQ(ierr);
      ierr = PetscOptionsReal("-mat_spooles_toosmall","toosmall","None", \
                           options->toosmall,&options->toosmall,PETSC_NULL);CHKERRQ(ierr);
      ierr = PetscOptionsInt("-mat_spooles_storeids","storeids","None", \
                           options->storeids,&options->storeids,PETSC_NULL);CHKERRQ(ierr);
      ierr = PetscOptionsInt("-mat_spooles_storevalues","storevalues","None", \
                           options->storevalues,&options->storevalues,PETSC_NULL);CHKERRQ(ierr);    
    }
  PetscOptionsEnd(); 

  PetscFunctionReturn(0);
}

/* used by -ksp_view */
#undef __FUNCT__  
#define __FUNCT__ "MatFactorInfo_Spooles"
PetscErrorCode MatFactorInfo_Spooles(Mat A,PetscViewer viewer)
{
  Mat_Spooles    *lu = (Mat_Spooles*)A->spptr; 
  PetscErrorCode ierr;
  int            size;
  char           *s;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(A->comm,&size);CHKERRQ(ierr);
  
  ierr = PetscViewerASCIIPrintf(viewer,"Spooles run parameters:\n");CHKERRQ(ierr);
  switch (lu->options.symflag) {
  case 0: s = "SPOOLES_SYMMETRIC"; break;
  case 1: s = "SPOOLES_HERMITIAN"; break;
  case 2: s = "SPOOLES_NONSYMMETRIC"; break; }
  ierr = PetscViewerASCIIPrintf(viewer,"  symmetryflag:   %s \n",s);CHKERRQ(ierr);

  switch (lu->options.pivotingflag) {
  case 0: s = "SPOOLES_NO_PIVOTING"; break;
  case 1: s = "SPOOLES_PIVOTING"; break; }
  ierr = PetscViewerASCIIPrintf(viewer,"  pivotingflag:   %s \n",s);CHKERRQ(ierr);

  ierr = PetscViewerASCIIPrintf(viewer,"  tau:            %g \n",lu->options.tau);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  seed:           %d \n",lu->options.seed);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  msglvl:         %d \n",lu->options.msglvl);CHKERRQ(ierr);

  switch (lu->options.ordering) {
  case 0: s = "BestOfNDandMS"; break;  
  case 1: s = "MMD"; break;
  case 2: s = "MS"; break;
  case 3: s = "ND"; break;
  }
  ierr = PetscViewerASCIIPrintf(viewer,"  ordering:       %s \n",s);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  maxdomainsize:  %d \n",lu->options.maxdomainsize);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  maxzeros:       %d \n",lu->options.maxzeros);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  maxsize:        %d \n",lu->options.maxsize);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  FrontMtxInfo:   %d \n",lu->options.FrontMtxInfo);CHKERRQ(ierr);

  if ( lu->options.symflag == SPOOLES_SYMMETRIC ) {
    ierr = PetscViewerASCIIPrintf(viewer,"  patchAndGoFlag: %d \n",lu->options.patchAndGoFlag);CHKERRQ(ierr);
    if ( lu->options.patchAndGoFlag > 0 ) {
      ierr = PetscViewerASCIIPrintf(viewer,"  fudge:          %g \n",lu->options.fudge);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  toosmall:       %g \n",lu->options.toosmall);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  storeids:       %d \n",lu->options.storeids);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  storevalues:    %d \n",lu->options.storevalues);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}
