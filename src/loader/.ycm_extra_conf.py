import os
DIR_OF_THIS_SCRIPT = os.path.abspath( os.path.dirname( __file__ ) )

def Settings( **kwargs ):
  cmake_commands = os.path.join( DIR_OF_THIS_SCRIPT, '../../build/stage_lowlevel-prefix/src/stage_lowlevel-build/' )
  if os.path.exists( cmake_commands ):
    return {
      'ls': {
        'compilationDatabasePath': os.path.dirname( cmake_commands )
      }
    }
