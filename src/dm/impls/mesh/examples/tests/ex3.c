static char help[] = "Mesh Distribution with SF.\n\n";
#include <petscdmmesh.h>
#include <petscsf.h>

typedef struct {
  DM            dm;                /* REQUIRED in order to use SNES evaluation functions */
  PetscInt      debug;             /* The debugging level */
  PetscMPIInt   rank;              /* The process rank */
  PetscMPIInt   numProcs;          /* The number of processes */
  PetscInt      dim;               /* The topological mesh dimension */
  PetscBool     interpolate;       /* Generate intermediate mesh elements */
  PetscReal     refinementLimit;   /* The largest allowable cell volume */
  char          filename[2048];    /* Optional filename to read mesh from */
  char          partitioner[2048]; /* The graph partitioner */
  PetscLogEvent createMeshEvent;
} AppCtx;

#undef __FUNCT__
#define __FUNCT__ "ProcessOptions"
PetscErrorCode ProcessOptions(MPI_Comm comm, AppCtx *options) {
  PetscErrorCode ierr;

  PetscFunctionBegin;
  options->debug           = 0;
  options->dim             = 2;
  options->interpolate     = PETSC_FALSE;
  options->refinementLimit = 0.0;

  ierr = MPI_Comm_size(comm, &options->numProcs);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &options->rank);CHKERRQ(ierr);
  ierr = PetscOptionsBegin(comm, "", "Mesh Distribution Options", "DMMESH");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-debug", "The debugging level", "ex1.c", options->debug, &options->debug, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-dim", "The topological mesh dimension", "ex1.c", options->dim, &options->dim, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-interpolate", "Generate intermediate mesh elements", "ex1.c", options->interpolate, &options->interpolate, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-refinement_limit", "The largest allowable cell volume", "ex1.c", options->refinementLimit, &options->refinementLimit, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscStrcpy(options->filename, "");CHKERRQ(ierr);
  ierr = PetscOptionsString("-filename", "The input filename", "ex1.c", options->filename, options->filename, 2048, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscStrcpy(options->partitioner, "chaco");CHKERRQ(ierr);
  ierr = PetscOptionsString("-partitioner", "The graph partitioner", "ex1.c", options->partitioner, options->partitioner, 2048, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();

  ierr = PetscLogEventRegister("CreateMesh",    DM_CLASSID,   &options->createMeshEvent);CHKERRQ(ierr);
  PetscFunctionReturn(0);
};

#undef __FUNCT__
#define __FUNCT__ "CreateMesh"
PetscErrorCode CreateMesh(MPI_Comm comm, AppCtx *user, DM *dm)
{
  PetscInt       dim             = user->dim;
  PetscBool      interpolate     = user->interpolate;
  PetscReal      refinementLimit = user->refinementLimit;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLogEventBegin(user->createMeshEvent,0,0,0,0);CHKERRQ(ierr);
  ierr = DMMeshCreateBoxMesh(comm, dim, interpolate, dm);CHKERRQ(ierr);
  {
    DM refinedMesh = PETSC_NULL;

    /* Refine mesh using a volume constraint */
    ierr = DMMeshRefine(*dm, refinementLimit, interpolate, &refinedMesh);CHKERRQ(ierr);
    if (refinedMesh) {
      ierr = DMDestroy(dm);CHKERRQ(ierr);
      *dm  = refinedMesh;
    }
  }
  ierr = DMSetFromOptions(*dm);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(user->createMeshEvent,0,0,0,0);CHKERRQ(ierr);
  user->dm = *dm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMMeshConvertOverlapToSF"
PetscErrorCode DMMeshConvertOverlapToSF(DM dm, PetscSF *sf)
{
  ALE::Obj<PETSC_MESH_TYPE> mesh;
  PetscInt      *local;
  PetscSFNode   *remote;
  PetscInt       numPoints;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscSFCreate(((PetscObject) dm)->comm, sf);CHKERRQ(ierr);
  ierr = DMMeshGetMesh(dm, mesh);CHKERRQ(ierr);
  {
    /* The local points have degree 1
         We use the recv overlap
    */
    ALE::Obj<PETSC_MESH_TYPE::recv_overlap_type> overlap = mesh->getRecvOverlap();

    numPoints = overlap->getNumPoints();
    ierr = PetscMalloc(numPoints * sizeof(PetscInt), &local);CHKERRQ(ierr);
    ierr = PetscMalloc(numPoints * sizeof(PetscSFNode), &remote);CHKERRQ(ierr);
    for(PetscInt r = 0, i = 0; r < overlap->getNumRanks(); ++r) {
      const PetscInt                                                      rank   = overlap->getRank(r);
      const PETSC_MESH_TYPE::recv_overlap_type::supportSequence::iterator cBegin = overlap->supportBegin(rank);
      const PETSC_MESH_TYPE::recv_overlap_type::supportSequence::iterator cEnd   = overlap->supportEnd(rank);

      for(PETSC_MESH_TYPE::recv_overlap_type::supportSequence::iterator c_iter = cBegin; c_iter != cEnd; ++c_iter, ++i) {
        local[i]        = *c_iter;
        remote[i].rank  = rank;
        remote[i].index = c_iter.color();
      }
    }
    ierr = PetscSFSetGraph(*sf, numPoints, numPoints, local, PETSC_OWN_POINTER, remote, PETSC_OWN_POINTER);CHKERRQ(ierr);
    ierr = PetscSFView(*sf, PETSC_NULL);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSFConvertPartition"
PetscErrorCode PetscSFConvertPartition(PetscSF sfPart, PetscSection partSection, IS partition, ISLocalToGlobalMapping *renumbering, PetscSF *sf)
{
  MPI_Comm        comm = ((PetscObject) sfPart)->comm;
  PetscSF         sfPoints;
  PetscSFNode    *remotePoints;
  PetscInt       *renumArray;
  PetscInt        numRanks, numRemoteRanks = 0, p, i;
  const PetscInt *rankOffsets;
  PetscInt        localSize[2], partSize, *partSizes = PETSC_NULL, *partOffsets = PETSC_NULL;
  const PetscInt *partArray;
  PetscMPIInt     numProcs, rank;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(comm, &numProcs);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = PetscSFGetRanks(sfPart, &numRanks, PETSC_NULL, &rankOffsets, PETSC_NULL, PETSC_NULL);CHKERRQ(ierr);
  numRemoteRanks = rankOffsets[numRanks];
  ierr = PetscMalloc2(2*numRemoteRanks,PetscInt,&partSizes,2*numRemoteRanks,PetscInt,&partOffsets);CHKERRQ(ierr);
  for(p = 0; p < numRemoteRanks; ++p) {
    ierr = PetscSectionGetDof(partSection, p, &partSizes[2*p+0]);CHKERRQ(ierr);
    ierr = PetscSectionGetOffset(partSection, p, &partSizes[2*p+1]);CHKERRQ(ierr);
  }
  localSize[0] = 0; localSize[1] = 0;
  ierr = PetscSFFetchAndOpBegin(sfPart, MPIU_2INT, &localSize, partSizes, partOffsets, MPIU_SUM);CHKERRQ(ierr);
  ierr = PetscSFFetchAndOpEnd(sfPart, MPIU_2INT, &localSize, partSizes, partOffsets, MPIU_SUM);CHKERRQ(ierr);
  ierr = PetscSynchronizedPrintf(comm, "localSize %d %d\n", localSize[0], localSize[1]);CHKERRQ(ierr);
  ierr = PetscSynchronizedFlush(comm);CHKERRQ(ierr);
  for(p = 0; p < numRemoteRanks; ++p) {
    ierr = PetscPrintf(comm, "offset for rank %d: %d\n", p, partOffsets[p]);CHKERRQ(ierr);
  }
  ierr = PetscFree2(partSizes,partOffsets);CHKERRQ(ierr);
  /* Create the inverse graph for the partition */
  ierr = PetscMalloc(localSize[0] * sizeof(PetscSFNode), &remotePoints);CHKERRQ(ierr);
  for(i = 0; i < localSize[0]; ++i) {
    remotePoints[i].rank  = 0;
    remotePoints[i].index = localSize[1] + i;
  }
  ierr = ISGetLocalSize(partition, &partSize);CHKERRQ(ierr);
  ierr = PetscSFCreate(comm, &sfPoints);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(sfPoints, partSize, localSize[0], PETSC_NULL, PETSC_OWN_POINTER, remotePoints, PETSC_USE_POINTER);CHKERRQ(ierr);
  /* Send global point numbers
     - owned values are sent
     - local values are received
  */
  ierr = PetscMalloc(localSize[0] * sizeof(PetscInt), &renumArray);CHKERRQ(ierr);
  ierr = ISGetIndices(partition, &partArray);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sfPoints, MPIU_INT, partArray, renumArray);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sfPoints, MPIU_INT, partArray, renumArray);CHKERRQ(ierr);
  ierr = ISRestoreIndices(partition, &partArray);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&sfPoints);CHKERRQ(ierr);

  ierr = ISLocalToGlobalMappingCreate(comm, localSize[0], renumArray, PETSC_OWN_POINTER, renumbering);CHKERRQ(ierr);
  ierr = PetscPrintf(comm, "Point Renumbering after partition:\n");CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingView(*renumbering, PETSC_NULL);CHKERRQ(ierr);

  for(i = 0; i < localSize[0]; ++i) {
    remotePoints[i].rank  = 0;
    remotePoints[i].index = renumArray[i];
  }
  ierr = PetscSFCreate(comm, sf);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(*sf, partSize, localSize[0], PETSC_NULL, PETSC_OWN_POINTER, remotePoints, PETSC_OWN_POINTER);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSFDistributeSection"
PetscErrorCode PetscSFDistributeSection(PetscSF sf, PetscSection originalSection, PetscInt **remoteOffsets, PetscSection *newSection)
{
  const PetscInt *rankOffsets;
  PetscInt        numRanks, numLocalPoints;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = PetscSFView(sf, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscSFGetRanks(sf, &numRanks, PETSC_NULL, &rankOffsets, PETSC_NULL, PETSC_NULL);CHKERRQ(ierr);
  numLocalPoints = rankOffsets[numRanks];
  ierr = PetscMalloc(numLocalPoints * sizeof(PetscInt), remoteOffsets);CHKERRQ(ierr);
  ierr = PetscSectionCreate(((PetscObject) sf)->comm, newSection);CHKERRQ(ierr);
  ierr = PetscSectionSetChart(*newSection, 0, numLocalPoints);CHKERRQ(ierr);
  /* Could fuse these at the cost of a copy and extra allocation */
  ierr = PetscSFBcastBegin(sf, MPIU_INT, &originalSection->atlasDof[-originalSection->atlasLayout.pStart], &(*newSection)->atlasDof[-(*newSection)->atlasLayout.pStart]);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf, MPIU_INT, &originalSection->atlasDof[-originalSection->atlasLayout.pStart], &(*newSection)->atlasDof[-(*newSection)->atlasLayout.pStart]);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf, MPIU_INT, &originalSection->atlasOff[-originalSection->atlasLayout.pStart], *remoteOffsets);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf, MPIU_INT, &originalSection->atlasOff[-originalSection->atlasLayout.pStart], *remoteOffsets);CHKERRQ(ierr);
  ierr = PetscSectionSetUp(*newSection);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSFCreateSectionSF"
/*
  . section       - Data layout of local points for incoming data
  . remoteOffsets - Offsets for point data on remote processes
*/
PetscErrorCode PetscSFCreateSectionSF(PetscSF sf, PetscSection section, const PetscInt remoteOffsets[], PetscSF *sectionSF)
{
  MPI_Comm           comm = ((PetscObject) sf)->comm;
  PetscInt           numRanks;
  const PetscInt    *ranks, *rankOffsets;
  const PetscMPIInt *localPoints, *remotePoints;
  PetscInt           pStart, pEnd;
  PetscInt           numPoints, numIndices = 0;
  PetscInt          *localIndices;
  PetscSFNode       *remoteIndices;
  PetscInt           i, r, ind;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  ierr = PetscSFView(sf, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscSFGetRanks(sf, &numRanks, &ranks, &rankOffsets, &localPoints, &remotePoints);CHKERRQ(ierr);
  numPoints = rankOffsets[numRanks];
  for(i = 0; i < numPoints; ++i) {
    PetscInt dof;

    ierr = PetscSectionGetDof(section, localPoints[i], &dof);CHKERRQ(ierr);
    numIndices += dof;
  }
  ierr = PetscMalloc(numIndices * sizeof(PetscInt), &localIndices);CHKERRQ(ierr);
  ierr = PetscMalloc(numIndices * sizeof(PetscSFNode), &remoteIndices);CHKERRQ(ierr);
  /* Create new index graph */
  ierr = PetscSectionGetChart(section, &pStart,  &pEnd);CHKERRQ(ierr);
  for(r = 0, ind = 0; r < numRanks; ++r) {
    PetscInt rank = ranks[r];

    for(i = rankOffsets[r]; i < rankOffsets[r+1]; ++i) {
      PetscInt localPoint   = localPoints[i];
      PetscInt remoteOffset = remoteOffsets[localPoint-pStart];
      PetscInt localOffset, dof, d;

      ierr = PetscSectionGetOffset(section, localPoint, &localOffset);CHKERRQ(ierr);
      ierr = PetscSectionGetDof(section, localPoint, &dof);CHKERRQ(ierr);
      for(d = 0; d < dof; ++d, ++ind) {
        localIndices[ind]        = localOffset+d;
        remoteIndices[ind].rank  = rank;
        remoteIndices[ind].index = remoteOffset+d;
      }
    }
  }
  ierr = PetscFree(remoteOffsets);CHKERRQ(ierr);
  if (numIndices != ind) {SETERRQ2(comm, PETSC_ERR_PLIB, "Inconsistency in indices, %d should be %d", ind, numIndices);}
  ierr = PetscSFCreate(comm, sectionSF);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(*sectionSF, numIndices, numIndices, localIndices, PETSC_OWN_POINTER, remoteIndices, PETSC_OWN_POINTER);CHKERRQ(ierr);
  ierr = PetscSFView(*sectionSF, PETSC_NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DistributeMesh"
PetscErrorCode DistributeMesh(DM dm, AppCtx *user, DM *parallelDM)
{
  ALE::Obj<PETSC_MESH_TYPE> mesh;
  MPI_Comm       comm = ((PetscObject) dm)->comm;
  PetscSF        partSF, pointSF;
  ISLocalToGlobalMapping renumbering;
  IS             cellPart,        part;
  PetscSection   cellPartSection, partSection;
  PetscMPIInt    numProcs, rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(comm, &numProcs);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = DMMeshGetMesh(dm, mesh);CHKERRQ(ierr);
  /* Create cell partition - We need to rewrite to use IS, use the MatPartition stuff */
  {
    typedef PETSC_MESH_TYPE::point_type          point_type;
    typedef ALE::Partitioner<>::part_type        rank_type;
    typedef ALE::ISection<rank_type, point_type> partition_type;
    const ALE::Obj<partition_type> cellPartition = new partition_type(comm, 0, numProcs, user->debug);
    const PetscInt                 height        = 0;
    PetscInt                       numRemoteRanks, p;
    PetscSFNode                   *remoteRanks;

    ALE::Partitioner<>::createPartitionV(mesh, cellPartition, height);
    if (user->debug) {
      PetscViewer    viewer;
      PetscErrorCode ierr;

      cellPartition->view("Cell Partition");
      ierr = PetscViewerCreate(mesh->comm(), &viewer);CHKERRXX(ierr);
      ierr = PetscViewerSetType(viewer, PETSCVIEWERASCII);CHKERRXX(ierr);
      ierr = PetscViewerFileSetName(viewer, "mesh.vtk");CHKERRXX(ierr);
      ierr = PetscViewerSetFormat(viewer, PETSC_VIEWER_ASCII_VTK);CHKERRQ(ierr);
      ierr = DMView(dm, viewer);CHKERRQ(ierr);
      //ierr = ISView(mesh, cellPartition);CHKERRQ(ierr);
      ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
    }
    /* Convert to PetscSection+IS and SF */
    ierr = PetscSectionCreate(comm, &cellPartSection);CHKERRQ(ierr);
    ierr = PetscSectionSetChart(cellPartSection, 0, numProcs);CHKERRQ(ierr);
    for(p = 0; p < numProcs; ++p) {
      ierr = PetscSectionSetDof(cellPartSection, p, cellPartition->getFiberDimension(p));CHKERRQ(ierr);
    }
    ierr = PetscSectionSetUp(cellPartSection);CHKERRQ(ierr);
    ierr = ISCreateGeneral(comm, cellPartition->size(), cellPartition->restrictSpace(), PETSC_COPY_VALUES, &cellPart);CHKERRQ(ierr);
    ierr = PetscSectionView(cellPartSection, PETSC_NULL);CHKERRQ(ierr);
    ierr = ISView(cellPart, PETSC_NULL);CHKERRQ(ierr);
    /* Create SF assuming a serial partition for all processes: Could check for IS length here */
    if (!rank) {
      numRemoteRanks = numProcs;
    } else {
      numRemoteRanks = 0;
    }
    ierr = PetscMalloc(numRemoteRanks * sizeof(PetscSFNode), &remoteRanks);CHKERRQ(ierr);
    for(p = 0; p < numRemoteRanks; ++p) {
      remoteRanks[p].rank  = p;
      remoteRanks[p].index = 0;
    }
    ierr = PetscSFCreate(comm, &partSF);CHKERRQ(ierr);
    ierr = PetscSFSetGraph(partSF, 1, numRemoteRanks, PETSC_NULL, PETSC_OWN_POINTER, remoteRanks, PETSC_OWN_POINTER);CHKERRQ(ierr);
    ierr = PetscSFView(partSF, PETSC_NULL);CHKERRQ(ierr);
  }
  /* Close the partition over the mesh */
  ierr = ALE::Partitioner<>::createPartitionClosureV(mesh, cellPartSection, cellPart, &partSection, &part, 0);CHKERRQ(ierr);
  ierr = PetscSectionView(partSection, PETSC_NULL);CHKERRQ(ierr);
  ierr = ISView(part, PETSC_NULL);CHKERRQ(ierr);
  /* Distribute sieve points and the global point numbering (replaces creating remote bases) */
  ierr = PetscSFConvertPartition(partSF, partSection, part, &renumbering, &pointSF);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&partSF);CHKERRQ(ierr);
  /* Distribute cones
   - Partitioning:         input partition point map and naive sf, output sf with inverse of map, distribute points
   - Distribute section:   input current sf, communicate sizes and offsets, output local section and offsets (only use for new sf)
   - Create SF for values: input current sf and offsets, output new sf
   - Distribute values:    input new sf, communicate values
   */
  PetscSF      coneSF;
  PetscSection originalConeSection, newConeSection;
  PetscInt     pStart, pEnd, p;
  PetscInt    *remoteOffsets;

  /* Create PetscSection for original cones */
  ierr = DMMeshCreateConeSection(dm, &originalConeSection);CHKERRQ(ierr);
  /* Distribute Section */
  ierr = PetscSFDistributeSection(pointSF, originalConeSection, &remoteOffsets, &newConeSection);CHKERRQ(ierr);
  ierr = PetscSectionView(originalConeSection, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = PetscSectionView(newConeSection, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  /* Create SF for cones */
  ierr = PetscSFCreateSectionSF(pointSF, newConeSection, remoteOffsets, &coneSF);CHKERRQ(ierr);
  ierr = PetscSectionGetChart(newConeSection, &pStart, &pEnd);CHKERRQ(ierr);

  ALE::Obj<PETSC_MESH_TYPE>             newMesh  = new PETSC_MESH_TYPE(comm, mesh->getDimension(), mesh->debug());
  ALE::Obj<PETSC_MESH_TYPE::sieve_type> newSieve = new PETSC_MESH_TYPE::sieve_type(comm, pStart, pEnd, mesh->debug());

  newMesh->setSieve(newSieve);
  for(p = pStart; p < pEnd; ++p) {
    PetscInt coneSize;

    ierr = PetscSectionGetDof(newConeSection, p, &coneSize);CHKERRQ(ierr);
    newMesh->getSieve()->setConeSize(p, coneSize);
  }
  newMesh->getSieve()->allocate();
  PetscInt    *cones    = mesh->getSieve()->getCones();
  PetscInt    *newCones = newMesh->getSieve()->getCones();
  PetscInt     newConesSize;

  ierr = PetscSFBcastBegin(coneSF, MPIU_INT, cones, newCones);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(coneSF, MPIU_INT, cones, newCones);CHKERRQ(ierr);
  ierr = PetscSectionGetStorageSize(newConeSection, &newConesSize);CHKERRQ(ierr);
  ierr = ISGlobalToLocalMappingApply(renumbering, IS_GTOLM_MASK, newConesSize, newCones, PETSC_NULL, newCones);CHKERRQ(ierr);

  ierr = PetscSectionDestroy(&originalConeSection);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&newConeSection);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&pointSF);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&coneSF);CHKERRQ(ierr);

  newMesh->getSieve()->symmetrize();
  newMesh->stratify();
  newMesh->view("Parallel Mesh");

  ierr = ISDestroy(&cellPart);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&cellPartSection);CHKERRQ(ierr);
  ierr = ISDestroy(&part);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&partSection);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&pointSF);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingDestroy(&renumbering);CHKERRQ(ierr);

  ierr = DMMeshCreate(comm, parallelDM);CHKERRQ(ierr);
  ierr = DMMeshSetMesh(*parallelDM, newMesh);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char *argv[])
{
  MPI_Comm       comm;
  DM             dm, parallelDM;
  AppCtx         user;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscInitialize(&argc, &argv, (char *) 0, help);CHKERRQ(ierr);
  comm = PETSC_COMM_WORLD;
  ierr = ProcessOptions(comm, &user);CHKERRQ(ierr);
  ierr = CreateMesh(comm, &user, &dm);CHKERRQ(ierr);
  ierr = DistributeMesh(dm, &user, &parallelDM);CHKERRQ(ierr);
  ierr = DMSetFromOptions(parallelDM);CHKERRQ(ierr);
  ierr = DMDestroy(&parallelDM);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  ierr = PetscFinalize();
  PetscFunctionReturn(0);
}
