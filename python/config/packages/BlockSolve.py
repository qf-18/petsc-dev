import config.base

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def setOutput(self):
    #self.addDefine('HAVE_BLOCKSOLVE', 0)
    self.addSubstitution('BLOCKSOLVE_INCLUDE', '', 'The BlockSolve include flags')
    self.addSubstitution('BLOCKSOLVE_LIB', '', 'The BlockSolve library flags')
    return

  def configure(self):
    self.setOutput()
    return
