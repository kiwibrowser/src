#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""pprof_proto_generator.py: read perf.data, generate pprof.profile, which can be
    used by pprof.

  Example:
    python app_profiler.py
    python pprof_proto_generator.py
    pprof -text pprof.profile
"""

from __future__ import print_function
import argparse
import os
import os.path
import re
import shutil
import sys
import time

from annotate import Addr2Line
from simpleperf_report_lib import *
from utils import *

try:
    import google.protobuf
except:
    log_exit('google.protobuf module is missing. Please install it first.')

import profile_pb2

def load_pprof_profile(filename):
    profile = profile_pb2.Profile()
    with open(filename, "rb") as f:
        profile.ParseFromString(f.read())
    return profile


def store_pprof_profile(filename, profile):
    with open(filename, 'wb') as f:
        f.write(profile.SerializeToString())


class PprofProfilePrinter(object):

    def __init__(self, profile):
        self.profile = profile
        self.string_table = profile.string_table

    def show(self):
        p = self.profile
        sub_space = '  '
        print('Profile {')
        print('%d sample_types' % len(p.sample_type))
        for i in range(len(p.sample_type)):
            print('sample_type[%d] = ' % i, end='')
            self.show_value_type(p.sample_type[i])
        print('%d samples' % len(p.sample))
        for i in range(len(p.sample)):
            print('sample[%d]:' % i)
            self.show_sample(p.sample[i], sub_space)
        print('%d mappings' % len(p.mapping))
        for i in range(len(p.mapping)):
            print('mapping[%d]:' % i)
            self.show_mapping(p.mapping[i], sub_space)
        print('%d locations' % len(p.location))
        for i in range(len(p.location)):
            print('location[%d]:' % i)
            self.show_location(p.location[i], sub_space)
        for i in range(len(p.function)):
            print('function[%d]:' % i)
            self.show_function(p.function[i], sub_space)
        print('%d strings' % len(p.string_table))
        for i in range(len(p.string_table)):
            print('string[%d]: %s' % (i, p.string_table[i]))
        print('drop_frames: %s' % self.string(p.drop_frames))
        print('keep_frames: %s' % self.string(p.keep_frames))
        print('time_nanos: %u' % p.time_nanos)
        print('duration_nanos: %u' % p.duration_nanos)
        print('period_type: ', end='')
        self.show_value_type(p.period_type)
        print('period: %u' % p.period)
        for i in range(len(p.comment)):
            print('comment[%d] = %s' % (i, self.string(p.comment[i])))
        print('default_sample_type: %d' % p.default_sample_type)
        print('} // Profile')
        print()

    def show_value_type(self, value_type, space=''):
        print('%sValueType(typeID=%d, unitID=%d, type=%s, unit=%s)' %
              (space, value_type.type, value_type.unit,
               self.string(value_type.type), self.string(value_type.unit)))

    def show_sample(self, sample, space=''):
        sub_space = space + '  '
        for i in range(len(sample.location_id)):
            print('%slocation_id[%d]: id %d' % (space, i, sample.location_id[i]))
            self.show_location_id(sample.location_id[i], sub_space)
        for i in range(len(sample.value)):
            print('%svalue[%d] = %d' % (space, i, sample.value[i]))
        for i in range(len(sample.label)):
            print('%slabel[%d] = ', (space, i))

    def show_location_id(self, location_id, space=''):
        location = self.profile.location[location_id - 1]
        self.show_location(location, space)

    def show_location(self, location, space=''):
        sub_space = space + '  '
        print('%sid: %d' % (space, location.id))
        print('%smapping_id: %d' % (space, location.mapping_id))
        self.show_mapping_id(location.mapping_id, sub_space)
        print('%saddress: %x' % (space, location.address))
        for i in range(len(location.line)):
            print('%sline[%d]:' % (space, i))
            self.show_line(location.line[i], sub_space)

    def show_mapping_id(self, mapping_id, space=''):
        mapping = self.profile.mapping[mapping_id - 1]
        self.show_mapping(mapping, space)

    def show_mapping(self, mapping, space=''):
        print('%sid: %d' % (space, mapping.id))
        print('%smemory_start: %x' % (space, mapping.memory_start))
        print('%smemory_limit: %x' % (space, mapping.memory_limit))
        print('%sfile_offset: %x' % (space, mapping.file_offset))
        print('%sfilename: %s(%d)' % (space, self.string(mapping.filename),
                                      mapping.filename))
        print('%sbuild_id: %s(%d)' % (space, self.string(mapping.build_id),
                                      mapping.build_id))
        print('%shas_functions: %s' % (space, mapping.has_functions))
        print('%shas_filenames: %s' % (space, mapping.has_filenames))
        print('%shas_line_numbers: %s' % (space, mapping.has_line_numbers))
        print('%shas_inline_frames: %s' % (space, mapping.has_inline_frames))

    def show_line(self, line, space=''):
        sub_space = space + '  '
        print('%sfunction_id: %d' % (space, line.function_id))
        self.show_function_id(line.function_id, sub_space)
        print('%sline: %d' % (space, line.line))

    def show_function_id(self, function_id, space=''):
        function = self.profile.function[function_id - 1]
        self.show_function(function, space)

    def show_function(self, function, space=''):
        print('%sid: %d' % (space, function.id))
        print('%sname: %s' % (space, self.string(function.name)))
        print('%ssystem_name: %s' % (space, self.string(function.system_name)))
        print('%sfilename: %s' % (space, self.string(function.filename)))
        print('%sstart_line: %d' % (space, function.start_line))

    def show_label(self, label, space=''):
        print('%sLabel(%s =', space, self.string(label.key), end='')
        if label.HasField('str'):
            print('%s)' % self.get_string(label.str))
        else:
            print('%d)' % label.num)

    def string(self, id):
        return self.string_table[id]


class Sample(object):

    def __init__(self):
        self.location_ids = []
        self.values = {}

    def add_location_id(self, location_id):
        self.location_ids.append(location_id)

    def add_value(self, id, value):
        self.values[id] = self.values.get(id, 0) + value

    def add_values(self, values):
        for id in values.keys():
            value = values[id]
            self.add_value(id, value)

    @property
    def key(self):
        return tuple(self.location_ids)


class Location(object):

    def __init__(self, mapping_id, address, vaddr_in_dso):
        self.id = -1  # unset
        self.mapping_id = mapping_id
        self.address = address
        self.vaddr_in_dso = vaddr_in_dso
        self.lines = []

    @property
    def key(self):
        return (self.mapping_id, self.address)


class Line(object):

    def __init__(self):
        self.function_id = 0
        self.line = 0


class Mapping(object):

    def __init__(self, start, end, pgoff, filename_id, build_id_id):
        self.id = -1  # unset
        self.memory_start = start
        self.memory_limit = end
        self.file_offset = pgoff
        self.filename_id = filename_id
        self.build_id_id = build_id_id

    @property
    def key(self):
        return (
            self.memory_start,
            self.memory_limit,
            self.file_offset,
            self.filename_id,
            self.build_id_id)


class Function(object):

    def __init__(self, name_id, dso_name_id, vaddr_in_dso):
        self.id = -1  # unset
        self.name_id = name_id
        self.dso_name_id = dso_name_id
        self.vaddr_in_dso = vaddr_in_dso
        self.source_filename_id = 0
        self.start_line = 0

    @property
    def key(self):
        return (self.name_id, self.dso_name_id)


class PprofProfileGenerator(object):

    def __init__(self, config):
        self.config = config
        self.lib = ReportLib()

        config['binary_cache_dir'] = 'binary_cache'
        if not os.path.isdir(config['binary_cache_dir']):
            config['binary_cache_dir'] = None
        else:
            self.lib.SetSymfs(config['binary_cache_dir'])
        if config.get('perf_data_path'):
            self.lib.SetRecordFile(config['perf_data_path'])
        kallsyms = 'binary_cache/kallsyms'
        if os.path.isfile(kallsyms):
            self.lib.SetKallsymsFile(kallsyms)
        self.comm_filter = set(config['comm_filters']) if config.get('comm_filters') else None
        if config.get('pid_filters'):
            self.pid_filter = {int(x) for x in config['pid_filters']}
        else:
            self.pid_filter = None
        if config.get('tid_filters'):
            self.tid_filter = {int(x) for x in config['tid_filters']}
        else:
            self.tid_filter = None
        self.dso_filter = set(config['dso_filters']) if config.get('dso_filters') else None

    def gen(self):
        self.profile = profile_pb2.Profile()
        self.profile.string_table.append('')
        self.string_table = {}
        self.sample_types = {}
        self.sample_map = {}
        self.sample_list = []
        self.location_map = {}
        self.location_list = []
        self.mapping_map = {}
        self.mapping_list = []
        self.function_map = {}
        self.function_list = []

        # 1. Process all samples in perf.data, aggregate samples.
        while True:
            report_sample = self.lib.GetNextSample()
            if report_sample is None:
                self.lib.Close()
                break
            event = self.lib.GetEventOfCurrentSample()
            symbol = self.lib.GetSymbolOfCurrentSample()
            callchain = self.lib.GetCallChainOfCurrentSample()

            if not self._filter_report_sample(report_sample):
                continue

            sample_type_id = self.get_sample_type_id(event.name)
            sample = Sample()
            sample.add_value(sample_type_id, 1)
            sample.add_value(sample_type_id + 1, report_sample.period)
            if self._filter_symbol(symbol):
                location_id = self.get_location_id(symbol.vaddr_in_file, symbol)
                sample.add_location_id(location_id)
            for i in range(callchain.nr):
                entry = callchain.entries[i]
                if self._filter_symbol(symbol):
                    location_id = self.get_location_id(entry.ip, entry.symbol)
                    sample.add_location_id(location_id)
            if sample.location_ids:
                self.add_sample(sample)

        # 2. Generate line info for locations and functions.
        self.gen_source_lines()

        # 3. Produce samples/locations/functions in profile
        for sample in self.sample_list:
            self.gen_profile_sample(sample)
        for mapping in self.mapping_list:
            self.gen_profile_mapping(mapping)
        for location in self.location_list:
            self.gen_profile_location(location)
        for function in self.function_list:
            self.gen_profile_function(function)

        return self.profile

    def _filter_report_sample(self, sample):
        """Return true if the sample can be used."""
        if self.comm_filter:
            if sample.thread_comm not in self.comm_filter:
                return False
            if self.pid_filter:
                if sample.pid not in self.pid_filter:
                    return False
            if self.tid_filter:
                if sample.tid not in self.tid_filter:
                    return False
        return True

    def _filter_symbol(self, symbol):
        if not self.dso_filter or symbol.dso_name in self.dso_filter:
            return True
        return False

    def get_string_id(self, str):
        if len(str) == 0:
            return 0
        id = self.string_table.get(str)
        if id is not None:
            return id
        id = len(self.string_table) + 1
        self.string_table[str] = id
        self.profile.string_table.append(str)
        return id

    def get_string(self, string_id):
        return self.profile.string_table[string_id]

    def get_sample_type_id(self, name):
        id = self.sample_types.get(name)
        if id is not None:
            return id
        id = len(self.profile.sample_type)
        sample_type = self.profile.sample_type.add()
        sample_type.type = self.get_string_id('event_' + name + '_samples')
        sample_type.unit = self.get_string_id('count')
        sample_type = self.profile.sample_type.add()
        sample_type.type = self.get_string_id('event_' + name + '_count')
        sample_type.unit = self.get_string_id('count')
        self.sample_types[name] = id
        return id

    def get_location_id(self, ip, symbol):
        mapping_id = self.get_mapping_id(symbol.mapping[0], symbol.dso_name)
        location = Location(mapping_id, ip, symbol.vaddr_in_file)
        function_id = self.get_function_id(symbol.symbol_name, symbol.dso_name,
                                           symbol.symbol_addr)
        if function_id:
            # Add Line only when it has a valid function id, see http://b/36988814.
            # Default line info only contains the function name
            line = Line()
            line.function_id = function_id
            location.lines.append(line)

        exist_location = self.location_map.get(location.key)
        if exist_location:
            return exist_location.id
        # location_id starts from 1
        location.id = len(self.location_list) + 1
        self.location_list.append(location)
        self.location_map[location.key] = location
        return location.id

    def get_mapping_id(self, report_mapping, filename):
        filename_id = self.get_string_id(filename)
        build_id = self.lib.GetBuildIdForPath(filename)
        if build_id and build_id[0:2] == "0x":
            build_id = build_id[2:]
        build_id_id = self.get_string_id(build_id)
        mapping = Mapping(report_mapping.start, report_mapping.end,
                          report_mapping.pgoff, filename_id, build_id_id)
        exist_mapping = self.mapping_map.get(mapping.key)
        if exist_mapping:
            return exist_mapping.id
        # mapping_id starts from 1
        mapping.id = len(self.mapping_list) + 1
        self.mapping_list.append(mapping)
        self.mapping_map[mapping.key] = mapping
        return mapping.id

    def get_mapping(self, mapping_id):
        return self.mapping_list[mapping_id - 1] if mapping_id > 0 else None

    def get_function_id(self, name, dso_name, vaddr_in_file):
        if name == 'unknown':
            return 0
        function = Function(self.get_string_id(name), self.get_string_id(dso_name), vaddr_in_file)
        exist_function = self.function_map.get(function.key)
        if exist_function:
            return exist_function.id
        # function_id starts from 1
        function.id = len(self.function_list) + 1
        self.function_list.append(function)
        self.function_map[function.key] = function
        return function.id

    def get_function(self, function_id):
        return self.function_list[function_id - 1] if function_id > 0 else None

    def add_sample(self, sample):
        exist_sample = self.sample_map.get(sample.key)
        if exist_sample:
            exist_sample.add_values(sample.values)
        else:
            self.sample_list.append(sample)
            self.sample_map[sample.key] = sample

    def gen_source_lines(self):
        # 1. Create Addr2line instance
        if not self.config.get('binary_cache_dir'):
            log_info("Can't generate line information because binary_cache is missing.")
            return
        if not self.config['addr2line_path'] or not is_executable_available(
            self.config['addr2line_path']):
            if not find_tool_path('addr2line'):
                log_info("Can't generate line information because can't find addr2line.")
                return

        addr2line = Addr2Line(self.config['addr2line_path'], self.config['binary_cache_dir'])

        # 2. Put all needed addresses to it.
        for location in self.location_list:
            mapping = self.get_mapping(location.mapping_id)
            dso_name = self.get_string(mapping.filename_id)
            addr2line.add_addr(dso_name, location.vaddr_in_dso)
        for function in self.function_list:
            dso_name = self.get_string(function.dso_name_id)
            addr2line.add_addr(dso_name, function.vaddr_in_dso)

        # 3. Generate source lines.
        addr2line.convert_addrs_to_lines()

        # 4. Annotate locations and functions.
        for location in self.location_list:
            mapping = self.get_mapping(location.mapping_id)
            dso_name = self.get_string(mapping.filename_id)
            sources = addr2line.get_sources(dso_name, location.vaddr_in_dso)
            source_id = 0
            for source in sources:
                if source.file and source.function and source.line:
                    function_id = self.get_function_id(source.function, dso_name, 0)
                    if function_id == 0:
                        continue
                    if source_id == 0:
                        # Clear default line info
                        location.lines = []
                    location.lines.append(self.add_line(source, dso_name, function_id))
                    source_id += 1

        for function in self.function_list:
            dso_name = self.get_string(function.dso_name_id)
            if function.vaddr_in_dso:
                sources = addr2line.get_sources(dso_name, function.vaddr_in_dso)
                source = sources[0] if sources else None
                if source and source.file:
                    function.source_filename_id = self.get_string_id(source.file)
                    if source.line:
                        function.start_line = source.line

    def add_line(self, source, dso_name, function_id):
        line = Line()
        function = self.get_function(function_id)
        function.source_filename_id = self.get_string_id(source.file)
        line.function_id = function_id
        line.line = source.line
        return line

    def gen_profile_sample(self, sample):
        profile_sample = self.profile.sample.add()
        profile_sample.location_id.extend(sample.location_ids)
        sample_type_count = len(self.sample_types) * 2
        values = [0] * sample_type_count
        for id in sample.values.keys():
            values[id] = sample.values[id]
        profile_sample.value.extend(values)

    def gen_profile_mapping(self, mapping):
        profile_mapping = self.profile.mapping.add()
        profile_mapping.id = mapping.id
        profile_mapping.memory_start = mapping.memory_start
        profile_mapping.memory_limit = mapping.memory_limit
        profile_mapping.file_offset = mapping.file_offset
        profile_mapping.filename = mapping.filename_id
        profile_mapping.build_id = mapping.build_id_id
        profile_mapping.has_filenames = True
        profile_mapping.has_functions = True
        if self.config.get('binary_cache_dir'):
            profile_mapping.has_line_numbers = True
            profile_mapping.has_inline_frames = True
        else:
            profile_mapping.has_line_numbers = False
            profile_mapping.has_inline_frames = False

    def gen_profile_location(self, location):
        profile_location = self.profile.location.add()
        profile_location.id = location.id
        profile_location.mapping_id = location.mapping_id
        profile_location.address = location.address
        for i in range(len(location.lines)):
            line = profile_location.line.add()
            line.function_id = location.lines[i].function_id
            line.line = location.lines[i].line

    def gen_profile_function(self, function):
        profile_function = self.profile.function.add()
        profile_function.id = function.id
        profile_function.name = function.name_id
        profile_function.system_name = function.name_id
        profile_function.filename = function.source_filename_id
        profile_function.start_line = function.start_line


def main():
    parser = argparse.ArgumentParser(description='Generate pprof profile data in pprof.profile.')
    parser.add_argument('--show', nargs='?', action='append', help='print existing pprof.profile.')
    parser.add_argument('-i', '--perf_data_path', default='perf.data', help=
"""The path of profiling data.""")
    parser.add_argument('-o', '--output_file', default='pprof.profile', help=
"""The path of generated pprof profile data.""")
    parser.add_argument('--comm', nargs='+', action='append', help=
"""Use samples only in threads with selected names.""")
    parser.add_argument('--pid', nargs='+', action='append', help=
"""Use samples only in processes with selected process ids.""")
    parser.add_argument('--tid', nargs='+', action='append', help=
"""Use samples only in threads with selected thread ids.""")
    parser.add_argument('--dso', nargs='+', action='append', help=
"""Use samples only in selected binaries.""")
    parser.add_argument('--addr2line', help=
"""Set the path of addr2line.""")

    args = parser.parse_args()
    if args.show:
        show_file = args.show[0] if args.show[0] else 'pprof.profile'
        profile = load_pprof_profile(show_file)
        printer = PprofProfilePrinter(profile)
        printer.show()
        return

    config = {}
    config['perf_data_path'] = args.perf_data_path
    config['output_file'] = args.output_file
    config['comm_filters'] = flatten_arg_list(args.comm)
    config['pid_filters'] = flatten_arg_list(args.pid)
    config['tid_filters'] = flatten_arg_list(args.tid)
    config['dso_filters'] = flatten_arg_list(args.dso)
    config['addr2line_path'] = args.addr2line
    generator = PprofProfileGenerator(config)
    profile = generator.gen()
    store_pprof_profile(config['output_file'], profile)


if __name__ == '__main__':
    main()
