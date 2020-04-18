#!/usr/bin/python
#
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
Responsible for generating baseline classes based on representations
in the decoder table file armv7.table.

Note: We define a notion of "actual" and "baseline" classes to
distinquish how they are used.

"baseline" instruction decoders are defined for each instruction that
appears in the manual. The "baseline" decoder captures the properties
of the instruction through the corresponding set of defined virtuals
(as defined in "inst_classes.h"). "baseline" instruction decoders are
used to test that individual instructions are decoded as expected.

"actual" instruction decoders are merged "baseline" instruction
decoders, and are used in the production instruction decoder.  The
main purpose of merging instruction decoders into corresponding actual
instruction decoders is to minimize production code size, by
minimizing the number of implementations that must be used by the
production intruction decoder.

To verify that automatically generated baseline instruction decoders
are equivalent to the hand-coded baseline instruction decoders, we
will use the current baseline to actual comparison framework. That is,
we run two tests. In the first test, we treat the hand-coded baseline
instruction decoders as the baseline, and the automatically generated
baseline instructions as actuals. In the second test, we treat the
automatically generated baseline instructions as the baseline, and the
hand-coded baseline decoders as actuals. If both tests pass, it should
show that these two implementations are identical.

Hence, once both tests pass, we can switch the automatically generated
baseline instruction decoders in as the 'standard' baseline and delete
the hand-coded baseline instruction decoders.
"""

import dgen_core
import dgen_decoder
import dgen_output

# Holds the sorted list of installed baseline decoders.
BASELINE_DECODERS = None

# Holds the map from a baseline decoder name, to the corresponding
# (sorted) list of baseline decoders with that name. Used to figure
# out which baseline decoders have the same name, and add a suffix
# that makes the name unique.
BASELINE_NAME_TO_BASELINES_MAP = None

# Holds the map from the baseline, to the unique name we will use for
# it.
BASELINE_TO_NAME_MAP = None

# Holds the decoder value name that baseline decoder information is
# stored on.
_BASELINE_DECODERS = 'baseline-decoders'

"""Defines the decoder action fields that hold names of instruction decoders.
   Such fields must be removed from baselines, so that the baselines remain
   neutral for other instruction decoder naming conventions."""
_DECODER_ACTION_DECODER_FIELD_NAMES = ['actual', 'baseline']

def RemoveDecoderActionDecoderFieldNames(action):
  """Returns a copy of the given decoder action, with fields that name
     the instruction decoder to use removed. Neutralizes the corresponding
     baseline action."""
  action = action.copy()
  for field in _DECODER_ACTION_DECODER_FIELD_NAMES:
    action.remove(field)
  action.freeze()
  return action

def GetBaselineDecoders(decoder):
  """Takes the given decoder table, and builds the corresponding
     internal maps, so that we can consistently name baseline classes.
     Returns the (sorted) list of baseline classes to build.
     """

  # See if aready defined, and update accordingly.
  maps = decoder.get_value(_BASELINE_DECODERS)
  if maps:
    _ReinstallBaselineMaps(maps)
  else:
    _BuildBaselineMaps(decoder)
  return BASELINE_DECODERS

def _RebuildBaslineDecodersIfAlreadyDefined(decoder):
  """Rebuilds the baseline decoders map if already defined.  Should
     be called whenever decoder actions are changed."""
  if decoder.get_value(_BASELINE_DECODERS):
    _BuildBaselineMaps

dgen_decoder.REBUILD_DECODER_CACHE_FUNCTIONS.add(
    _RebuildBaslineDecodersIfAlreadyDefined)

def _BuildBaselineMaps(decoder):
  """Takes the given decoder table, and builds the corresponding
     internal maps, so that we can consistently name baseline classees.
     """
  global BASELINE_DECODERS
  global BASELINE_NAME_TO_BASELINES_MAP
  global BASELINE_TO_NAME_MAP
  BASELINE_NAME_TO_BASELINES_MAP = {}
  BASELINE_TO_NAME_MAP = {}

  baselines = set()

  # Get the list of decoder actions defined for the decoder table, and
  # install them into the global tables.
  for baseline in decoder.decoders():
    if not dgen_decoder.ActionDefinesDecoder(baseline): continue

    simp_baseline = RemoveDecoderActionDecoderFieldNames(baseline)

    baselines.add(baseline)
    _AddBaselineToBaselineNameToBaselinesMap(simp_baseline)

  _FixBaselineNameToBaselinesMap()
  _DefineBaselineNames()
  BASELINE_DECODERS = sorted(baselines, key=BaselineName)
  decoder.define_value(_BASELINE_DECODERS,
                       (BASELINE_DECODERS,
                        BASELINE_NAME_TO_BASELINES_MAP,
                        BASELINE_TO_NAME_MAP))

def _ReinstallBaselineMaps(maps):
  assert len(maps) == 3
  global BASELINE_DECODERS
  global BASELINE_NAME_TO_BASELINES_MAP
  global BASELINE_TO_NAME_MAP

  BASELINE_DECODERS = maps[0]
  BASELINE_NAME_TO_BASELINES_MAP = maps[1]
  BASELINE_TO_NAME_MAP = maps[2]

BASELINE_BASE_H_HEADER="""%(FILE_HEADER)s
#ifndef %(IFDEF_NAME)s
#define %(IFDEF_NAME)s

// Note: baselines are spread out over multiple files so that
// they are small enough to be handled by the Rietveld server.
"""

BASELINE_BASE_INCLUDE=(
"""#include "%(FILEBASE)s_%(filename_index)s.h"
""")

BASELINE_BASE_H_FOOTER="""
#endif  // %(IFDEF_NAME)s
"""

def generate_baselines_base_h(decoder, decoder_name,
                               filename, out, cl_args):
  """Generates baseline decoder C++ declarations in the given file.

    Args:
        decoder: The decoder tables.
        decoder_name: The name of the decoder state to build.
        filename: The (localized) name for the .h file.
        out: a COutput object to write to.
        cl_args: A dictionary of additional command line arguments.
        """

  separators = cl_args['auto-baseline-sep']
  num_blocks = dgen_output.GetNumberCodeBlocks(separators)

  assert filename.endswith('baselines.h')

  values = {
      'FILE_HEADER': dgen_output.HEADER_BOILERPLATE,
      'IFDEF_NAME' : dgen_output.ifdef_name(filename),
      'FILEBASE'   : filename[:-len('.h')],
      'decoder_name': decoder_name,
      }
  out.write(BASELINE_BASE_H_HEADER % values)
  for block in range(1, num_blocks+1):
    values['filename_index'] = block
    out.write(BASELINE_BASE_INCLUDE % values)
  out.write(BASELINE_BASE_H_FOOTER % values)

BASELINE_H_HEADER="""%(FILE_HEADER)s
#ifndef %(IFDEF_NAME)s
#define %(IFDEF_NAME)s

#include "native_client/src/trusted/validator_arm/arm_helpers.h"
#include "native_client/src/trusted/validator_arm/inst_classes.h"

namespace nacl_arm_dec {
"""

BASELINE_H_FOOTER="""
} // namespace nacl_arm_test

#endif  // %(IFDEF_NAME)s
"""

def generate_baselines_h(decoder, decoder_name,
                         filename, out, cl_args):
  """Generates baseline decoder C++ declarations in the given file.

    Args:
        decoder: The decoder tables.
        decoder_name: The name of the decoder state to build.
        filename: The (localized) name for the .h file.
        out: a COutput object to write to.
        cl_args: A dictionary of additional command line arguments.
        """
  separators = cl_args['auto-baseline-sep']
  num_blocks = dgen_output.GetNumberCodeBlocks(separators)

  # Find block to print
  block = dgen_output.FindBlockIndex(filename, 'baselines_%s.h', num_blocks)

  values = {
      'FILE_HEADER': dgen_output.HEADER_BOILERPLATE,
      'IFDEF_NAME' : dgen_output.ifdef_name(filename),
      'decoder_name': decoder_name,
      }
  out.write(BASELINE_H_HEADER % values)
  _print_baseline_headers(
      GetBaselineDecodersBlock(decoder, block, separators), out)
  out.write(BASELINE_H_FOOTER % values)

BASELINE_CC_HEADER="""%(FILE_HEADER)s
#include "native_client/src/trusted/validator_arm/gen/arm32_decode_baselines.h"
#include "native_client/src/trusted/validator_arm/inst_classes_inline.h"

namespace nacl_arm_dec {
"""

BASELINE_CC_FOOTER="""
}  // namespace nacl_arm_dec
"""

def generate_baselines_cc(decoder, decoder_name, filename,
                          out, cl_args):
  """Generates the baseline decoder C++ definitions in the given file.

    Args:
        decoder: The decoder tables.
        decoder_name: The name of the decoder state to build.
        filename: The (localized) name for the .h file.
        out: a COutput object to write to.
        cl_args: A dictionary of additional command line arguments.
        """
  separators = cl_args['auto-baseline-sep']
  num_blocks = dgen_output.GetNumberCodeBlocks(separators)

  # Find block to print
  block = dgen_output.FindBlockIndex(filename, 'baselines_%s.cc', num_blocks)

  values = {
      'FILE_HEADER': dgen_output.HEADER_BOILERPLATE,
      'decoder_name': decoder_name,
      }

  out.write(BASELINE_CC_HEADER % values)
  _print_baseline_classes(
      GetBaselineDecodersBlock(decoder, block, separators), out)
  out.write(BASELINE_CC_FOOTER % values)

BASELINE_CLASS_HEADER="""
// %(decoder_name)s:
//
//  %(baseline_rep)s"""

def _print_baseline_headers(baselines, out):
  """Generates C++ class declarations for each of the given baseline
     decoders."""
  for baseline in baselines:
    baseline_name = BaselineName(baseline)
    values = {
        'decoder_name': baseline_name,
        'baseline_rep': dgen_decoder.commented_decoder_repr(baseline)
        }
    out.write(BASELINE_CLASS_HEADER % values)
    dgen_decoder.DeclareDecoder(baseline, baseline_name, out)

def _print_baseline_classes(baselines, out):
  """Generates C++ class definitions for each of the given baseline
     decoders."""
  for baseline in baselines:
    baseline_name = BaselineName(baseline)
    values = {
        'decoder_name': baseline_name,
        'baseline_rep': dgen_decoder.commented_decoder_repr(baseline),
        }
    out.write(BASELINE_CLASS_HEADER % values)
    dgen_decoder.DefineDecoder(baseline, baseline_name, out)

def _AddBaselineToBaselineNameToBaselinesMap(baseline):
  """Add entry into BASELINE_NAME_TO_BASELINES_MAP for the
     given baseline class."""
  baseline_name = dgen_decoder.BaselineName(baseline)
  bases = BASELINE_NAME_TO_BASELINES_MAP.get(baseline_name)
  if bases == None:
    bases = set()
    BASELINE_NAME_TO_BASELINES_MAP[baseline_name] = bases
  bases.add(baseline)

def _FixBaselineNameToBaselinesMap():
  """Replaces the sets in BASELINE_NAME_TO_BASELINES_MAP with
     corresponding sorted lists."""
  for baseline_name in BASELINE_NAME_TO_BASELINES_MAP.keys():
    BASELINE_NAME_TO_BASELINES_MAP[baseline_name] = sorted(
        BASELINE_NAME_TO_BASELINES_MAP[baseline_name])

def _DefineBaselineNames():
  """Installs a unique name for each baseline, based on the baseline decoders
     with the same (dgen_decoder) baseline name.
     """
  global BASELINE_TO_NAME_MAP

  name_map = {}
  for (name, bases) in BASELINE_NAME_TO_BASELINES_MAP.items():
    count = 0
    for base in bases:
      BASELINE_TO_NAME_MAP[base] = "%s_case_%s" % (name, count)
      count += 1

def BaselineName(baseline):
  """Returns the name to use for the baseline."""
  simp_baseline = RemoveDecoderActionDecoderFieldNames(baseline)
  return BASELINE_TO_NAME_MAP[simp_baseline]

def GetBaselineDecodersBlock(decoder, n, separators):
  """Returns the (sorted) list of baseline class to include
     in block n, assuming baseline classes are split using
     the list of separators."""
  return dgen_output.GetDecodersBlock(n, separators,
                                      GetBaselineDecoders(decoder),
                                      BaselineName)

def AddBaselinesToDecoder(decoder, tables=None):
  """Adds the automatically generated baseline decoders (in files
     "*_baselines.h" and "*_baselines.cc" into the listed tables
     of the decoder, and returns the generated (new) decoder.
     """
  if tables == None:
    tables = decoder.table_names()
  elif tables == []:
    return decoder
  GetBaselineDecoders(decoder)

  # Update actions in tables.
  decoder = decoder.table_filter(
      lambda tbl: _AddBaselineToTable(tbl) if tbl.name in tables
      else tbl.copy())

  # Update other defined actions associated with the decoder.
  for key in decoder.value_keys():
    action = decoder.get_value(key)
    if isinstance(action, dgen_core.DecoderAction):
      decoder.define_value(key, _AddBaselineToAction(action))

  dgen_decoder.RebuildDecoderCaches(decoder)

  return decoder

def _AddBaselineToTable(table):
  """Generates a copy of the given table, where the 'baseline'
     field is defined by the corresponding automatically generated
     baseline decoder (described in the table)."""
  return table.row_filter(_AddBaselineToRow)

def _AddBaselineToRow(row):
  """Generates a copy of the given row, where (if applicable),
     the field 'baseline' is defined by the corresponding
     (baseline) instruction decoder described by the action
     of the row."""
  action = row.action.copy()
  return dgen_core.Row(list(row.patterns), _AddBaselineToAction(action))

def _AddBaselineToAction(action):
  action = action.copy()
  if (isinstance(action, dgen_core.DecoderAction) and
      dgen_decoder.ActionDefinesDecoder(action)):
    action.define('baseline', BaselineName(action))
  return action
