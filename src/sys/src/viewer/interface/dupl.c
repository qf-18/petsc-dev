
#include "src/sys/src/viewer/viewerimpl.h"  /*I "petscviewer.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerGetSingleton" 
/*@
   PetscViewerGetSingleton - Creates a new PetscViewer (same type as the old)
    that lives on a single processor (with MPI_comm PETSC_COMM_SELF)

    Collective on PetscViewer

   Input Parameter:
.  viewer - the PetscViewer to be duplicated

   Output Parameter:
.  outviewer - new PetscViewer

   Level: advanced

   Notes: Call PetscViewerRestoreSingleton() to return this PetscViewer, NOT PetscViewerDestroy()

     This is most commonly used to view a sequential object that is part of a 
    parallel object. For example block Jacobi PC view could use this to obtain a
    PetscViewer that is used with the sequential KSP on one block of the preconditioner.

   Concepts: PetscViewer^sequential version

.seealso: PetscViewerSocketOpen(), PetscViewerASCIIOpen(), PetscViewerDrawOpen(), PetscViewerRestoreSingleton()
@*/
PetscErrorCode PetscViewerGetSingleton(PetscViewer viewer,PetscViewer *outviewer)
{
  PetscErrorCode ierr,size;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  PetscValidPointer(outviewer,2);
  ierr = MPI_Comm_size(viewer->comm,&size);CHKERRQ(ierr);
  if (size == 1) {
    *outviewer = viewer;
    ierr = PetscObjectReference((PetscObject)viewer);CHKERRQ(ierr);
  } else if (viewer->ops->getsingleton) {
    ierr = (*viewer->ops->getsingleton)(viewer,outviewer);CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_ERR_SUP,"Cannot get singleton PetscViewer for type %s",viewer->type_name);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerRestoreSingleton" 
/*@
   PetscViewerRestoreSingleton - Restores a new PetscViewer obtained with PetscViewerGetSingleton().

    Collective on PetscViewer

   Input Parameters:
+  viewer - the PetscViewer to be duplicated
-  outviewer - new PetscViewer

   Level: advanced

   Notes: Call PetscViewerGetSingleton() to get this PetscViewer, NOT PetscViewerCreate()

.seealso: PetscViewerSocketOpen(), PetscViewerASCIIOpen(), PetscViewerDrawOpen(), PetscViewerGetSingleton()
@*/
PetscErrorCode PetscViewerRestoreSingleton(PetscViewer viewer,PetscViewer *outviewer)
{
  PetscErrorCode ierr,size;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);

  ierr = MPI_Comm_size(viewer->comm,&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = PetscObjectDereference((PetscObject)viewer);CHKERRQ(ierr);
    if (outviewer) *outviewer = 0;
  } else if (viewer->ops->restoresingleton) {
    ierr = (*viewer->ops->restoresingleton)(viewer,outviewer);CHKERRQ(ierr);
  } 
  PetscFunctionReturn(0);
}

