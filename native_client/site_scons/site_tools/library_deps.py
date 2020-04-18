# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Harness for defining library dependencies for scons files."""


# The following is a map from a library, to the corresponding
# list of dependent libraries that must be included after that library, in
# the list of libraries.
LIBRARY_DEPENDENCIES_DEFAULT = {
    'arm_validator_core': [
        'cpu_features',
        ],
    'validation_cache': [
        'platform',
        ],
    'debug_stub': [
        'sel',
        ],
    'imc': [
        'platform',
        ],
    'platform': [
        'gio',
        ],
    'nacl_base': [
        'platform',
        ],
    'nrd_xfer': [
        'nacl_base',
        'imc',
        'platform',
        ],
    'platform_qual_lib': [
        'cpu_features',
        ],
    'sel': [
        'nacl_error_code',
        'env_cleanser',
        'nrd_xfer',
        'nacl_perf_counter',
        'nacl_base',
        'imc',
        'nacl_fault_inject',
        'nacl_interval',
        'platform',
        'platform_qual_lib',
        'gio',
        'validation_cache',
        'validators',
        ],
    'sel_main_chrome': [
        'sel',
        'debug_stub',
        ],
    'sel_main': [
        'sel',
        'debug_stub',
        ],
    'serialization': [
        'platform',
        ],
    'testrunner_browser': [
        'ppapi',
        ],
    'validation_cache': [
        # For CHECK(...)
        'platform',
        ],
    'irt_support_private': [
        'platform',
        ],
    'pnacl_dynloader': [
        'platform',
        ],
    'pll_loader_lib': [
        'platform',
        'pnacl_dynloader',
        ],
    }

# Untrusted only library dependencies.
# Include names here that otherwise clash with trusted names.
UNTRUSTED_LIBRARY_DEPENDENCIES = {
    'ppapi_cpp': [
        'ppapi',
        ],
    }

# Platform specific library dependencies. Mapping from a platform,
# to a map from a library, to the corresponding list of dependendent
# libraries that must be included after that library, in the list
# of libraries.
PLATFORM_LIBRARY_DEPENDENCIES = {
    'x86-32': {
        'dfa_validate_caller_x86_32': [
            'cpu_features',
            'validation_cache',
            'nccopy_x86_32',
            ],
        },
    'x86-64': {
        'dfa_validate_caller_x86_64': [
            'cpu_features',
            'validation_cache',
            'nccopy_x86_64',
            ],
        },
    'arm': {
        'ncvalidate_arm_v2': [
            'arm_validator_core',
            'validation_cache',
            ],
        'validators': [
            'ncvalidate_arm_v2',
            ],
        },
    'mips32': {
        'ncvalidate_mips': [
            'mips_validator_core',
            'cpu_features',
            ],
        'validators': [
            'ncvalidate_mips',
            ],
        },
    }


def AddLibDeps(env, platform, libraries):
  """ Adds dependent libraries to list of libraries.

  Computes the transitive closure of library dependencies for each library
  in the given list. Dependent libraries are added after libraries
  as defined in LIBRARY_DEPENDENCIES, unless there is a cycle. If
  a cycle occurs, it is broken and the remaining (acyclic) graph
  is used. Also removes duplicate library entries.

  Note: Keeps libraries (in same order) as given
  in the argument list. This includes duplicates if specified.
  """
  visited = set()                    # Nodes already visited
  closure = []                       # Collected closure

  # If library A depends on library B, B must appear in the link line
  # after A.  This is why we reverse the list and reverse it back
  # again later.
  def VisitList(libraries):
    for library in reversed(libraries):
      if library not in visited:
        VisitLibrary(library)

  def GetLibraryDeps(library):
    ret = (LIBRARY_DEPENDENCIES_DEFAULT.get(library, []) +
        PLATFORM_LIBRARY_DEPENDENCIES.get(platform, {}).get(library, []))
    if env['NACL_BUILD_FAMILY'] != 'TRUSTED':
      ret.extend(UNTRUSTED_LIBRARY_DEPENDENCIES.get(library, []))
    if library == 'validators' and env.Bit('build_x86'):
      ret.append(env.NaClTargetArchSuffix('dfa_validate_caller'))
    return ret

  def VisitLibrary(library):
    visited.add(library)
    VisitList(GetLibraryDeps(library))
    closure.append(library)

  # Ideally we would just do "VisitList(libraries)" here, but some
  # PPAPI tests (specifically, tests/ppapi_gles_book) list "ppapi_cpp"
  # twice in the link line, and we need to maintain these duplicates.
  for library in reversed(libraries):
    VisitLibrary(library)

  closure.reverse()
  return closure
