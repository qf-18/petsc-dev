#define PETSCMAT_DLL

#include "src/mat/impls/aij/mpi/mpiaij.h"

#undef __FUNCT__  
#define __FUNCT__ "MatCreateMPICSRPERM"
/*@C
   MatCreateMPICSRPERM - Creates a sparse parallel matrix whose local 
   portions are stored as SEQCSRPERM matrices (a matrix class that inherits 
   from SEQAIJ but includes some optimizations to allow more effective 
   vectorization).  The same guidelines that apply to MPIAIJ matrices for 
   preallocating the matrix storage apply here as well.

      Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  m - number of local rows (or PETSC_DECIDE to have calculated if M is given)
           This value should be the same as the local size used in creating the 
           y vector for the matrix-vector product y = Ax.
.  n - This value should be the same as the local size used in creating the 
       x vector for the matrix-vector product y = Ax. (or PETSC_DECIDE to have
       calculated if N is given) For square matrices n is almost always m.
.  M - number of global rows (or PETSC_DETERMINE to have calculated if m is given)
.  N - number of global columns (or PETSC_DETERMINE to have calculated if n is given)
.  d_nz  - number of nonzeros per row in DIAGONAL portion of local submatrix
           (same value is used for all local rows)
.  d_nnz - array containing the number of nonzeros in the various rows of the 
           DIAGONAL portion of the local submatrix (possibly different for each row)
           or PETSC_NULL, if d_nz is used to specify the nonzero structure. 
           The size of this array is equal to the number of local rows, i.e 'm'. 
           You must leave room for the diagonal entry even if it is zero.
.  o_nz  - number of nonzeros per row in the OFF-DIAGONAL portion of local
           submatrix (same value is used for all local rows).
-  o_nnz - array containing the number of nonzeros in the various rows of the
           OFF-DIAGONAL portion of the local submatrix (possibly different for
           each row) or PETSC_NULL, if o_nz is used to specify the nonzero 
           structure. The size of this array is equal to the number 
           of local rows, i.e 'm'. 

   Output Parameter:
.  A - the matrix 

   Notes:
   If the *_nnz parameter is given then the *_nz parameter is ignored

   m,n,M,N parameters specify the size of the matrix, and its partitioning across
   processors, while d_nz,d_nnz,o_nz,o_nnz parameters specify the approximate
   storage requirements for this matrix.

   If PETSC_DECIDE or  PETSC_DETERMINE is used for a particular argument on one 
   processor than it must be used on all processors that share the object for 
   that argument.

   The user MUST specify either the local or global matrix dimensions
   (possibly both).

   The parallel matrix is partitioned such that the first m0 rows belong to 
   process 0, the next m1 rows belong to process 1, the next m2 rows belong 
   to process 2 etc.. where m0,m1,m2... are the input parameter 'm'.

   The DIAGONAL portion of the local submatrix of a processor can be defined 
   as the submatrix which is obtained by extraction the part corresponding 
   to the rows r1-r2 and columns r1-r2 of the global matrix, where r1 is the 
   first row that belongs to the processor, and r2 is the last row belonging 
   to the this processor. This is a square mxm matrix. The remaining portion 
   of the local submatrix (mxN) constitute the OFF-DIAGONAL portion.

   If o_nnz, d_nnz are specified, then o_nz, and d_nz are ignored.

   When calling this routine with a single process communicator, a matrix of
   type SEQCSRPERM is returned.  If a matrix of type MPICSRPERM is desired 
   for this type of communicator, use the construction mechanism:
     MatCreate(...,&A); MatSetType(A,MPIAIJ); MatMPIAIJSetPreallocation(A,...);

   By default, this format uses inodes (identical nodes) when possible.
   We search for consecutive rows with the same nonzero structure, thereby
   reusing matrix information to achieve increased efficiency.

   Options Database Keys:
+  -mat_no_inode  - Do not use inodes
.  -mat_inode_limit <limit> - Sets inode limit (max limit=5)
-  -mat_aij_oneindex - Internally use indexing starting at 1
        rather than 0.  Note that when calling MatSetValues(),
        the user still MUST index entries starting at 0!
@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatCreateMPICSRPERM(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[],Mat *A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,M,N);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) {
    ierr = MatSetType(*A,MATMPICSRPERM);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(*A,d_nz,d_nnz,o_nz,o_nnz);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(*A,MATSEQCSRPERM);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(*A,d_nz,d_nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


/* MatConvert_MPIAIJ_MPICSRPERM() converts an MPIAIJ matrix into a 
 * SeqCSRPERM matrix.  This routine is called by the MatCreate_MPICSRPERM() 
 * routine, but can also be used to convert an assembled MPIAIJ matrix 
 * into a SeqCSRPERM one. */
EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "MatConvert_MPIAIJ_MPICSRPERM"
PetscErrorCode PETSCMAT_DLLEXPORT MatConvert_MPIAIJ_MPICSRPERM(Mat A,MatType type,MatReuse reuse,Mat *newmat)
{
  /* This routine is only called to convert to MATMPICSRPERM
   * from MATMPIAIJ, so we can ignore 'MatType Type'. */
  PetscErrorCode ierr;
  Mat B = *newmat;
  Mat_MPIAIJ *mpimat;
  Mat localmat_A, localmat_B;

  PetscFunctionBegin;
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatDuplicate(A,MAT_COPY_VALUES,&B);CHKERRQ(ierr);
  }
  mpimat = (Mat_MPIAIJ *) B->data;

  /* I believe that all I need to do to convert from MPIAIJ to MPICSRPERM 
   * is to change the typename to MPICSRPERM and convert the local 
   * submatrices from SEQAIJ to SEQCSRPERM. */
  ierr = PetscObjectChangeTypeName( (PetscObject) B, MATMPICSRPERM);CHKERRQ(ierr);
  localmat_A = mpimat->A;
  localmat_B = mpimat->B;
  ierr = MatConvert(localmat_A, MATSEQCSRPERM, MAT_REUSE_MATRIX, &localmat_A);CHKERRQ(ierr);
  ierr = MatConvert(localmat_B, MATSEQCSRPERM, MAT_REUSE_MATRIX, &localmat_B);CHKERRQ(ierr);

  *newmat = B;
  PetscFunctionReturn(0);
}
EXTERN_C_END


EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPICSRPERM"
PetscErrorCode PETSCMAT_DLLEXPORT MatCreate_MPICSRPERM(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Following the example of the SuperLU class, I change the type name 
   * before calling MatSetType() to force proper construction of MPIAIJ 
   * and MATMPICSRPERM types. */
  ierr = PetscObjectChangeTypeName((PetscObject)A,MATMPICSRPERM);CHKERRQ(ierr);
  ierr = MatSetType(A,MATMPIAIJ);CHKERRQ(ierr);
  ierr = MatConvert_MPIAIJ_MPICSRPERM(A,MATMPICSRPERM,MAT_REUSE_MATRIX,&A);
  CHKERRQ(ierr);
  
  PetscFunctionReturn(0);
}
EXTERN_C_END
