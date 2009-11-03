static char help[] = "Test MatMult() for Hermitian matrix.\n\n";

#include "petscmat.h"
extern PetscErrorCode MatMult_SeqSBAIJ_1_Hermitian_tmp(Mat,Vec,Vec);
#undef __FUNCT__
#define __FUNCT__ "main"
PetscInt main(PetscInt argc,char **args)
{
  Mat            A,As;    
  Vec            x,y,ys;
  PetscTruth     flg,disp_mat=PETSC_FALSE,disp_vec=PETSC_FALSE;  
  PetscErrorCode ierr;
  PetscScalar    sigma;
  PetscMPIInt    size;
  PetscInt       m,i,j; 
  PetscScalar    v,none = -1.0,sigma2,pfive = 0.5;
  PetscRandom    rctx;
  PetscReal      h2,sigma1=100.0,norm;
  PetscInt       dim,Ii,J,n = 3,use_random;
  
  PetscInitialize(&argc,&args,(char *)0,help);
#if !defined(PETSC_USE_COMPLEX)
  SETERRQ(1,"This example requires complex numbers");
#endif
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(PETSC_NULL, "-display_mat", &disp_mat);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(PETSC_NULL, "-display_vec", &disp_vec);CHKERRQ(ierr);

  ierr = PetscOptionsGetReal(PETSC_NULL,"-sigma1",&sigma1,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRQ(ierr);
  dim  = n*n;

  ierr = MatCreate(PETSC_COMM_SELF,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,dim,dim);CHKERRQ(ierr);
  ierr = MatSetType(A,MATAIJ);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);

  ierr = PetscOptionsHasName(PETSC_NULL,"-norandom",&flg);CHKERRQ(ierr);
  if (flg) use_random = 0;
  else     use_random = 1;
  if (use_random) {
    ierr = PetscRandomCreate(PETSC_COMM_SELF,&rctx);CHKERRQ(ierr);
    ierr = PetscRandomSetFromOptions(rctx);CHKERRQ(ierr);
    ierr = PetscRandomSetInterval(rctx,0.0,PETSC_i);CHKERRQ(ierr); 
    ierr = PetscRandomGetValue(rctx,&sigma2);CHKERRQ(ierr); /* RealPart(sigma2) == 0.0 */
  } else {
    sigma2 = 10.0*PETSC_i;
  }
  h2 = 1.0/((n+1)*(n+1));
  for (Ii=0; Ii<dim; Ii++) { 
    v = -1.0; i = Ii/n; j = Ii - i*n;  
    if (i>0) {
      J = Ii-n; ierr = MatSetValues(A,1,&Ii,1,&J,&v,ADD_VALUES);CHKERRQ(ierr);}
    if (i<n-1) {
      J = Ii+n; ierr = MatSetValues(A,1,&Ii,1,&J,&v,ADD_VALUES);CHKERRQ(ierr);}
    if (j>0) {
      J = Ii-1; ierr = MatSetValues(A,1,&Ii,1,&J,&v,ADD_VALUES);CHKERRQ(ierr);}
    if (j<n-1) {
      J = Ii+1; ierr = MatSetValues(A,1,&Ii,1,&J,&v,ADD_VALUES);CHKERRQ(ierr);}
    v = 4.0 - sigma1*h2; 
    ierr = MatSetValues(A,1,&Ii,1,&Ii,&v,ADD_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* Check whether A is symmetric */
  ierr = PetscOptionsHasName(PETSC_NULL, "-check_symmetric", &flg);CHKERRQ(ierr);
  if (flg) {
    Mat Trans;
    ierr = MatTranspose(A,MAT_INITIAL_MATRIX, &Trans);
    ierr = MatEqual(A, Trans, &flg);
    if (!flg) SETERRQ(PETSC_ERR_USER,"A is not symmetric");
    ierr = MatDestroy(Trans);CHKERRQ(ierr);
  } 
  ierr = MatSetOption(A,MAT_SYMMETRIC,PETSC_TRUE);CHKERRQ(ierr);

  /* make A complex Hermitian */
  Ii = 0; J = 1;
  v = sigma2*h2; /* RealPart(v) = 0.0 */
  ierr = MatSetValues(A,1,&Ii,1,&J,&v,ADD_VALUES);CHKERRQ(ierr);
  v = -sigma2*h2;
  ierr = MatSetValues(A,1,&J,1,&Ii,&v,ADD_VALUES);CHKERRQ(ierr);

  Ii = dim-2; J = dim-1;
  v = sigma2*h2; /* RealPart(v) = 0.0 */
  ierr = MatSetValues(A,1,&Ii,1,&J,&v,ADD_VALUES);CHKERRQ(ierr);
  v = -sigma2*h2;
  ierr = MatSetValues(A,1,&J,1,&Ii,&v,ADD_VALUES);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* Check whether A is Hermitian */
  ierr = PetscOptionsHasName(PETSC_NULL, "-check_Hermitian", &flg);CHKERRQ(ierr);
  if (flg) {
    Mat Hermit;
    if (disp_mat){
      printf(" A:\n");
      ierr = MatView(A,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    }
    ierr = MatHermitianTranspose(A,MAT_INITIAL_MATRIX, &Hermit);
    if (disp_mat){
      printf(" A_Hermitian:\n");
      ierr = MatView(Hermit,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    }
    ierr = MatEqual(A, Hermit, &flg);
    if (!flg) SETERRQ(PETSC_ERR_USER,"A is not Hermitian");
    ierr = MatDestroy(Hermit);CHKERRQ(ierr);
  }
  ierr = MatSetOption(A,MAT_HERMITIAN,PETSC_TRUE);CHKERRQ(ierr);
  
  /* Create a Hermitian matrix As in sbaij format */
  ierr = MatConvert(A,MATSBAIJ,MAT_INITIAL_MATRIX,&As);CHKERRQ(ierr); 
  if (disp_mat){
    printf(" As:\n");
    ierr = MatView(As,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  ierr = MatGetLocalSize(A,&m,&n);CHKERRQ(ierr);
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,n,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  if (use_random){
    ierr = VecSetRandom(x,rctx);CHKERRQ(ierr);
  } else {
    ierr = VecSet(x,1.0);CHKERRQ(ierr);
  }

  /* Create vectors */
  ierr = VecCreate(PETSC_COMM_WORLD,&y);CHKERRQ(ierr);
  ierr = VecSetSizes(y,m,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = VecSetFromOptions(y);CHKERRQ(ierr);
  ierr = VecDuplicate(y,&ys);CHKERRQ(ierr);

  /* Test MatMult */
  ierr = MatMult(A,x,y);CHKERRQ(ierr);
  ierr = MatMult(As,x,ys);CHKERRQ(ierr); /* crash with option '-n 1000' ??? */
  /* ierr = MatMult_SeqSBAIJ_1_Hermitian_tmp(As,x,ys);CHKERRQ(ierr); */
  if (disp_mat){
    printf("y = A*x:\n");
    ierr = VecView(y,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    printf("ys = As*x:\n");
    ierr = VecView(ys,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }
  ierr = VecAXPY(y,-1.0,ys);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_INFINITY,&norm);CHKERRQ(ierr);
  if (norm > 1.e-12){
    printf("|| A*x - As*x || = %G\n",norm);
  }

  /* Free spaces */
  if (use_random) {ierr = PetscRandomDestroy(rctx);CHKERRQ(ierr);}
  ierr = MatDestroy(A);CHKERRQ(ierr);
  ierr = MatDestroy(As);CHKERRQ(ierr);
  
  ierr = VecDestroy(x);CHKERRQ(ierr);
  ierr = VecDestroy(y);CHKERRQ(ierr);
  ierr = VecDestroy(ys);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
/* -----------------------------------*/
#include "../src/mat/impls/sbaij/seq/sbaij.h"
#undef __FUNCT__  
#define __FUNCT__ "MatMult_SeqSBAIJ_1_Hermitian_tmp"
PetscErrorCode MatMult_SeqSBAIJ_1_Hermitian_tmp(Mat A,Vec xx,Vec zz)
{
  Mat_SeqSBAIJ         *a = (Mat_SeqSBAIJ*)A->data;
  const PetscScalar    *x;
  PetscScalar          *z,x1,sum;
  const MatScalar      *v;
  MatScalar            vj;
  PetscErrorCode       ierr;
  PetscInt             mbs=a->mbs,i,j,nz;
  const PetscInt       *ai=a->i;
#if defined(USESHORT)
  const unsigned short *ib=a->jshort;
  unsigned short       ibt;
#else
  const PetscInt       *ib=a->j;
  PetscInt             ibt;
#endif

  PetscFunctionBegin;
  ierr = VecSet(zz,0.0);CHKERRQ(ierr);
  ierr = VecGetArray(xx,(PetscScalar**)&x);CHKERRQ(ierr);
  ierr = VecGetArray(zz,&z);CHKERRQ(ierr);

  v  = a->a; 
  for (i=0; i<mbs; i++) {
    nz   = ai[i+1] - ai[i];  /* length of i_th row of A */    
    x1   = x[i];
    sum  = v[0]*x1;          /* diagonal term */
    for (j=1; j<nz; j++) {
      ibt  = ib[j];
      vj   = v[j];
      sum += vj * x[ibt];   /* (strict upper triangular part of A)*x  */
     
      vj = PetscConj(v[j]);
      z[ibt] += vj * x1;    /* (strict lower triangular part of A)*x  */
    }
    z[i] += sum;
    v    += nz;
    ib   += nz;
  }

  ierr = VecRestoreArray(xx,(PetscScalar**)&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(zz,&z);CHKERRQ(ierr);
  ierr = PetscLogFlops(2.0*(2.0*a->nz - mbs) - mbs);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}
