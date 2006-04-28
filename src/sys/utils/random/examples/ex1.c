
static char help[] = "Tests PetscRandom functions.\n\n";

#include "petsc.h"
#include "petscsys.h"

/* Usage: 
   ./ex1 -log_summary
*/

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscInt       i,n = 1000,*values;
  int            event;
  PetscRandom    rnd;
  PetscScalar    value;
  PetscErrorCode ierr;

  PetscInitialize(&argc,&argv,(char *)0,help);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRQ(ierr);
  
  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&rnd);CHKERRQ(ierr);
  ierr = PetscRandomSetType(rnd,PETSCRAND48);CHKERRQ(ierr); 
  ierr = PetscRandomSetFromOptions(rnd);CHKERRQ(ierr); 

  ierr = PetscMalloc(n*sizeof(PetscInt),&values);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscRandomGetValue(rnd,&value);CHKERRQ(ierr);
    /* printf("value[%d] = %g\n",i,value); */
    values[i] = (PetscInt)(n*PetscRealPart(value) + 2.0);
  }
  ierr = PetscSortInt(n,values);CHKERRQ(ierr);

  ierr = PetscLogEventRegister(&event,"Sort",0);CHKERRQ(ierr);
  ierr = PetscLogEventBegin(event,0,0,0,0);CHKERRQ(ierr);
 
  ierr = PetscRandomSeed(rnd);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscRandomGetValue(rnd,&value);CHKERRQ(ierr);
    values[i] = (PetscInt)(n*PetscRealPart(value) + 2.0);
    /* printf("value[%d] = %g\n",i,value); */
  }
  ierr = PetscSortInt(n,values);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(event,0,0,0,0);CHKERRQ(ierr);

  for (i=1; i<n; i++) {
    if (values[i] < values[i-1]) SETERRQ(1,"Values not sorted");
  }
  ierr = PetscFree(values);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(rnd);CHKERRQ(ierr);

  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 
