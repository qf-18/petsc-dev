/*
  Code for manipulating distributed regular arrays in parallel.
*/

#include "src/dm/da/daimpl.h"    /*I   "petscda.h"   I*/

#undef __FUNCT__  
#define __FUNCT__ "DAGlobalToLocalBegin"
/*@
   DAGlobalToLocalBegin - Maps values from the global vector to the local
   patch; the ghost points are included. Must be followed by 
   DAGlobalToLocalEnd() to complete the exchange.

   Collective on DA

   Input Parameters:
+  da - the distributed array context
.  g - the global vector
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the local values

   Level: beginner

   Notes:
   The global and local vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateLocalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, begin

.seealso: DAGlobalToLocalEnd(), DALocalToGlobal(), DACreate2d(), 
          DALocalToLocalBegin(), DALocalToLocalEnd(),
          DALocalToGlobalBegin(), DALocalToGlobalEnd()
          

@*/
PetscErrorCode DAGlobalToLocalBegin(DA da,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,4);
  ierr = VecScatterBegin(g,l,mode,SCATTER_FORWARD,da->gtol);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DALocalToGlobalBegin"
/*@
   DALocalToGlobalBegin - Adds values from the local (ghosted) vector
   into the global (nonghosted) vector.

   Collective on DA

   Input Parameters:
+  da - the distributed array context
-  l  - the local values

   Output Parameter:
.  g - the global vector

   Level: beginner

   Notes:
   Use DALocalToGlobal() to discard the ghost point values

   The global and local vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateLocalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, begin

.seealso: DAGlobalToLocalEnd(), DALocalToGlobal(), DACreate2d(), 
          DALocalToLocalBegin(), DALocalToLocalEnd(), DALocalToGlobalEnd()

@*/
PetscErrorCode DALocalToGlobalBegin(DA da,Vec l,Vec g)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,3);
  ierr = VecScatterBegin(l,g,ADD_VALUES,SCATTER_REVERSE,da->gtol);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DALocalToGlobalEnd"
/*@
   DALocalToGlobalEnd - Adds values from the local (ghosted) vector
   into the global (nonghosted) vector.

   Collective on DA

   Input Parameters:
+  da - the distributed array context
-  l  - the local values

   Output Parameter:
.  g - the global vector

   Level: beginner

   Notes:
   Use DALocalToGlobal() to discard the ghost point values

   The global and local vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateLocalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, begin

.seealso: DAGlobalToLocalEnd(), DALocalToGlobal(), DACreate2d(), 
          DALocalToLocalBegin(), DALocalToLocalEnd(), DALocalToGlobalBegin()

@*/
PetscErrorCode DALocalToGlobalEnd(DA da,Vec l,Vec g)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,3);
  ierr = VecScatterEnd(l,g,ADD_VALUES,SCATTER_REVERSE,da->gtol);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DAGlobalToLocalEnd"
/*@
   DAGlobalToLocalEnd - Maps values from the global vector to the local
   patch; the ghost points are included. Must be preceeded by 
   DAGlobalToLocalBegin().

   Collective on DA

   Input Parameters:
+  da - the distributed array context
.  g - the global vector
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the local values

   Level: beginner

   Notes:
   The global and local vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateLocalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, end

.seealso: DAGlobalToLocalBegin(), DALocalToGlobal(), DACreate2d(),
     DALocalToLocalBegin(), DALocalToLocalEnd(), DALocalToGlobalBegin(), DALocalToGlobalEnd()
@*/
PetscErrorCode DAGlobalToLocalEnd(DA da,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,4);
  ierr = VecScatterEnd(g,l,mode,SCATTER_FORWARD,da->gtol);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN PetscErrorCode DAGetNatural_Private(DA,PetscInt*,IS*);
#undef __FUNCT__  
#define __FUNCT__ "DAGlobalToNatural_Create"
/*
   DAGlobalToNatural_Create - Create the global to natural scatter object

   Collective on DA

   Input Parameter:
.  da - the distributed array context

   Level: developer

   Notes: This is an internal routine called by DAGlobalToNatural() to 
     create the scatter context.

.keywords: distributed array, global to local, begin

.seealso: DAGlobalToNaturalEnd(), DALocalToGlobal(), DACreate2d(), 
          DAGlobalToLocalBegin(), DAGlobalToLocalEnd(), DACreateNaturalVector()
*/
PetscErrorCode DAGlobalToNatural_Create(DA da)
{
  PetscErrorCode ierr;
  PetscInt  m,start,Nlocal;
  IS  from,to;
  Vec global;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  if (!da->natural) {
    SETERRQ(PETSC_ERR_ORDER,"Natural layout vector not yet created; cannot scatter into it");
  }

  /* create the scatter context */
  ierr = VecGetLocalSize(da->natural,&m);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(da->natural,&start,PETSC_NULL);CHKERRQ(ierr);

  ierr = DAGetNatural_Private(da,&Nlocal,&to);CHKERRQ(ierr);
  if (Nlocal != m) SETERRQ2(PETSC_ERR_PLIB,"Internal error: Nlocal %D local vector size %D",Nlocal,m);
  ierr = ISCreateStride(da->comm,m,start,1,&from);CHKERRQ(ierr);
  ierr = VecCreateMPIWithArray(da->comm,da->Nlocal,PETSC_DETERMINE,0,&global);
  ierr = VecScatterCreate(global,from,da->natural,to,&da->gton);CHKERRQ(ierr);
  ierr = VecDestroy(global);CHKERRQ(ierr);
  ierr = ISDestroy(from);CHKERRQ(ierr);
  ierr = ISDestroy(to);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DAGlobalToNaturalBegin"
/*@
   DAGlobalToNaturalBegin - Maps values from the global vector to a global vector
   in the "natural" grid ordering. Must be followed by 
   DAGlobalToNaturalEnd() to complete the exchange.

   Collective on DA

   Input Parameters:
+  da - the distributed array context
.  g - the global vector
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the natural ordering values

   Level: advanced

   Notes:
   The global and natrual vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateNaturalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, begin

.seealso: DAGlobalToNaturalEnd(), DALocalToGlobal(), DACreate2d(), 
          DAGlobalToLocalBegin(), DAGlobalToLocalEnd(), DACreateNaturalVector(),
          DALocalToGlobalBegin(), DALocalToGlobalEnd()
@*/
PetscErrorCode DAGlobalToNaturalBegin(DA da,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,4);
  if (!da->gton) {
    /* create the scatter context */
    ierr = DAGlobalToNatural_Create(da);CHKERRQ(ierr);
  }
  ierr = VecScatterBegin(g,l,mode,SCATTER_FORWARD,da->gton);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DAGlobalToNaturalEnd"
/*@
   DAGlobalToNaturalEnd - Maps values from the global vector to a global vector
   in the natural ordering. Must be preceeded by DAGlobalToNaturalBegin().

   Collective on DA

   Input Parameters:
+  da - the distributed array context
.  g - the global vector
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the global values in the natural ordering

   Level: advanced

   Notes:
   The global and local vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateNaturalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, end

.seealso: DAGlobalToNaturalBegin(), DALocalToGlobal(), DACreate2d(),
          DAGlobalToLocalBegin(), DAGlobalToLocalEnd(), DACreateNaturalVector(),
          DALocalToGlobalBegin(), DALocalToGlobalEnd()
@*/
PetscErrorCode DAGlobalToNaturalEnd(DA da,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,4);
  ierr = VecScatterEnd(g,l,mode,SCATTER_FORWARD,da->gton);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DANaturalToGlobalBegin"
/*@
   DANaturalToGlobalBegin - Maps values from a global vector in the "natural" ordering 
   to a global vector in the PETSc DA grid ordering. Must be followed by 
   DANaturalToGlobalEnd() to complete the exchange.

   Collective on DA

   Input Parameters:
+  da - the distributed array context
.  g - the global vector in a natural ordering
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the values in the DA ordering

   Level: advanced

   Notes:
   The global and natural vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateNaturalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, begin

.seealso: DAGlobalToNaturalEnd(), DAGlobalToNaturalBegin(), DALocalToGlobal(), DACreate2d(), 
          DAGlobalToLocalBegin(), DAGlobalToLocalEnd(), DACreateNaturalVector(),
          DALocalToGlobalBegin(), DALocalToGlobalEnd()

@*/
PetscErrorCode DANaturalToGlobalBegin(DA da,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,4);
  if (!da->gton) {
    /* create the scatter context */
    ierr = DAGlobalToNatural_Create(da);CHKERRQ(ierr);
  }
  ierr = VecScatterBegin(g,l,mode,SCATTER_REVERSE,da->gton);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DANaturalToGlobalEnd"
/*@
   DANaturalToGlobalEnd - Maps values from the natural ordering global vector 
   to a global vector in the PETSc DA ordering. Must be preceeded by DANaturalToGlobalBegin().

   Collective on DA

   Input Parameters:
+  da - the distributed array context
.  g - the global vector in a natural ordering
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the global values in the PETSc DA ordering

   Level: intermediate

   Notes:
   The global and local vectors used here need not be the same as those
   obtained from DACreateGlobalVector() and DACreateNaturalVector(), BUT they
   must have the same parallel data layout; they could, for example, be 
   obtained with VecDuplicate() from the DA originating vectors.

.keywords: distributed array, global to local, end

.seealso: DAGlobalToNaturalBegin(), DAGlobalToNaturalEnd(), DALocalToGlobal(), DACreate2d(),
          DAGlobalToLocalBegin(), DAGlobalToLocalEnd(), DACreateNaturalVector(),
          DALocalToGlobalBegin(), DALocalToGlobalEnd()

@*/
PetscErrorCode DANaturalToGlobalEnd(DA da,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DA_COOKIE,1);
  PetscValidHeaderSpecific(l,VEC_COOKIE,2);
  PetscValidHeaderSpecific(g,VEC_COOKIE,4);
  ierr = VecScatterEnd(g,l,mode,SCATTER_REVERSE,da->gton);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

