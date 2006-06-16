#define PETSCMAT_DLL

/*
  Defines matrix-matrix product routines for pairs of MPIAIJ matrices
          C = A * B
*/
#include "src/mat/impls/aij/seq/aij.h" /*I "petscmat.h" I*/
#include "src/mat/utils/freespace.h"
#include "src/mat/impls/aij/mpi/mpiaij.h"
#include "petscbt.h"

#undef __FUNCT__
#define __FUNCT__ "MatMatMult_MPIAIJ_MPIAIJ"
PetscErrorCode MatMatMult_MPIAIJ_MPIAIJ(Mat A,Mat B,MatReuse scall,PetscReal fill, Mat *C) 
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX){ 
    ierr = MatMatMultSymbolic_MPIAIJ_MPIAIJ(A,B,fill,C);CHKERRQ(ierr);/* numeric product is computed as well */
  } else if (scall == MAT_REUSE_MATRIX){
    ierr = MatMatMultNumeric_MPIAIJ_MPIAIJ(A,B,*C);CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_ERR_ARG_WRONG,"Invalid MatReuse %d",(int)scall);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscObjectContainerDestroy_Mat_MatMatMultMPI"
PetscErrorCode PetscObjectContainerDestroy_Mat_MatMatMultMPI(void *ptr)
{
  PetscErrorCode       ierr;
  Mat_MatMatMultMPI    *mult=(Mat_MatMatMultMPI*)ptr;

  PetscFunctionBegin;
  ierr = PetscFree(mult->startsj);CHKERRQ(ierr);
  ierr = PetscFree(mult->bufa);CHKERRQ(ierr);
  if (mult->isrowa){ierr = ISDestroy(mult->isrowa);CHKERRQ(ierr);}
  if (mult->isrowb){ierr = ISDestroy(mult->isrowb);CHKERRQ(ierr);}
  if (mult->iscolb){ierr = ISDestroy(mult->iscolb);CHKERRQ(ierr);}
  if (mult->C_seq){ierr = MatDestroy(mult->C_seq);CHKERRQ(ierr);} 
  if (mult->A_loc){ierr = MatDestroy(mult->A_loc);CHKERRQ(ierr); }
  if (mult->B_seq){ierr = MatDestroy(mult->B_seq);CHKERRQ(ierr);}
  if (mult->B_loc){ierr = MatDestroy(mult->B_loc);CHKERRQ(ierr);}
  if (mult->B_oth){ierr = MatDestroy(mult->B_oth);CHKERRQ(ierr);}
  ierr = PetscFree(mult->abi);CHKERRQ(ierr);
  ierr = PetscFree(mult->abj);CHKERRQ(ierr);
  ierr = PetscFree(mult);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}

EXTERN PetscErrorCode MatDestroy_AIJ(Mat);
#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_MPIAIJ_MatMatMult"
PetscErrorCode MatDestroy_MPIAIJ_MatMatMult(Mat A)
{
  PetscErrorCode       ierr;
  PetscObjectContainer container;
  Mat_MatMatMultMPI    *mult=PETSC_NULL;

  PetscFunctionBegin;
  ierr = PetscObjectQuery((PetscObject)A,"Mat_MatMatMultMPI",(PetscObject *)&container);CHKERRQ(ierr);
  if (container) {
    ierr = PetscObjectContainerGetPointer(container,(void **)&mult);CHKERRQ(ierr);
  } else {
    SETERRQ(PETSC_ERR_PLIB,"Container does not exit");
  }
  A->ops->destroy = mult->MatDestroy;
  ierr = PetscObjectCompose((PetscObject)A,"Mat_MatMatMultMPI",0);CHKERRQ(ierr);
  ierr = (*A->ops->destroy)(A);CHKERRQ(ierr);
  ierr = PetscObjectContainerDestroy(container);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_MPIAIJ_MatMatMult"
PetscErrorCode MatDuplicate_MPIAIJ_MatMatMult(Mat A, MatDuplicateOption op, Mat *M) {
  PetscErrorCode       ierr;
  Mat_MatMatMultMPI    *mult; 
  PetscObjectContainer container;

  PetscFunctionBegin;
  ierr = PetscObjectQuery((PetscObject)A,"Mat_MatMatMultMPI",(PetscObject *)&container);CHKERRQ(ierr);
  if (container) {
    ierr  = PetscObjectContainerGetPointer(container,(void **)&mult);CHKERRQ(ierr); 
  } else {
    SETERRQ(PETSC_ERR_PLIB,"Container does not exit");
  }
  ierr = (*mult->MatDuplicate)(A,op,M);CHKERRQ(ierr);
  (*M)->ops->destroy   = mult->MatDestroy;   /* =MatDestroy_MPIAIJ, *M doesn't duplicate A's container! */
  (*M)->ops->duplicate = mult->MatDuplicate; /* =MatDuplicate_ MPIAIJ */
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatMatMultSymbolic_MPIAIJ_MPIAIJ"
PetscErrorCode MatMatMultSymbolic_MPIAIJ_MPIAIJ(Mat A,Mat B,PetscReal fill,Mat *C)
{
  PetscErrorCode       ierr;
  PetscInt             start,end;
  Mat_MatMatMultMPI    *mult;
  PetscObjectContainer container;
 
  PetscFunctionBegin;
  if (A->cmap.rstart != B->rmap.rstart || A->cmap.rend != B->rmap.rend){
    SETERRQ4(PETSC_ERR_ARG_SIZ,"Matrix local dimensions are incompatible, (%D, %D) != (%D,%D)",A->cmap.rstart,A->cmap.rend,B->rmap.rstart,B->rmap.rend);
  }
  ierr = PetscNew(Mat_MatMatMultMPI,&mult);CHKERRQ(ierr);

  /* create a seq matrix B_seq = submatrix of B by taking rows of B that equal to nonzero col of A */
  ierr = MatGetBrowsOfAcols(A,B,MAT_INITIAL_MATRIX,&mult->isrowb,&mult->iscolb,&mult->brstart,&mult->B_seq);CHKERRQ(ierr);

  /*  create a seq matrix A_seq = submatrix of A by taking all local rows of A */
  start = A->rmap.rstart; end = A->rmap.rend;
  ierr = ISCreateStride(PETSC_COMM_SELF,end-start,start,1,&mult->isrowa);CHKERRQ(ierr); 
  ierr = MatGetLocalMatCondensed(A,MAT_INITIAL_MATRIX,&mult->isrowa,&mult->isrowb,&mult->A_loc);CHKERRQ(ierr); 

  /* compute C_seq = A_seq * B_seq */
  ierr = MatMatMult_SeqAIJ_SeqAIJ(mult->A_loc,mult->B_seq,MAT_INITIAL_MATRIX,fill,&mult->C_seq);CHKERRQ(ierr);

  /* create mpi matrix C by concatinating C_seq */
  ierr = PetscObjectReference((PetscObject)mult->C_seq);CHKERRQ(ierr); /* prevent C_seq being destroyed by MatMerge() */
  ierr = MatMerge(A->comm,mult->C_seq,B->cmap.n,MAT_INITIAL_MATRIX,C);CHKERRQ(ierr); 
 
  /* attach the supporting struct to C for reuse of symbolic C */
  ierr = PetscObjectContainerCreate(PETSC_COMM_SELF,&container);CHKERRQ(ierr);
  ierr = PetscObjectContainerSetPointer(container,mult);CHKERRQ(ierr);
  ierr = PetscObjectCompose((PetscObject)(*C),"Mat_MatMatMultMPI",(PetscObject)container);CHKERRQ(ierr);
  ierr = PetscObjectContainerSetUserDestroy(container,PetscObjectContainerDestroy_Mat_MatMatMultMPI);CHKERRQ(ierr);
  mult->MatDestroy   = (*C)->ops->destroy;
  mult->MatDuplicate = (*C)->ops->duplicate;

  (*C)->ops->destroy   = MatDestroy_MPIAIJ_MatMatMult; 
  (*C)->ops->duplicate = MatDuplicate_MPIAIJ_MatMatMult;
  PetscFunctionReturn(0);
}

/* This routine is called ONLY in the case of reusing previously computed symbolic C */
#undef __FUNCT__  
#define __FUNCT__ "MatMatMultNumeric_MPIAIJ_MPIAIJ"
PetscErrorCode MatMatMultNumeric_MPIAIJ_MPIAIJ(Mat A,Mat B,Mat C)
{
  PetscErrorCode       ierr;
  Mat                  *seq;
  Mat_MatMatMultMPI    *mult; 
  PetscObjectContainer container;

  PetscFunctionBegin;
  ierr = PetscObjectQuery((PetscObject)C,"Mat_MatMatMultMPI",(PetscObject *)&container);CHKERRQ(ierr);
  if (container) {
    ierr  = PetscObjectContainerGetPointer(container,(void **)&mult);CHKERRQ(ierr); 
  } else {
    SETERRQ(PETSC_ERR_PLIB,"Container does not exit");
  }

  seq = &mult->B_seq;
  ierr = MatGetSubMatrices(B,1,&mult->isrowb,&mult->iscolb,MAT_REUSE_MATRIX,&seq);CHKERRQ(ierr);
  mult->B_seq = *seq;
  
  seq = &mult->A_loc;
  ierr = MatGetSubMatrices(A,1,&mult->isrowa,&mult->isrowb,MAT_REUSE_MATRIX,&seq);CHKERRQ(ierr);
  mult->A_loc = *seq;

  ierr = MatMatMult_SeqAIJ_SeqAIJ(mult->A_loc,mult->B_seq,MAT_REUSE_MATRIX,0.0,&mult->C_seq);CHKERRQ(ierr);

  ierr = PetscObjectReference((PetscObject)mult->C_seq);CHKERRQ(ierr); 
  ierr = MatMerge(A->comm,mult->C_seq,B->cmap.n,MAT_REUSE_MATRIX,&C);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatMult_MPIAIJ_MPIDense"
PetscErrorCode MatMatMult_MPIAIJ_MPIDense(Mat A,Mat B,MatReuse scall,PetscReal fill,Mat *C) 
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX){
    ierr = MatMatMultSymbolic_MPIAIJ_MPIDense(A,B,fill,C);CHKERRQ(ierr);
  }  
  ierr = MatMatMultNumeric_MPIAIJ_MPIDense(A,B,*C);CHKERRQ(ierr);  
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatMatMultSymbolic_MPIDense_MPIDense"
PetscErrorCode MatMatMultSymbolic_MPIDense_MPIDense(Mat A,Mat B,PetscReal fill,Mat *C) 
{
  PetscErrorCode ierr;
  PetscInt       m=A->rmap.n,n=B->cmap.n;
  Mat            Cmat;

  PetscFunctionBegin;
  if (A->cmap.n != B->rmap.n) SETERRQ2(PETSC_ERR_ARG_SIZ,"A->cmap.n %d != B->rmap.n %d\n",A->cmap.n,B->rmap.n);
  ierr = MatCreate(B->comm,&Cmat);CHKERRQ(ierr);
  ierr = MatSetSizes(Cmat,m,n,A->rmap.N,B->cmap.N);CHKERRQ(ierr);
  ierr = MatSetType(Cmat,MATMPIDENSE);CHKERRQ(ierr);
  ierr = MatMPIDenseSetPreallocation(Cmat,PETSC_NULL);CHKERRQ(ierr);
  Cmat->assembled = PETSC_TRUE;
  *C = Cmat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatMultSymbolic_MPIAIJ_MPIDense"
PetscErrorCode MatMatMultSymbolic_MPIAIJ_MPIDense(Mat A,Mat B,PetscReal fill,Mat *C) 
{
  PetscErrorCode ierr;

  PetscFunctionBegin; 
  ierr = MatMatMultSymbolic_MPIDense_MPIDense(A,B,0.0,C);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatMatMultNumeric_MPIAIJ_MPIDense"
PetscErrorCode MatMatMultNumeric_MPIAIJ_MPIDense(Mat A,Mat B,Mat C)
{
  PetscErrorCode ierr;
  PetscScalar    *b,*c,*barray,*carray;
  PetscInt       cm=C->rmap.n, bN=B->cmap.N, bm=B->rmap.n, col;
  Vec            vb,vc;

  PetscFunctionBegin;
  //PetscMPIInt rank;
  //ierr = MPI_Comm_rank(A->comm,&rank);CHKERRQ(ierr);
  if (!C->rmap.N || !bN) PetscFunctionReturn(0);
  ierr = VecCreateMPI(B->comm,bm,PETSC_DECIDE,&vb);CHKERRQ(ierr);
  ierr = VecCreateMPI(A->comm,cm,PETSC_DECIDE,&vc);CHKERRQ(ierr);

  ierr = MatGetArray(B,&barray);CHKERRQ(ierr);
  ierr = MatGetArray(C,&carray);CHKERRQ(ierr);
  b = barray; c = carray;
  for (col=0; col<bN; col++){
    ierr = VecPlaceArray(vb,b);CHKERRQ(ierr);
    ierr = VecPlaceArray(vc,c);CHKERRQ(ierr);
    //if (!rank) printf(" col %d, vb:\n",col);
    //ierr = VecView(vb,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    ierr = MatMult(A,vb,vc);CHKERRQ(ierr);
    //if (!rank) printf(" col %d, vc:\n",col);
    //ierr = VecView(vc,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    b += bm; c += cm;
    ierr = VecResetArray(vb);CHKERRQ(ierr);
    ierr = VecResetArray(vc);CHKERRQ(ierr);
  }
  ierr = MatRestoreArray(B,&barray);CHKERRQ(ierr);
  ierr = MatRestoreArray(C,&carray);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = VecDestroy(vb);CHKERRQ(ierr);
  ierr = VecDestroy(vc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

