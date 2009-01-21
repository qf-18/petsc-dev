static char help[] = "Mesh Refinement Tests.\n\n";

#include <petscmesh_viewers.hh>

using ALE::Obj;

typedef struct {
  PetscInt debug;
  PetscInt numLevels; // The number of refinement levels
} Options;

#undef __FUNCT__
#define __FUNCT__ "ProcessOptions"
PetscErrorCode ProcessOptions(MPI_Comm comm, Options *options)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  options->debug     = 0;
  options->numLevels = 1;

  ierr = PetscOptionsBegin(comm, "", "Options for the Sieve package tests", "Sieve");CHKERRQ(ierr);
    ierr = PetscOptionsInt("-debug", "Debugging flag", "refineTests", options->debug, &options->debug, PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsInt("-num_levels", "The number of refinement levels", "refineTests", options->numLevels, &options->numLevels, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SerialTetrahedronTest"
PetscErrorCode SerialTetrahedronTest(const Options *options)
{
  typedef PETSC_MESH_TYPE       mesh_type;
  typedef mesh_type::sieve_type sieve_type;
  typedef mesh_type::point_type point_type;
  typedef std::pair<point_type,point_type> edge_type;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  // Create a single tetrahedron
  Obj<mesh_type>  mesh  = new mesh_type(PETSC_COMM_WORLD, 3, options->debug);
  Obj<sieve_type> sieve = new sieve_type(mesh->comm(), options->debug);
  int    cone[4]    = {1, 2, 3, 4};
  int    support[1] = {0};
  double coords[12] = {0.0, 0.0, 0.0,
                       0.0, 1.0, 0.0,
                       1.0, 0.0, 0.0,
                       0.0, 0.0, 1.0};
  double v0[3], J[9], invJ[9], detJ;
  std::map<edge_type, point_type> edge2vertex;

  if (mesh->commSize() > 1) {PetscFunctionReturn(0);}
  sieve->setChart(sieve_type::chart_type(0, 5));
  sieve->setConeSize(0, 4);
  for(int v = 1; v < 5; ++v) {sieve->setSupportSize(v, 1);}
  sieve->allocate();
  sieve->setCone(cone, 0);
  for(int v = 1; v < 5; ++v) {sieve->setSupport(v, support);}
  mesh->setSieve(sieve);
  mesh->stratify();
  ALE::SieveBuilder<mesh_type>::buildCoordinates(mesh, mesh->getDimension(), coords);
  mesh->computeElementGeometry(mesh->getRealSection("coordinates"), 0, v0, J, invJ, detJ);
  if (detJ <= 0.0) {SETERRQ1(PETSC_ERR_LIB, "Inverted element, detJ %g", detJ);}

  for(int l = 0; l < options->numLevels; ++l) {
    Obj<mesh_type>  newMesh  = new mesh_type(mesh->comm(), 3, options->debug);
    Obj<sieve_type> newSieve = new sieve_type(newMesh->comm(), options->debug);

    newMesh->setSieve(newSieve);
    ALE::MeshBuilder<mesh_type>::refineTetrahedra(*mesh, *newMesh, edge2vertex);
    edge2vertex.clear();
    if (options->debug) {
      PetscViewer viewer;

      ierr = PetscViewerCreate(PETSC_COMM_WORLD, &viewer);CHKERRQ(ierr);
      ierr = PetscViewerSetType(viewer, PETSC_VIEWER_ASCII);CHKERRQ(ierr);
      ierr = PetscViewerSetFormat(viewer, PETSC_VIEWER_ASCII_VTK);CHKERRQ(ierr);
      ierr = PetscViewerFileSetName(viewer, "refineTest1.vtk");CHKERRQ(ierr);
      ierr = VTKViewer::writeHeader(viewer);CHKERRQ(ierr);
      ierr = VTKViewer::writeVertices(newMesh, viewer);CHKERRQ(ierr);
      ierr = VTKViewer::writeElements(newMesh, viewer);CHKERRQ(ierr);
      ierr = PetscViewerDestroy(viewer);CHKERRQ(ierr);
    }
    for(int c = 0; c < pow(8, l+1); ++c) {
      newMesh->computeElementGeometry(newMesh->getRealSection("coordinates"), c, v0, J, invJ, detJ);
      if (detJ <= 0.0) {SETERRQ1(PETSC_ERR_LIB, "Inverted element, detJ %g", detJ);}
    }
    mesh = newMesh;
    newMesh = PETSC_NULL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ParallelTetrahedronTest"
PetscErrorCode ParallelTetrahedronTest(const Options *options)
{
  typedef PETSC_MESH_TYPE       mesh_type;
  typedef mesh_type::sieve_type sieve_type;
  typedef mesh_type::point_type point_type;
  typedef std::pair<point_type,point_type> edge_type;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  // Create a single tetrahedron
  Obj<mesh_type>  serialMesh  = new mesh_type(PETSC_COMM_WORLD, 3, options->debug);
  Obj<sieve_type> serialSieve = new sieve_type(serialMesh->comm(), options->debug);
  int    cone[8]    = {2, 3, 4, 5, 3, 4, 5, 6};
  int    support[2] = {0, 1};
  double coords[15] = {0.0, 0.0, 0.0,
                       0.0, 1.0, 0.0,
                       1.0, 0.0, 0.0,
                       0.0, 0.0, 1.0,
                       1.0, 1.0, 1.0};
  double v0[3], J[9], invJ[9], detJ;
  std::map<edge_type, point_type> edge2vertex;

  if (serialMesh->commSize() != 2) {PetscFunctionReturn(0);}
  if (!serialMesh->commRank()) {
    serialSieve->setChart(sieve_type::chart_type(0, 7));
    serialSieve->setConeSize(0, 4);
    serialSieve->setConeSize(1, 4);
    serialSieve->setSupportSize(2, 1);
    for(int v = 3; v < 6; ++v) {serialSieve->setSupportSize(v, 2);}
    serialSieve->setSupportSize(6, 1);
  } else {
    serialSieve->setChart(sieve_type::chart_type(0, 0));
  }
  serialSieve->allocate();
  if (!serialMesh->commRank()) {
    serialSieve->setCone(&cone[0], 0);
    serialSieve->setCone(&cone[4], 1);
    for(int v = 2; v < 6; ++v) {serialSieve->setSupport(v, support);}
    serialSieve->setSupport(6, &support[1]);
  }
  serialMesh->setSieve(serialSieve);
  serialMesh->stratify();
  ALE::SieveBuilder<mesh_type>::buildCoordinates(serialMesh, serialMesh->getDimension(), coords);
  for(int c = 0; c < (int) serialMesh->heightStratum(0)->size(); ++c) {
    serialMesh->computeElementGeometry(serialMesh->getRealSection("coordinates"), c, v0, J, invJ, detJ);
    if (detJ <= 0.0) {SETERRQ1(PETSC_ERR_LIB, "Inverted element, detJ %g", detJ);}
  }
  Obj<mesh_type>  mesh  = new mesh_type(serialMesh->comm(), serialMesh->getDimension(), options->debug);
  Obj<sieve_type> sieve = new sieve_type(mesh->comm(), options->debug);

  mesh->setSieve(sieve);
  ALE::DistributionNew<mesh_type>::distributeMeshAndSectionsV(serialMesh, mesh);
  mesh->view("Parallel Mesh");
  for(int c = 0; c < (int) mesh->heightStratum(0)->size(); ++c) {
    mesh->computeElementGeometry(mesh->getRealSection("coordinates"), c, v0, J, invJ, detJ);
    if (detJ <= 0.0) {SETERRQ1(PETSC_ERR_LIB, "Inverted element, detJ %g", detJ);}
  }

  for(int l = 0; l < options->numLevels; ++l) {
    Obj<mesh_type>  newMesh  = new mesh_type(mesh->comm(), 3, options->debug);
    Obj<sieve_type> newSieve = new sieve_type(newMesh->comm(), options->debug);

    newMesh->setSieve(newSieve);
    ALE::MeshBuilder<mesh_type>::refineTetrahedra(*mesh, *newMesh, edge2vertex);
    edge2vertex.clear();
    newMesh->view("Refined Parallel Mesh");
    // Create the parallel overlap
    size_t  nCells      = mesh->heightStratum(0)->size();
    size_t  nNewCells   = newMesh->heightStratum(0)->size();
    size_t *numCells    = new size_t[mesh->commSize()];
    size_t *numNewCells = new size_t[newMesh->commSize()];

    ierr = MPI_Allgather(&nCells, 1, MPI_INT, numCells, 1, MPI_INT, mesh->comm());CHKERRXX(ierr);
    ierr = MPI_Allgather(&nNewCells, 1, MPI_INT, numNewCells, 1, MPI_INT, newMesh->comm());CHKERRXX(ierr);
    Obj<mesh_type::send_overlap_type> newSendOverlap = newMesh->getSendOverlap();
    Obj<mesh_type::recv_overlap_type> newRecvOverlap = newMesh->getRecvOverlap();
    const Obj<mesh_type::send_overlap_type>& sendOverlap = mesh->getSendOverlap();
    const Obj<mesh_type::recv_overlap_type>& recvOverlap = mesh->getRecvOverlap();
    Obj<mesh_type::send_overlap_type::traits::capSequence> sendPoints  = sendOverlap->cap();
    const mesh_type::send_overlap_type::source_type        localOffset = numNewCells[newMesh->commRank()] - numCells[mesh->commRank()];

    for(mesh_type::send_overlap_type::traits::capSequence::iterator p_iter = sendPoints->begin(); p_iter != sendPoints->end(); ++p_iter) {
      const Obj<mesh_type::send_overlap_type::traits::supportSequence>& ranks      = sendOverlap->support(*p_iter);
      const mesh_type::send_overlap_type::source_type&                  localPoint = *p_iter;

      for(mesh_type::send_overlap_type::traits::supportSequence::iterator r_iter = ranks->begin(); r_iter != ranks->end(); ++r_iter) {
        const int                                   rank         = *r_iter;
        const mesh_type::send_overlap_type::source_type& remotePoint  = r_iter.color();
        const mesh_type::send_overlap_type::source_type  remoteOffset = numNewCells[rank] - numCells[rank];

        newSendOverlap->addArrow(localPoint+localOffset, rank, remotePoint+remoteOffset);
      }
    }
    Obj<mesh_type::recv_overlap_type::traits::baseSequence> recvPoints = recvOverlap->base();

    for(mesh_type::recv_overlap_type::traits::baseSequence::iterator p_iter = recvPoints->begin(); p_iter != recvPoints->end(); ++p_iter) {
      const Obj<mesh_type::recv_overlap_type::traits::coneSequence>& ranks      = recvOverlap->cone(*p_iter);
      const mesh_type::recv_overlap_type::target_type&               localPoint = *p_iter;

      for(mesh_type::recv_overlap_type::traits::coneSequence::iterator r_iter = ranks->begin(); r_iter != ranks->end(); ++r_iter) {
        const int                                   rank         = *r_iter;
        const mesh_type::recv_overlap_type::target_type& remotePoint  = r_iter.color();
        const mesh_type::recv_overlap_type::target_type  remoteOffset = numNewCells[rank] - numCells[rank];

        newRecvOverlap->addArrow(rank, localPoint+localOffset, remotePoint+remoteOffset);
      }
    }
    newMesh->setCalculatedOverlap(true);
    delete [] numCells;
    delete [] numNewCells;
    // Check edges in edge2vertex for both endpoints sent to same process
    //   Put it in section with point being the lowest numbered vertex and value (other endpoint, new vertex)
    // Copy across overlap
    // Merge by translating edge to local points, finding edge in edge2vertex, and adding (local new vetex, remote new vertex) to overlap
    newSendOverlap->view("Refined Send Overlap");
    newRecvOverlap->view("Refined Recv Overlap");
    if (options->debug) {
      PetscViewer viewer;

      ierr = PetscViewerCreate(PETSC_COMM_WORLD, &viewer);CHKERRQ(ierr);
      ierr = PetscViewerSetType(viewer, PETSC_VIEWER_ASCII);CHKERRQ(ierr);
      ierr = PetscViewerSetFormat(viewer, PETSC_VIEWER_ASCII_VTK);CHKERRQ(ierr);
      ierr = PetscViewerFileSetName(viewer, "refineTest1.vtk");CHKERRQ(ierr);
      ierr = VTKViewer::writeHeader(viewer);CHKERRQ(ierr);
      ierr = VTKViewer::writeVertices(newMesh, viewer);CHKERRQ(ierr);
      ierr = VTKViewer::writeElements(newMesh, viewer);CHKERRQ(ierr);
      ierr = PetscViewerDestroy(viewer);CHKERRQ(ierr);
    }
    for(int c = 0; c < pow(8, l+1); ++c) {
      newMesh->computeElementGeometry(newMesh->getRealSection("coordinates"), c, v0, J, invJ, detJ);
      if (detJ <= 0.0) {SETERRQ1(PETSC_ERR_LIB, "Inverted element, detJ %g", detJ);}
    }
    mesh = newMesh;
    newMesh = PETSC_NULL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "RunUnitTests"
PetscErrorCode RunUnitTests(const Options *options)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SerialTetrahedronTest(options);CHKERRQ(ierr);
  ierr = ParallelTetrahedronTest(options);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char *argv[])
{
  Options        options;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscInitialize(&argc, &argv, (char *) 0, help);CHKERRQ(ierr);
  ierr = ProcessOptions(PETSC_COMM_WORLD, &options);CHKERRQ(ierr);
  try {
    ierr = RunUnitTests(&options);CHKERRQ(ierr);
  } catch(ALE::Exception e) {
    std::cerr << "ERROR: " << e.msg() << std::endl;
  }
  ierr = PetscFinalize();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
