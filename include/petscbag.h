
#if !defined(__PETSCBAG_H)
#define __PETSCBAG_H
#include "petsc.h"
PETSC_EXTERN_CXX_BEGIN

/*S
     PetscBag - PETSc object that manages a collection of user data including parameters.
           A bag is essentially a C struct with serialization (you can save it and load it from files).

   Level: beginner

    Sample Usage:
$      typedef struct {
$         PetscInt     height;
$         PetscScalar  root;
$         PetscReal    byebye;
$      } MyParameters;
$
$      PetscBag     bag;
$      MyParameters *params;
$      
$      ierr = PetscBagCreate(MyParameters,sizeof(MyParameters),&bag);
$      ierr = PetscBagGetData(bag,(void **)&params);
$      ierr = PetscBagSetName(params,"MyParameters");
$      ierr = PetscBagRegisterInt(params,&params.height,22,"height","Height of the water tower");
$       
$       
$       
$       
$       

.seealso:  PetscBagSetName(), PetscBagGetName(), PetscBagView(), PetscBagLoad(), PetscBagGetData()
           PetscBagRegisterReal(), PetscBagRegisterInt(), PetscBagRegisterTruth(), PetscBagRegisterScalar()
           PetscBagSetFromOptions(), PetscBagRegisterVec(), PetscBagCreate(), PetscBagDestroy(), PetscBagRegisterEnum()
S*/
typedef struct _n_PetscBag* PetscBag;

typedef struct _n_PetscBagItem* PetscBagItem;

extern PetscErrorCode PetscBagCreate(MPI_Comm,size_t,PetscBag*);
extern PetscErrorCode PetscBagDestroy(PetscBag);
extern PetscErrorCode PetscBagGetData(PetscBag,void **);
extern PetscErrorCode PetscBagRegisterReal(PetscBag,void*,PetscReal, const char*, const char*);
extern PetscErrorCode PetscBagRegisterString(PetscBag,void*,PetscInt,const char*, const char*, const char*);
extern PetscErrorCode PetscBagRegisterScalar(PetscBag,void*,PetscScalar,const  char*,const  char*);
extern PetscErrorCode PetscBagRegisterInt(PetscBag,void*,PetscInt,const  char*,const  char*);
extern PetscErrorCode PetscBagRegisterEnum(PetscBag,void*,const  char*[],PetscEnum,const char*,const  char*);
extern PetscErrorCode PetscBagRegisterTruth(PetscBag,void*,PetscTruth,const  char*,const  char*);
extern PetscErrorCode PetscBagRegisterVec(PetscBag,void*,const char*,const  char*);

extern PetscErrorCode PetscBagSetFromOptions(PetscBag);
extern PetscErrorCode PetscBagGetName(PetscBag, char **name);
extern PetscErrorCode PetscBagSetName(PetscBag, const char *, const char *);

extern PetscErrorCode PetscBagView(PetscBag,PetscViewer);
extern PetscErrorCode PetscBagLoad(PetscViewer,PetscBag*);

extern PetscErrorCode PetscBagSetViewer(PetscBag,PetscErrorCode (*)(PetscBag,PetscViewer));
extern PetscErrorCode PetscBagSetLoader(PetscBag,PetscErrorCode (*)(PetscBag,PetscViewer));
extern PetscErrorCode PetscBagSetDestroy(PetscBag,PetscErrorCode (*)(PetscBag));

PETSC_EXTERN_CXX_END
#endif
