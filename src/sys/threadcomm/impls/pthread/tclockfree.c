#include <../src/sys/threadcomm/impls/pthread/tcpthreadimpl.h>

#define THREAD_TERMINATE      0
#define THREAD_INITIALIZED    1
#define THREAD_CREATED        0
#if defined PETSC_HAVE_MALLOC_H
#include <malloc.h>
#endif

/* lock-free data structure */
typedef struct {
  PetscThreadCommJobCtx *data;
  PetscInt           *my_job_status;
} sjob_lockfree;

static sjob_lockfree job_lockfree = {NULL,NULL};

extern PetscThreadCommJobQueue PetscJobQueue;

/* Checks whether this thread is a member of tcomm */
PetscBool CheckThreadCommMembership(PetscInt myrank,PetscThreadComm tcomm)
{
  PetscInt i;
  PetscThreadComm_PThread ptcomm;

  ptcomm = (PetscThreadComm_PThread)tcomm->data;

  for(i=0;i<tcomm->nworkThreads;i++) {
    if(myrank == ptcomm->granks[i])
       return PETSC_TRUE;
  }
  return PETSC_FALSE;
}

void SparkThreads_LockFree(PetscInt myrank,PetscThreadComm tcomm,PetscThreadCommJobCtx job)
{
  PetscThreadComm_PThread  ptcomm;

  ptcomm = (PetscThreadComm_PThread)tcomm->data;
  
  switch(ptcomm->spark) {
  case PTHREADPOOLSPARK_SELF:
    if(CheckThreadCommMembership(myrank,tcomm)) {
      job_lockfree.data[myrank] = job;
      job->job_status[myrank]   = THREAD_JOB_RECIEVED;
    }
    break;
  }
}

void* PetscPThreadCommFunc_LockFree(void* arg)
{
  PetscInt              my_job_counter = 0,my_kernel_ctr=0;
  PetscInt              kernel_ctr,n=0;
  PetscThreadCommJobCtx job;

#if defined(PETSC_PTHREAD_LOCAL)
  PetscPThreadRank = *(PetscInt*)arg;
#else
  PetscInt PetscPThreadRank=*(PetscInt*)arg;
  pthread_setspecific(PetscPThreadRankkey,&PetscPThreadRank);
#endif

#if defined(PETSC_HAVE_SCHED_CPU_SET_T)
  PetscPThreadCommDoCoreAffinity();
#endif
  job_lockfree.data[PetscPThreadRank] = 0;
  job_lockfree.my_job_status[PetscPThreadRank] = THREAD_INITIALIZED;

  /* Spin loop */
  while(PetscReadOnce(int,job_lockfree.my_job_status[PetscPThreadRank]) != THREAD_TERMINATE) {
    kernel_ctr = PetscReadOnce(int,PetscJobQueue->ctr) + n*PETSC_KERNELS_MAX;
    if(kernel_ctr != my_kernel_ctr) {
      job = PetscJobQueue->jobs[my_job_counter];
      /* Spark the thread pool */
      SparkThreads_LockFree(PetscPThreadRank,job->tcomm,job);
      if(PetscReadOnce(int,job->job_status[PetscPThreadRank]) == THREAD_JOB_RECIEVED) {
	/* Do own job */
	PetscRunKernel(PetscPThreadRank,job_lockfree.data[PetscPThreadRank]->nargs,job_lockfree.data[PetscPThreadRank]);
	/* Post job completed status */
	job->job_status[PetscPThreadRank] = THREAD_JOB_COMPLETED;
      } 
      my_job_counter = (my_job_counter+1)%PETSC_KERNELS_MAX;
      if(!my_job_counter) n++;
      my_kernel_ctr++;
    }
  }

  return NULL;
}

#undef __FUNCT__
#define __FUNCT__ "PetscThreadCommBarrier_PThread_LockFree"
PetscErrorCode PetscThreadCommBarrier_PThread_LockFree(PetscThreadComm tcomm)
{
  PetscInt active_threads=0,i;
  PetscBool wait=PETSC_TRUE;
  PetscThreadComm_PThread ptcomm=(PetscThreadComm_PThread)tcomm->data;
  PetscThreadCommJobCtx job=PetscJobQueue->jobs[tcomm->job_ctr];

  PetscFunctionBegin;
  if(tcomm->nworkThreads == 1 && ptcomm->ismainworker) {
    PetscFunctionReturn(0);
  }
  /* Loop till all threads signal that they have done their job */
  while(wait) {
    for(i=0;i<tcomm->nworkThreads;i++) active_threads += PetscReadOnce(int,job->job_status[ptcomm->granks[i]]);
    if(active_threads) active_threads = 0;
    else wait=PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscPThreadCommInitialize_LockFree"
PetscErrorCode PetscPThreadCommInitialize_LockFree(PetscThreadComm tcomm)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscThreadComm_PThread ptcomm=(PetscThreadComm_PThread)tcomm->data;

  PetscFunctionBegin;

#if defined(PETSC_HAVE_MEMALIGN)
  /* Align data and my_job_status so that each thread reads from/writes to a seperate cache line */
  job_lockfree.data = (PetscThreadCommJobCtx*)memalign(PETSC_LEVEL1_DCACHE_LINESIZE,tcomm->nworkThreads*PETSC_LEVEL1_DCACHE_LINESIZE);
  job_lockfree.my_job_status = (int*)memalign(PETSC_LEVEL1_DCACHE_LINESIZE,tcomm->nworkThreads*PETSC_LEVEL1_DCACHE_LINESIZE);
#else
  ierr = PetscMalloc(tcomm->nworkThreads*sizeof(PetscThreadCommJobCtx),&job_lockfree.data);CHKERRQ(ierr);
  ierr = PetscMalloc(tcomm->nworkThreads*sizeof(PetscInt),&job_lockfree.my_job_status);CHKERRQ(ierr);
#endif
  /* Create threads */
  for(i=ptcomm->thread_num_start; i < tcomm->nworkThreads;i++) {
    job_lockfree.my_job_status[i] = THREAD_CREATED;
    ierr = pthread_create(&ptcomm->tid[i],NULL,&PetscPThreadCommFunc_LockFree,&ptcomm->granks[i]);CHKERRQ(ierr);
  }

  if(ptcomm->ismainworker) job_lockfree.my_job_status[0] = THREAD_INITIALIZED;

  PetscInt  threads_initialized=0;
  /* Wait till all threads have been initialized */
  while(threads_initialized != tcomm->nworkThreads) {
    threads_initialized=0;
    for(i=0;i<tcomm->nworkThreads;i++) {
      if(!job_lockfree.my_job_status[ptcomm->granks[i]])
	break;
      threads_initialized++;
    }
  }

  /* Put a barrier so that all threads get pinned properly */
  ierr = PetscThreadCommBarrier_PThread_LockFree(tcomm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscPThreadCommFinalize_LockFree"
PetscErrorCode PetscPThreadCommFinalize_LockFree(PetscThreadComm tcomm)
{
  PetscErrorCode           ierr;
  void*                    jstatus;
  PetscThreadComm_PThread  ptcomm=(PetscThreadComm_PThread)tcomm->data;
  PetscInt                 i;
  PetscFunctionBegin;
  ierr = PetscThreadCommBarrier_PThread_LockFree(tcomm);CHKERRQ(ierr);
  for(i=ptcomm->thread_num_start; i < tcomm->nworkThreads;i++) {
    job_lockfree.my_job_status[i] = THREAD_TERMINATE;
    ierr = pthread_join(ptcomm->tid[i],&jstatus);CHKERRQ(ierr);
  }
#if defined(PETSC_HAVE_MEMALIGN)
  free(job_lockfree.my_job_status);
  free(job_lockfree.data);
#else
  ierr = PetscFree(job_lockfree.my_job_status);CHKERRQ(ierr);
  ierr = PetscFree(job_lockfree.data);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscThreadCommRunKernel_PThread_LockFree"
PetscErrorCode PetscThreadCommRunKernel_PThread_LockFree(MPI_Comm comm,PetscThreadCommJobCtx job)
{
  PetscErrorCode           ierr;
  PetscThreadComm          tcomm=0;
  PetscThreadComm_PThread  ptcomm;    
  
  PetscFunctionBegin;
  ierr = PetscCommGetThreadComm(comm,&tcomm);CHKERRQ(ierr);
  ptcomm = (PetscThreadComm_PThread)tcomm->data;

  if(ptcomm->ismainworker) {
    job->job_status[0] = THREAD_JOB_RECIEVED;
    job_lockfree.data[0]  = job;
    PetscRunKernel(0,job->nargs, job_lockfree.data[0]);
    job->job_status[0]    = THREAD_JOB_COMPLETED;
  }
  if(ptcomm->synchronizeafter) {
    ierr = PetscThreadCommBarrier(comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
