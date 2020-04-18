# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module handles the JSON de/serialization of the core classes.

This is needed for both long term storage (e.g., loading/storing traces to local
files) and for short term data exchange (AJAX with the HTML UI).

The rationale of these serializers is to store data in an efficient (i.e. avoid
to store redundant information) and intelligible (i.e. flatten the classes
hierarchy keeping only the meaningful bits) format.
"""

import json

from memory_inspector.classification import results
from memory_inspector.core import backends
from memory_inspector.core import memory_map
from memory_inspector.core import native_heap
from memory_inspector.core import stacktrace
from memory_inspector.core import symbol


class Encoder(json.JSONEncoder):
  def default(self, obj):  # pylint: disable=E0202
    if isinstance(obj, memory_map.Map):
      return [entry.__dict__ for entry in obj.entries]

    if isinstance(obj, symbol.Symbols):
      return obj.symbols

    if isinstance(obj, (symbol.Symbol, symbol.SourceInfo)):
      return obj.__dict__

    if isinstance(obj, native_heap.NativeHeap):
      # Just keep the list of (distinct) stack frames from the index. Encoding
      # it as a JSON dictionary would be redundant.
      return {'stack_frames': obj.stack_frames.values(),
              'allocations': obj.allocations}

    if isinstance(obj, native_heap.Allocation):
      return obj.__dict__

    if isinstance(obj, stacktrace.Stacktrace):
      # Keep just absolute addrs of stack frames. The full frame details will be
      # kept in (and rebuilt from) |native_heap.NativeHeap.stack_frames|. See
      # NativeHeapDecoder below.
      return [frame.address for frame in obj.frames]

    if isinstance(obj, stacktrace.Frame):
      # Strip out the symbol information from stack frames. Symbols are stored
      # (and will be loaded) separately. Rationale: different heap snapshots can
      # share the same symbol db. Serializing the symbol information for each
      # stack frame for each heap snapshot is a waste.
      return {'address': obj.address,
              'exec_file_rel_path': obj.exec_file_rel_path,
              'offset': obj.offset}

    if isinstance(obj, (backends.DeviceStats, backends.ProcessStats)):
      return obj.__dict__

    if isinstance(obj, results.AggreatedResults):
      return {'keys': obj.keys, 'buckets': obj.total}

    if isinstance(obj, results.Bucket):
      return {obj.rule.name : {'values': obj.values, 'children': obj.children}}

    return json.JSONEncoder.default(self, obj)


class MmapDecoder(json.JSONDecoder):
  def decode(self, json_str):  # pylint: disable=W0221
    d = super(MmapDecoder, self).decode(json_str)
    mmap = memory_map.Map()
    for entry_dict in d:
      entry = memory_map.MapEntry(**entry_dict)
      mmap.Add(entry)
    return mmap


class SymbolsDecoder(json.JSONDecoder):
  def decode(self, json_str):  # pylint: disable=W0221
    d = super(SymbolsDecoder, self).decode(json_str)
    symbols = symbol.Symbols()
    for sym_key, sym_dict in d.iteritems():
      sym = symbol.Symbol(sym_dict['name'])
      for source_info in sym_dict['source_info']:
        sym.AddSourceLineInfo(**source_info)
      symbols.symbols[sym_key] = sym
    return symbols


class NativeHeapDecoder(json.JSONDecoder):
  def decode(self, json_str):  # pylint: disable=W0221
    d = super(NativeHeapDecoder, self).decode(json_str)
    nh = native_heap.NativeHeap()
    # First load and rebuild the stack_frame index.
    for frame_dict in d['stack_frames']:
      frame = nh.GetStackFrame(frame_dict['address'])
      frame.SetExecFileInfo(frame_dict['exec_file_rel_path'],
                            frame_dict['offset'])
    # Then load backtraces (reusing stack frames from the index above).
    for alloc_dict in d['allocations']:
      stack_trace = stacktrace.Stacktrace()
      for absolute_addr in alloc_dict['stack_trace']:
        stack_trace.Add(nh.GetStackFrame(absolute_addr))
      nh.Add(native_heap.Allocation(start=alloc_dict['start'],
                                    size=alloc_dict['size'],
                                    stack_trace=stack_trace,
                                    flags=alloc_dict['flags'],
                                    resident_size=alloc_dict['resident_size']))
    return nh