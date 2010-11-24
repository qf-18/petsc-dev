/*
    Index sets for scatter-gather type operations in vectors
and matrices. 

*/

#if !defined(_IS_H)
#define _IS_H

#include "petscis.h"

struct _ISOps {
  PetscErrorCode (*getsize)(IS,PetscInt*);
  PetscErrorCode (*getlocalsize)(IS,PetscInt*);
  PetscErrorCode (*getindices)(IS,const PetscInt*[]);
  PetscErrorCode (*restoreindices)(IS,const PetscInt*[]);
  PetscErrorCode (*invertpermutation)(IS,PetscInt,IS*);
  PetscErrorCode (*sort)(IS);
  PetscErrorCode (*sorted)(IS,PetscBool*);
  PetscErrorCode (*duplicate)(IS,IS*);
  PetscErrorCode (*destroy)(IS);
  PetscErrorCode (*view)(IS,PetscViewer);
  PetscErrorCode (*identity)(IS,PetscBool*);
  PetscErrorCode (*copy)(IS,IS);
  PetscErrorCode (*togeneral)(IS);
  PetscErrorCode (*oncomm)(IS,MPI_Comm,PetscCopyMode,IS*);
  PetscErrorCode (*setblocksize)(IS,PetscInt);
  PetscErrorCode (*contiguous)(IS);
};

/* Currently only used in this implementation, but more general in principle */
typedef enum {PETSC_TERNARY_FALSE = -1,PETSC_TERNARY_UNKNOWN = 0,PETSC_TERNARY_TRUE = 1} PetscTernary;

struct _p_IS {
  PETSCHEADER(struct _ISOps);
  PetscBool    isperm;          /* if is a permutation */
  PetscInt     max,min;         /* range of possible values */
  PetscInt     bs;              /* block size */
  void         *data;
  PetscBool    isidentity;
  PetscTernary contiguous;
  PetscInt     *total, *nonlocal;   /* local representation of ALL indices across the comm as well as the nonlocal part. */
  PetscInt     local_offset;        /* offset to the local part within the total index set */
  IS           complement;          /* IS wrapping nonlocal indices. */
};


#endif
