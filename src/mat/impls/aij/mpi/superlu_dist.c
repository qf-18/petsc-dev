/*$Id: superlu_DIST.c,v 1.10 2001/08/15 15:56:50 bsmith Exp $*/
/* 
        Provides an interface to the SuperLU_DIST sparse solver
        Usage:
             mpirun -np <procs> main -mat_aij_superlu_dist -r <proc rows> -c <proc columns>
          or
             mpirun -np <procs> main -mat_aij_superlu_dist (use the default process grid)

          Command line options:
                  -mat_aij_superlu_dist_equil <YES/NO>
                  -mat_aij_superlu_dist_rowperm <NATURAL/LargeDiag>
                  -mat_aij_superlu_dist_colperm <NATURAL/COLAMD/MMD_ATA/MMD_AT_PLUS_A>
                  -mat_aij_superlu_dist_replacetinypivot <YES/NO>
                  -mat_aij_superlu_dist_iterrefine <NO/DOUBLE>

          SuperLU_DIST default options: 
               equil:              YES
               rowperm:            LargeDiag
               colperm:            MMD_AT_PLUS_A
               replacetinypivot:   YES
               iterrefine:         NO
*/

#include "src/mat/impls/aij/seq/aij.h"
#include "src/mat/impls/aij/mpi/mpiaij.h"

#if defined(PETSC_HAVE_SUPERLUDIST) && !defined(PETSC_USE_SINGLE) && !defined(PETSC_USE_COMPLEX)

EXTERN_C_BEGIN
#include "superlu_ddefs.h"
EXTERN_C_END


typedef struct {
  gridinfo_t              grid;
  superlu_options_t       options;
  SuperMatrix             A_sup;
  ScalePermstruct_t       ScalePermstruct;
  LUstruct_t              LUstruct;
  int                     size;    /* size of the MPI communicator */
} Mat_MPIAIJ_SuperLU_DIST;

extern int MatDestroy_MPIAIJ(Mat);
extern int MatDestroy_SeqAIJ(Mat);

/* SuperLU function: Convert a row compressed storage into a column compressed storage. */
#undef __FUNCT__  
#define __FUNCT__ "dCompRow_to_CompCol"
void dCompRow_to_CompCol(int m, int n, int nnz,
                    double *a, int *colind, int *rowptr,
                    double **at, int **rowind, int **colptr)
{
    register int i, j, col, relpos;
    int *marker;

    /* Allocate storage for another copy of the matrix. */
    *at = (double *) doubleMalloc(nnz);
    *rowind = (int *) intMalloc(nnz);
    *colptr = (int *) intMalloc(n+1);
    marker = (int *) intCalloc(n);

    /* Get counts of each column of A, and set up column pointers */
    for (i = 0; i < m; ++i)
        for (j = rowptr[i]; j < rowptr[i+1]; ++j) ++marker[colind[j]];
    (*colptr)[0] = 0;
    for (j = 0; j < n; ++j) {
        (*colptr)[j+1] = (*colptr)[j] + marker[j];
        marker[j] = (*colptr)[j];
    }

    /* Transfer the matrix into the compressed column storage. */
    for (i = 0; i < m; ++i) {
        for (j = rowptr[i]; j < rowptr[i+1]; ++j) {
            col = colind[j];
            relpos = marker[col];
            (*rowind)[relpos] = i;
            (*at)[relpos] = a[j];
            ++marker[col];
        }
    }

    SUPERLU_FREE(marker);
}

#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_MPIAIJ_SuperLU_DIST"
extern int MatDestroy_MPIAIJ_SuperLU_DIST(Mat A)
{
  Mat_MPIAIJ         *a  = (Mat_MPIAIJ*)A->data; 
  Mat_MPIAIJ_SuperLU_DIST *lu = (Mat_MPIAIJ_SuperLU_DIST*)a->spptr; 
  int                ierr,size=lu->size;

  PetscFunctionBegin;
  /* PetscPrintf(PETSC_COMM_SELF, "MatDestroy is called\n"); */
 
  /* Deallocate SuperLU_DIST storage */
  Destroy_CompCol_Matrix(&lu->A_sup);
  Destroy_LU(A->N, &lu->grid, &lu->LUstruct);
  ScalePermstructFree(&lu->ScalePermstruct);
  LUstructFree(&lu->LUstruct);

  /* Release the SuperLU_DIST process grid. */
  superlu_gridexit(&lu->grid);

  ierr = PetscFree(lu);CHKERRQ(ierr); 
  
  if (size == 1){
    ierr = MatDestroy_SeqAIJ(A);CHKERRQ(ierr);
  } else {
    ierr = MatDestroy_MPIAIJ(A);CHKERRQ(ierr);
  } 
  
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_MPIAIJ_SuperLU_DIST"
extern int MatSolve_MPIAIJ_SuperLU_DIST(Mat A,Vec b_mpi,Vec x)
{
  Mat_MPIAIJ              *aa = (Mat_MPIAIJ*)A->data;
  Mat_MPIAIJ_SuperLU_DIST *lu = (Mat_MPIAIJ_SuperLU_DIST*)aa->spptr;
  int                     ierr, size=lu->size;
  int_t                   m=A->M, N=A->N; 
  superlu_options_t       options=lu->options;
  SuperLUStat_t           stat;
  double                  berr[1],*bptr;  
  int                     info, nrhs=1;
  Vec                     x_seq;
  IS                      iden;
  VecScatter              scat;

  PetscFunctionBegin;
  /* PetscPrintf(PETSC_COMM_SELF, "Solve is called ..., size: %d\n",size); */

  if (size > 1) {  /* convert mpi vector b to seq vector x_seq */
    VecCreateSeq(PETSC_COMM_SELF,N,&x_seq);
    ierr = ISCreateStride(PETSC_COMM_SELF,N,0,1,&iden);CHKERRQ(ierr);
    VecScatterCreate(b_mpi,iden,x_seq,iden,&scat);
    ierr = ISDestroy(iden);CHKERRQ(ierr);

    VecScatterBegin(b_mpi,x_seq,INSERT_VALUES,SCATTER_FORWARD,scat);
    VecScatterEnd(b_mpi,x_seq,INSERT_VALUES,SCATTER_FORWARD,scat);
    ierr = VecGetArray(x_seq,&bptr);CHKERRQ(ierr); 
  } else {
    ierr = VecCopy(b_mpi,x);
    ierr = VecGetArray(x,&bptr);CHKERRQ(ierr); 
  }
 
  options.Fact = FACTORED; /* The factored form of A is supplied. Local option used by this func. only.*/
  
  PStatInit(&stat);        /* Initialize the statistics variables. */
  pdgssvx_ABglobal(&options, &lu->A_sup, &lu->ScalePermstruct, bptr, m, nrhs, 
                   &lu->grid, &lu->LUstruct, berr, &stat, &info);
  PStatPrint(&stat, &lu->grid);     /* Print the statistics. */
  PStatFree(&stat);
    
  if (size > 1) {    /* convert seq x to mpi x */
    ierr = VecRestoreArray(x_seq,&bptr);CHKERRQ(ierr);
    VecScatterBegin(x_seq,x,INSERT_VALUES,SCATTER_REVERSE,scat);
    VecScatterEnd(x_seq,x,INSERT_VALUES,SCATTER_REVERSE,scat);
    ierr = VecScatterDestroy(scat);CHKERRQ(ierr);
    ierr = VecDestroy(x_seq);CHKERRQ(ierr);
  } else {
    ierr = VecRestoreArray(x,&bptr);CHKERRQ(ierr); 
  }

  PetscFunctionReturn(0);
}


#undef __FUNCT__   
#define __FUNCT__ "MatLUFactorNumeric_MPIAIJ_SuperLU_DIST"
extern int MatLUFactorNumeric_MPIAIJ_SuperLU_DIST(Mat A,Mat *F)
{
  Mat_MPIAIJ              *fac = (Mat_MPIAIJ*)(*F)->data;
  Mat                     *tseq,A_seq;
  Mat_SeqAIJ              *aa;
  Mat_MPIAIJ_SuperLU_DIST *lu = (Mat_MPIAIJ_SuperLU_DIST*)fac->spptr;
  int                     M=A->M,N=A->N,info,ierr,size=lu->size;
  SuperLUStat_t           stat;
  double                  *berr=0, *bptr=0;
  int_t                   *asub, *xa;
  double                  *a; 
  SuperMatrix             A_sup;
  IS                      isrow,iscol;

  PetscFunctionBegin;
  /* PetscPrintf(PETSC_COMM_SELF, "NumericFac is called, %d, M: %d, N: %d\n", lu->options.Fact,M,N); */

  if (size > 1) { /* convert mpi A to seq mat A */
    ierr = ISCreateStride(PETSC_COMM_SELF,M,0,1,&isrow); CHKERRQ(ierr);
    ierr = ISCreateStride(PETSC_COMM_SELF,N,0,1,&iscol); CHKERRQ(ierr);

    ierr = MatGetSubMatrices(A,1,&isrow,&iscol,MAT_INITIAL_MATRIX,&tseq); CHKERRQ(ierr);

    ierr = ISDestroy(isrow);CHKERRQ(ierr);
    ierr = ISDestroy(iscol);CHKERRQ(ierr);
    A_seq = *tseq;
    ierr = PetscFree(tseq);CHKERRQ(ierr);
 
    aa =  (Mat_SeqAIJ*)A_seq->data;
  } else {
    aa =  (Mat_SeqAIJ*)A->data;
  }

  /* Allocate storage for compressed column representation. */
  dallocateA(N, aa->nz, &a, &asub, &xa);
  
  /* Convert Petsc NR matrix storage to SuperLU_DIST NC storage */
  dCompRow_to_CompCol(M,N,aa->nz,aa->a,aa->j,aa->i,&a, &asub, &xa);

  /* Create compressed column matrix A_sup. */
  dCreate_CompCol_Matrix(&A_sup, M, N, aa->nz, a, asub, xa, NC, _D, GE);  

  /* Factor the matrix. */
  PStatInit(&stat);                /* Initialize the statistics variables. */
  pdgssvx_ABglobal(&lu->options, &A_sup, &lu->ScalePermstruct, bptr, M, 0, 
                   &lu->grid, &lu->LUstruct, berr, &stat, &info);  
  PStatPrint(&stat, &lu->grid);        /* Print the statistics. */
  PStatFree(&stat);  

  lu->A_sup  = A_sup;
  lu->options.Fact = SamePattern; /* Sparsity pattern of A and perm_c can be reused. */
  if (size > 1){
    ierr = MatDestroy(A_seq);CHKERRQ(ierr);
  }
 
  PetscFunctionReturn(0);
}

/* Note the Petsc r and c permutations are ignored */
#undef __FUNCT__  
#define __FUNCT__ "MatLUFactorSymbolic_MPIAIJ_SuperLU_DIST"

extern int MatLUFactorSymbolic_MPIAIJ_SuperLU_DIST(Mat A,IS r,IS c,MatLUInfo *info,Mat *F)
{
  Mat_MPIAIJ              *fac;
  Mat_MPIAIJ_SuperLU_DIST *lu;   /* ptr to Mat_MPIAIJ_SuperLU_DIST */
  int                     ierr,M=A->M,N=A->N,size;
  int_t                   nprow, npcol;
  gridinfo_t              grid; 
  superlu_options_t       options;
  ScalePermstruct_t       ScalePermstruct;
  LUstruct_t              LUstruct;
  char                    Equil[256],RowPerm[256],ColPerm[256],ReplaceTinyPivot[256],IterRefine[256];
  PetscTruth              flg,flg1;

  PetscFunctionBegin;	
  /* PetscPrintf(PETSC_COMM_WORLD, "SymbolicFac is called, M: %d, N: %d\n",M,N); */

  /* Initialize the SuperLU process grid. */
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  nprow = size/2;               /* Default process rows.      */
  if (nprow == 0) nprow = 1;
  npcol = size/nprow;           /* Default process columns.   */
  
  ierr = PetscOptionsGetInt(PETSC_NULL,"-r",&nprow,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-c",&npcol,PETSC_NULL);CHKERRQ(ierr);
 
  if ( size != nprow * npcol ) 
    SETERRQ(1,"Number of processes should be equal to nprow*npcol");
  
  superlu_gridinit(MPI_COMM_WORLD, nprow, npcol, &grid);

  /* Create the factorization matrix F */ 
  ierr = MatCreateMPIAIJ(A->comm,PETSC_DECIDE,PETSC_DECIDE,M,N,0,PETSC_NULL,0,PETSC_NULL,F);
  CHKERRQ(ierr);

  (*F)->ops->lufactornumeric  = MatLUFactorNumeric_MPIAIJ_SuperLU_DIST;
  (*F)->ops->solve            = MatSolve_MPIAIJ_SuperLU_DIST;
  (*F)->ops->destroy          = MatDestroy_MPIAIJ_SuperLU_DIST;  
  (*F)->factor                = FACTOR_LU;  
  fac                         = (Mat_MPIAIJ*)(*F)->data; 

  ierr             = PetscNew(Mat_MPIAIJ_SuperLU_DIST,&lu);CHKERRQ(ierr); 
  fac->spptr       = (void*)lu;

  /* Set the input options */
  set_default_options(&options);
  options.IterRefine = NOREFINE;

  ierr = PetscOptionsGetString(PETSC_NULL,"--mat_aij_superlu_dist_equil",Equil,256,&flg); 
  if (flg) {
    ierr = PetscStrcmp(Equil,"NO",&flg1);CHKERRQ(ierr);
    if (flg1) options.Equil = NO;
  }

  ierr = PetscOptionsGetString(PETSC_NULL,"-mat_aij_superlu_dist_rowperm",RowPerm,256,&flg); 
  if (flg) {
    ierr = PetscStrcmp(RowPerm,"NATURAL",&flg1);CHKERRQ(ierr);
    if (flg1) options.RowPerm = NOROWPERM;
  }

  ierr = PetscOptionsGetString(PETSC_NULL,"-mat_aij_superlu_dist_colperm",ColPerm,256,&flg); 
  while (flg) {
    ierr = PetscStrcmp(ColPerm,"NATURAL",&flg1);CHKERRQ(ierr);
    if (flg1) {
      options.ColPerm = NATURAL;
      break;
    }
    ierr = PetscStrcmp(ColPerm,"MMD_ATA",&flg1);CHKERRQ(ierr);
    if (flg1) {
      options.ColPerm = MMD_ATA;
      break;
    }
    ierr = PetscStrcmp(ColPerm,"COLAMD",&flg1);CHKERRQ(ierr);
    if (flg1) {
      options.ColPerm = COLAMD;
      break;
    }
    break;
  }
  /* PetscPrintf(PETSC_COMM_WORLD, "ColPerm: %s \n",ColPerm); */

  ierr = PetscOptionsGetString(PETSC_NULL,"-mat_aij_superlu_dist_replacetinypivot",ReplaceTinyPivot,256,&flg); 
  if (flg) {
    ierr = PetscStrcmp(ReplaceTinyPivot,"NO",&flg1);CHKERRQ(ierr);
    if (flg1) options.ReplaceTinyPivot = NO;
  }

  ierr = PetscOptionsGetString(PETSC_NULL,"-mat_aij_superlu_dist_iterrefine",IterRefine,256,&flg);
  if (flg) {
    ierr = PetscStrcmp(IterRefine,"DOUBLE",&flg1);CHKERRQ(ierr);
    if (flg1) options.IterRefine = DOUBLE;    
  }

  /* Initialize ScalePermstruct and LUstruct. */
  ScalePermstructInit(M, N, &ScalePermstruct);
  LUstructInit(M, N, &LUstruct);

  lu->ScalePermstruct = ScalePermstruct;
  lu->LUstruct        = LUstruct;
  lu->options = options;
  lu->grid    = grid;
  lu->size    = size;

  PetscFunctionReturn(0); 
}

#undef __FUNCT__  
#define __FUNCT__ "MatUseSuperLU_DIST_MPIAIJ"
int MatUseSuperLU_DIST_MPIAIJ(Mat A)
{
  PetscTruth flg1,flg2;
  int        ierr,size;  

  PetscFunctionBegin;
  /* PetscPrintf(PETSC_COMM_SELF, "UseSuperLU_DIST is called\n");   */
  PetscValidHeaderSpecific(A,MAT_COOKIE);  
  ierr = PetscTypeCompare((PetscObject)A,MATMPIAIJ,&flg1);
  ierr = PetscTypeCompare((PetscObject)A,MATSEQAIJ,&flg2);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if ( (size>1 && !flg1 ) || ( size==1 && !flg2 )) 
    SETERRQ(1,"Matrix has wrong format");

  A->ops->lufactorsymbolic = MatLUFactorSymbolic_MPIAIJ_SuperLU_DIST;
  A->ops->lufactornumeric  = MatLUFactorNumeric_MPIAIJ_SuperLU_DIST;

  PetscFunctionReturn(0);
}


#else

#undef __FUNCT__  
#define __FUNCT__ "MatUseSuperLU_DIST_MPIAIJ"
int MatUseSuperLU_DIST_MPIAIJ(Mat A)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#endif


