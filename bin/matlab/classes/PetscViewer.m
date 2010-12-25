classdef PetscViewer < PetscObject
  methods
    function obj = PetscViewer()
      comm = PETSC_COMM_SELF();
      [err,obj.pobj] = calllib('libpetsc', 'PetscViewerCreate', comm,0);PetscCHKERRQ(err);
    end
    function SetType(obj,name)
      err = calllib('libpetsc', 'PetscViewerSetType', obj.pobj,name);PetscCHKERRQ(err);
    end
    function View(obj,viewer)
      err = calllib('libpetsc', 'PetscViewerView', obj.pobj,viewer.pobj);PetscCHKERRQ(err);
    end
    function Destroy(obj)
      err = calllib('libpetsc', 'PetscViewerDestroy', obj.pobj);PetscCHKERRQ(err);
    end
  end
end

 
