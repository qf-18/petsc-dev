#ifndef __PETSc_IndexSpace_h__
#define __PETSc_IndexSpace_h__

// this contains the definition of PetscMap
#include "petscvec.h"

#include "esi/petsc/object.h"

// The esi::petsc::IndexSpace supports the esi::IndexSpace interfaces

#include "esi/IndexSpace.h"

namespace esi{namespace petsc{

/**=========================================================================**/
template<class Ordinal> 
class IndexSpace : public virtual esi::IndexSpace<Ordinal>, public esi::petsc::Object
{
  public:

    // constructor.
    IndexSpace(MPI_Comm icomm) {this->map = 0;};	
    IndexSpace() {this->map = 0;};	

    // Construct an IndexSpace form an IndexSpace 
    IndexSpace(esi::IndexSpace<Ordinal>& sourceIndexSpace);

    // Construct an IndexSpace from a PETSc (old-style) map.
    IndexSpace(PetscMap sourceIndexSpace);

    // Basic constructor
    IndexSpace(MPI_Comm comm, int n, int N);

    // destructor.
    virtual ~IndexSpace(void);

    //  Interface for esi::Object  ---------------

    virtual esi::ErrorCode getInterface(const char* name, void*& iface);
    virtual esi::ErrorCode getInterfacesSupported(esi::Argv * list);

    //  Interface for esi::IndexSpace  ---------------

    // Get the size of this mapped dimension of the problem.
    virtual esi::ErrorCode getGlobalSize(Ordinal& globalSize);
    virtual esi::ErrorCode getLocalSize(Ordinal& localSize);

    // Get the size of this dimension of the problem, as well as 
    // the global offset info for all processors.
    virtual esi::ErrorCode getGlobalPartitionSizes(Ordinal* globalSizes);
    virtual esi::ErrorCode getGlobalPartitionOffsets(Ordinal* globalOffsets);

    virtual esi::ErrorCode getGlobalPartitionSetSize(Ordinal &) {return 1;};
    virtual esi::ErrorCode getLocalPartitionRank(Ordinal &) {return 1;};

    virtual esi::ErrorCode getGlobalColorSetSize(Ordinal &) {return 1;};
    virtual esi::ErrorCode getLocalColors(Ordinal *) {return 1;};
    virtual esi::ErrorCode getLocalIdentifiers(Ordinal *) {return 1;};

    // Get the local size offset info in this dimension.
    virtual esi::ErrorCode getLocalPartitionOffset(Ordinal& localOffset);

  class Factory : public virtual ::esi::IndexSpace<Ordinal>::Factory
  {
    public:

    // Destructor.
    virtual ~Factory(void){};

    // Construct a IndexSpace
    virtual esi::ErrorCode create(const char * name,void *comm,int m,int M,int base,esi::IndexSpace<Ordinal>*&v); 
  };

  private:
    PetscMap map;
};


/**=========================================================================**/
/*
    This is required for certain C++ compilers (borland,solaris) to 
   allow providing methods directly for IndexSpace<int>
*/
template<> 
class IndexSpace<int>: public virtual esi::IndexSpace<int>, public esi::petsc::Object
{
  public:

    // constructor.
    IndexSpace(MPI_Comm icomm) {};	

    // Construct an IndexSpace form an IndexSpace 
    IndexSpace(esi::IndexSpace<int>& sourceIndexSpace);

    // Construct an IndexSpace from a PETSc (old-style) map.
    IndexSpace(PetscMap sourceIndexSpace);

    // Basic constructor
    IndexSpace(MPI_Comm comm, int n, int N);

    // destructor.
    virtual ~IndexSpace(void);

    //  Interface for esi::Object  ---------------

    virtual esi::ErrorCode getInterface(const char* name, void*& iface);
    virtual esi::ErrorCode getInterfacesSupported(esi::Argv * list);

    //  Interface for esi::IndexSpace  ---------------

    // Get the size of this mapped dimension of the problem.
    virtual esi::ErrorCode getGlobalSize(int& globalSize);
    virtual esi::ErrorCode getLocalSize(int& localSize);

    // Get the size of this dimension of the problem, as well as 
    // the global offset info for all processors.
    virtual esi::ErrorCode getGlobalPartitionSizes(int* globalSizes);
    virtual esi::ErrorCode getGlobalPartitionOffsets(int* globalOffsets);

    virtual esi::ErrorCode getGlobalPartitionSetSize(int &) {return 1;};
    virtual esi::ErrorCode getLocalPartitionRank(int &) {return 1;};

    virtual esi::ErrorCode getGlobalColorSetSize(int &) {return 1;};
    virtual esi::ErrorCode getLocalColors(int*) {return 1;};
    virtual esi::ErrorCode getLocalIdentifiers(int*) {return 1;};

    // Get the local size offset info in this dimension.
    virtual esi::ErrorCode getLocalPartitionOffset(int& localOffset);

    class Factory : public virtual ::esi::IndexSpace<int>::Factory
  {
    public:

    // Destructor.
    virtual ~Factory(void){};

    // Construct a IndexSpace
    virtual esi::ErrorCode create(const char * name,void *comm,int m,int M,int base,esi::IndexSpace<int>*&v); 
  };

  private:
    PetscMap map;
};

}}

extern int ESICreateIndexSpace(const char * commname,void *comm,int m,::esi::IndexSpace<int>*&v);
#endif




