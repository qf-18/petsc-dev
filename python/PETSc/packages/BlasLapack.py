from __future__ import generators
import user
import config.base

import os

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    self.argDB        = framework.argDB
    self.found        = 0
    self.compilers    = self.framework.require('config.compilers',     self)
    self.libraries    = self.framework.require('config.libraries',     self)
    return

  def configureHelp(self, help):
    import nargs
    help.addArgument('BLAS/LAPACK', '-with-blas-lapack-dir=<lib>', nargs.ArgDir(None, None, 'Indicate the directory containing BLAS and LAPACK libraries'))
    help.addArgument('BLAS/LAPACK', '-with-blas-lapack=<lib>', nargs.Arg(None, None, 'Indicate the library containing BLAS and LAPACK'))
    help.addArgument('BLAS/LAPACK', '-with-blas=<lib>',        nargs.Arg(None, None, 'Indicate the library containing BLAS'))
    help.addArgument('BLAS/LAPACK', '-with-lapack=<lib>',      nargs.Arg(None, None, 'Indicate the library containing LAPACK'))
    return

  def parseLibrary(self, library):
    (dir, lib)  = os.path.split(library)
    lib         = os.path.splitext(lib)[0]
    if lib.startswith('lib'): lib = lib[3:]
    return (dir, lib)

  def checkLib(self, library, blasLibrary = None):
    '''Checking for BLAS and LAPACK symbols'''
    foundBlas   = 0
    foundLapack = 0
    otherLibs   = self.compilers.flibs
    (dir, lib)  = self.parseLibrary(library)
    if blasLibrary:
      (blasDir, blasLib) = self.parseLibrary(blasLibrary)
    else:
      (blasDir, blasLib) = (dir, lib)
    # Check for BLAS
    oldLibs = self.framework.argDB['LIBS']
    if self.libraries.check(blasLib, 'ddot', libDir = blasDir, otherLibs = otherLibs, fortranMangle = 1):
      foundBlas = 1
      if blasLibrary:
        otherLibs = self.libraries.getLibArgument(blasLibrary)+' '+otherLibs
    self.framework.argDB['LIBS'] = oldLibs
    # Check for LAPACK
    oldLibs = self.framework.argDB['LIBS']
    if self.libraries.check(lib, 'dtrtrs', libDir = dir, otherLibs = otherLibs, fortranMangle = 1):
      foundLapack = 1
    self.framework.argDB['LIBS'] = oldLibs
    return (foundBlas, foundLapack)

  def generateGuesses(self):
    # Try specified BLASLAPACK library
    if 'with-blas-lapack' in self.framework.argDB:
      yield ('User specified BLAS/LAPACK library', None, self.framework.argDB['with-blas-lapack'])
    # Try specified BLAS and LAPACK libraries
    if 'with-blas' in self.framework.argDB and 'with-lapack' in self.framework.argDB:
      yield ('User specified BLAS and LAPACK libraries', self.framework.argDB['with-blas'], self.framework.argDB['with-lapack'])
    # Try specified installation root
    if 'with-blas-lapack-dir' in self.framework.argDB:
      dir = self.framework.argDB['with-blas-lapack-dir']
      yield ('User specified installation root', os.path.join(dir, 'libblas.a'), os.path.join(dir, 'liblapack.a'))
    # Try compiler defaults
    yield ('Default compiler locations', 'libblas.a', 'liblapack.a')
    # Try PETSc location
    PETSC_DIR  = None
    PETSC_ARCH = None
    if 'PETSC_DIR' in self.framework.argDB and 'PETSC_ARCH' in self.framework.argDB:
      PETSC_DIR  = self.framework.argDB['PETSC_DIR']
      PETSC_ARCH = self.framework.argDB['PETSC_ARCH']
    elif os.getenv('PETSC_DIR') and os.getenv('PETSC_ARCH'):
      PETSC_DIR  = os.getenv('PETSC_DIR')
      PETSC_ARCH = os.getenv('PETSC_ARCH')

    if PETSC_ARCH and PETSC_DIR:
      dir1 = os.path.abspath(os.path.join(PETSC_DIR, '..', 'blaslapack', 'lib'))
      yield ('PETSc location 1', os.path.join(dir1, 'libblas.a'), os.path.join(dir1, 'liblapack.a'))
      dir2 = os.path.join(dir1, 'libg_c++', PETSC_ARCH)
      yield ('PETSc location 2', os.path.join(dir2, 'libblas.a'), os.path.join(dir2, 'liblapack.a'))
      dir3 = os.path.join(dir1, 'libO_c++', PETSC_ARCH)
      yield ('PETSc location 3', os.path.join(dir3, 'libblas.a'), os.path.join(dir3, 'liblapack.a'))
    return

  def configureLibrary(self):
    functionalBlasLapack = []
    for (name, blasLibrary, lapackLibrary) in self.generateGuesses():
      self.framework.log.write('================================================================================\n')
      self.framework.log.write('Checking for a functional BLAS and LAPACK in '+name+'\n')
      (foundBlas, foundLapack) = self.executeTest(self.checkLib, [lapackLibrary, blasLibrary])
      if foundBlas:   self.foundBlas   = 1
      if foundLapack: self.foundLapack = 1
      if foundBlas and foundLapack:
        functionalBlasLapack.append((name, blasLibrary, lapackLibrary))
    # User chooses one or take first (sort by version)
    if self.foundBlas and self.foundLapack:
      name, self.blasLibrary, self.lapackLibrary = functionalBlasLapack[0]
    else:
      if not self.foundBlas:
        raise RuntimeError('Could not find a functional BLAS\n')
      if not self.foundLapack:
        raise RuntimeError('Could not find a functional LAPACK\n')
    return

  def setOutput(self):
    '''Add defines and substitutions
       - BLAS_DIR is the location of the BLAS library
       - LAPACK_DIR is the location of the LAPACK library
       - LAPACK_LIB is the LAPACK linker flags'''
    if self.foundBlas:
      dir = os.path.dirname(self.blasLibrary)
      self.addSubstitution('BLAS_DIR', dir)
      libFlag = self.libraries.getLibArgument(self.blasLibrary)
      self.addSubstitution('BLAS_LIB', libFlag)
    if self.foundLapack:
      dir = os.path.dirname(self.lapackLibrary)
      self.addSubstitution('LAPACK_DIR', dir)
      libFlag = self.libraries.getLibArgument(self.lapackLibrary)
      self.addSubstitution('LAPACK_LIB', libFlag)
    if self.foundBlas and self.foundLapack:
      blasDir   = os.path.dirname(self.blasLibrary)
      lapackDir = os.path.dirname(self.lapackLibrary)
      if blasDir == lapackDir:
        self.addSubstitution('BLASLAPACK_DIR', lapackDir)
        libFlag  = self.libraries.getLibArgument(self.lapackLibrary)
        libFlag += ' '+self.libraries.getLibArgument(os.path.basename(self.blasLibrary))
        self.addSubstitution('BLASLAPACK_LIB', libFlag)
      else:
        self.addSubstitution('BLASLAPACK_DIR', [blasDir, lapackDir])
        libFlag  = self.libraries.getLibArgument(self.lapackLibrary)
        libFlag += ' '+self.libraries.getLibArgument(self.blasLibrary)
        self.addSubstitution('BLASLAPACK_LIB', libFlag)
    return

  def configure(self):
    self.executeTest(self.configureLibrary)
    self.setOutput()
    return

if __name__ == '__main__':
  import config.framework
  import sys
  framework = config.framework.Framework(sys.argv[1:])
  framework.setupLogging()
  framework.children.append(Configure(framework))
  framework.configure()
  framework.dumpSubstitutions()
