import os
DIR_OF_THIS_SCRIPT = os.path.abspath( os.path.dirname( __file__ ) )

def findDatabase(filename):
  databases = {
    'lowlevel':  'stage_lowlevel-prefix/src/stage_lowlevel-build',
    'stdlibs':   'stage_stdlibs-prefix/src/stage_stdlibs-build',
    'userspace': 'stage_userspace-prefix/src/stage_userspace-build',
    'initramfs': 'stage_initramfs-prefix/src/stage_initramfs-build', }

  paths = {
    'lowlevel':  [
      'src/loader',
      'src/kernel'
    ],
    'stdlibs':   [
      'src/llvm/libcxx',
      'src/llvm/libcxxabi',
      'src/llvm/compiler-rt',
      'src/userspace/lib9p/lib',
      'src/newlib'
    ],
    'userspace': [
      'src/userspace'
    ],
    'initramfs': [
      'src/userspace/lib9p/example/server'
    ],
  }

  relPath = os.path.relpath(filename)

  retDB = None
  spec = 0

  for db in paths:
    for path in paths[db]:
      if relPath.startswith(path + '/'):
        if len(path) > spec:
          spec = len(path)
          retDB = db
  
  return databases[retDB]

def readSysrootLocation(builddir):
  f = open(builddir + '/CMakeCache.txt')
  for line in f:
    if line.startswith('lf_os_sysroot:'):
      return line.split('=')[1][:-1]

def Settings( **kwargs ):
  if kwargs['language'] == 'cfamily':
    filename   = kwargs['filename']
    db         = findDatabase(filename)
    buildDir   = os.path.join(DIR_OF_THIS_SCRIPT, 'build')
    sysroot    = readSysrootLocation(buildDir)
    fullDBPath = os.path.join(buildDir, db)

    if os.path.exists( fullDBPath ):
      return {
        'ls': {
          'compilationDatabasePath': fullDBPath
        }
      }
