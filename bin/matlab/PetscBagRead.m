function bag = PetscBagRead(fd)
%
%  Reads in PETSc binary file bag object
%  emits as Matlab struct.  Called from
%  PetscBinaryRead.m.
%

petsc_dir = GetPetscDir
[name_len help_len] = ParsePetscBagDotH(petsc_dir)

bagsizecount = fread(fd,2,'int32');
count        = bagsizecount(2);

bag.bag_name = deblank(char(fread(fd,name_len,'uchar')'));
bag.bag_help = deblank(char(fread(fd,help_len,'uchar')'));

for lcv = 1:count
  offsetdtype = fread(fd,2,'int32');
  dtype = offsetdtype(2);
  name  = deblank(char(fread(fd,name_len,'uchar')'));
  help  = deblank(char(fread(fd,help_len,'uchar')'));
  msize = fread(fd,1,'int32');

  if dtype == 0     % integer
    val = fread(fd,1,'int32');
  elseif dtype == 1 % double
    val = fread(fd,1,'double');
  elseif dtype == 6 % char
    val = deblank(char(fread(fd,msize,'uchar')'));
  elseif dtype == 7 % boolean
    val = fread(fd,1,'bit1');
  else 
    val = [];
    warning('Bag entry %s could not be read',name);
  end 
  bag = setfield(bag,name,val);
  bag = setfield(bag,[name,'_help'],help);
end

fclose(fd);
return

% ---------------------------------------------------- %

function dir = GetPetscDir
   
   dir = getenv('PETSC_DIR');
   if length(dir)==0
      error(['Please set environment variable PETSC_DIR' ...
	    ' and try again.'])
   end
   return
   
function [n h] = ParsePetscBagDotH(dir)
   
   petscbagh = [dir,'/include/petscbag.h'];
   fid = fopen(petscbagh,'rt');
   if (fid<0)
      errstr = sprintf('Could not open %s.',petscbagh);
      error(errstr);
   end
   
   nametag = '#define PETSC_BAG_NAME_LENGTH';
   helptag = '#define PETSC_BAG_HELP_LENGTH';
   n = 0; h = 0;
   while ~feof(fid)
      lin = fgetl(fid);
      ni = strfind(lin,nametag);
      nh = strfind(lin,helptag);
      if ni
	 n = lin(ni+length(nametag)+1:end);
      elseif nh
	 h = lin(nh+length(helptag)+1:end);
      end   
      if (n>0 && h>0) break; end;
   end
   if (n==0 || h==0)
      errstr = sprintf('Could not parse %s.',petscbagh);
      error(errstr);
   end
   fclose(fid);
   return
