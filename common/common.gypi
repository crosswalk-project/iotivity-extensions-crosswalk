{
  'variables': {
    'extension_build_type%': '<(extension_build_type)',
    'extension_build_type%': 'Debug',
  },
  'target_defaults': {
    'conditions': [
      ['extension_build_type== "Debug"', {
        'defines': ['_DEBUG', ],
        'cflags': [ '-O0', '-g', ],
      }],
      ['extension_build_type == "Release"', {
        'defines': ['NDEBUG', ],
        'cflags': [
          '-O2',
          # Don't emit the GCC version ident directives, they just end up
          # in the .comment section taking up binary size.
          '-fno-ident',
          # Put data and code in their own sections, so that unused symbols
          # can be removed at link time with --gc-sections.
          '-fdata-sections',
          '-ffunction-sections',
        ],
      }],
    ],
    'includes': [
      'xwalk_js2c.gypi',
    ],
    'include_dirs': [
      '../',
      '<(SHARED_INTERMEDIATE_DIR)',
    ],
    'sources': [
      'extension.cc',
      'extension.h',
      'picojson.h',
      'scope_exit.h',
      'utils.h',
      'XW_Extension.h',
      'XW_Extension_EntryPoints.h',
      'XW_Extension_Permissions.h',
      'XW_Extension_Runtime.h',
      'XW_Extension_SyncMessage.h',
    ],
    'cflags': [
      '-std=c++0x',
      '-fPIC',
      '-fvisibility=hidden',
    ],
  },
}
