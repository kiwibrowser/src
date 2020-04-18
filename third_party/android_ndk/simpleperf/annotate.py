#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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

"""annotate.py: annotate source files based on perf.data.
"""


import argparse
import os
import os.path
import shutil
import subprocess
import sys

from simpleperf_report_lib import *
from utils import *

class SourceLine(object):
    def __init__(self, file, function, line):
        self.file = file
        self.function = function
        self.line = line

    @property
    def file_key(self):
        return self.file

    @property
    def function_key(self):
        return (self.file, self.function)

    @property
    def line_key(self):
        return (self.file, self.line)


# TODO: using addr2line can't convert from function_start_address to
# source_file:line very well for java code. Because in .debug_line section,
# there is some distance between function_start_address and the address
# of the first instruction which can be mapped to source line.
class Addr2Line(object):
    """collect information of how to map [dso_name,vaddr] to [source_file:line].
    """
    def __init__(self, addr2line_path, symfs_dir=None):
        self.dso_dict = dict()
        if addr2line_path and is_executable_available(addr2line_path):
            self.addr2line_path = addr2line_path
        else:
            self.addr2line_path = find_tool_path('addr2line')
            if not self.addr2line_path:
                log_exit("Can't find addr2line.")
        self.symfs_dir = symfs_dir


    def add_addr(self, dso_name, addr):
        dso = self.dso_dict.get(dso_name)
        if dso is None:
            self.dso_dict[dso_name] = dso = dict()
        if addr not in dso:
            dso[addr] = None


    def convert_addrs_to_lines(self):
        # store a list of source files
        self.file_list = []
        # map from file to id with file_list[id] == file
        self.file_dict = {}
        self.file_list.append('')
        self.file_dict[''] = 0

        for dso_name in self.dso_dict.keys():
            self._convert_addrs_to_lines(dso_name, self.dso_dict[dso_name])
        self._combine_source_files()


    def _run_llvm_symbolizer(self, dso_path, addrs):
        subproc = subprocess.Popen([self.addr2line_path, '-obj=%s' % dso_path,
                                    '-demangle', '-functions=linkage'],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        addr_str = '\n'.join('0x%x' % addr for addr in addrs)
        (stdoutdata, _) = subproc.communicate(str_to_bytes(addr_str))
        stdoutdata = bytes_to_str(stdoutdata)
        stanzas = stdoutdata.split('\n\n')
        if len(stanzas) < len(addrs):
            log_fatal("llvm-symbolizer produced %d stanzas for %d addresses" % (
                len(stanzas), len(addrs)))
        # Munge output to look like addr2line output
        result = []
        for i, addr in enumerate(addrs):
            result.append('0x%x' % addr)
            lines = stanzas[i].split('\n')
            if len(lines) % 2:
                log_fatal("llvm-symbolizer produced a stanza with %d lines" % len(lines))
            for j in xrange(0, len(lines), 2):
                result.append(lines[j])
                # Strip character position in line
                result.append(lines[j+1].rsplit(':', 1)[0])
        return result


    def _run_addr2line(self, dso_path, addrs):
        subproc = subprocess.Popen([self.addr2line_path, '-e', dso_path, '-aifC'],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        addr_str = '\n'.join(['0x%x' % addr for addr in addrs])
        (stdoutdata, _) = subproc.communicate(str_to_bytes(addr_str))
        stdoutdata = bytes_to_str(stdoutdata)
        stdoutdata = stdoutdata.strip().split('\n')
        if len(stdoutdata) < len(addrs):
            log_fatal("addr2line didn't output enough lines")
        return stdoutdata


    def _convert_addrs_to_lines(self, dso_name, dso):
        dso_path = self._find_dso_path(dso_name)
        if dso_path is None:
            log_warning("can't find dso '%s'" % dso_name)
            dso.clear()
            return
        addrs = sorted(dso.keys())
        if self.addr2line_path.endswith('llvm-symbolizer'):
            stdoutdata = self._run_llvm_symbolizer(dso_path, addrs)
        else:
            stdoutdata = self._run_addr2line(dso_path, addrs)
        addr_pos = 0
        out_pos = 0
        while addr_pos < len(addrs) and out_pos < len(stdoutdata):
            addr_line = stdoutdata[out_pos]
            out_pos += 1
            assert addr_line[:2] == "0x"
            assert out_pos < len(stdoutdata)
            source_lines = []
            while out_pos < len(stdoutdata) and stdoutdata[out_pos][:2] != "0x":
                function = stdoutdata[out_pos]
                out_pos += 1
                assert out_pos < len(stdoutdata)
                # Handle lines like "C:\Users\...\file:32".
                items = stdoutdata[out_pos].rsplit(':', 1)
                if len(items) != 2:
                    continue
                (file, line) = items
                line = line.split()[0]  # Remove comments after line number
                out_pos += 1
                if '?' in file:
                    file = 0
                else:
                    file = self._get_file_id(file)
                if '?' in line:
                    line = 0
                else:
                    line = int(line)
                source_lines.append(SourceLine(file, function, line))
            dso[addrs[addr_pos]] = source_lines
            addr_pos += 1
        assert addr_pos == len(addrs)


    def _get_file_id(self, file):
        id = self.file_dict.get(file)
        if id is None:
            id = len(self.file_list)
            self.file_list.append(file)
            self.file_dict[file] = id
        return id

    def _combine_source_files(self):
        """It is possible that addr2line gives us different names for the same
           file, like:
            /usr/local/.../src/main/jni/sudo-game-jni.cpp
            sudo-game-jni.cpp
           We'd better combine these two files. We can do it by combining
           source files with no conflicts in path.
        """
        # Collect files having the same filename.
        filename_dict = dict()
        for file in self.file_list:
            index = max(file.rfind('/'), file.rfind(os.sep))
            filename = file[index+1:]
            entry = filename_dict.get(filename)
            if entry is None:
                filename_dict[filename] = entry = []
            entry.append(file)

        # Combine files having the same filename and having no conflicts in path.
        for filename in filename_dict.keys():
            files = filename_dict[filename]
            if len(files) == 1:
                continue
            for file in files:
                to_file = file
                # Test if we can merge files[i] with another file having longer
                # path.
                for f in files:
                    if len(f) > len(to_file) and f.find(file) != -1:
                        to_file = f
                if to_file != file:
                    from_id = self.file_dict[file]
                    to_id = self.file_dict[to_file]
                    self.file_list[from_id] = self.file_list[to_id]


    def get_sources(self, dso_name, addr):
        dso = self.dso_dict.get(dso_name)
        if dso is None:
            return []
        item = dso.get(addr, [])
        source_lines = []
        for source in item:
            source_lines.append(SourceLine(self.file_list[source.file],
                                           source.function, source.line))
        return source_lines


    def _find_dso_path(self, dso):
        if dso[0] != '/' or dso == '//anon':
            return None
        if self.symfs_dir:
            dso_path = os.path.join(self.symfs_dir, dso[1:])
            if os.path.isfile(dso_path):
                return dso_path
        if os.path.isfile(dso):
            return dso
        return None


class Period(object):
    """event count information. It can be used to represent event count
       of a line, a function, a source file, or a binary. It contains two
       parts: period and acc_period.
       When used for a line, period is the event count occurred when running
       that line, acc_period is the accumulated event count occurred when
       running that line and functions called by that line. Same thing applies
       when it is used for a function, a source file, or a binary.
    """
    def __init__(self, period=0, acc_period=0):
        self.period = period
        self.acc_period = acc_period


    def __iadd__(self, other):
        self.period += other.period
        self.acc_period += other.acc_period
        return self


class DsoPeriod(object):
    """Period for each shared library"""
    def __init__(self, dso_name):
        self.dso_name = dso_name
        self.period = Period()


    def add_period(self, period):
        self.period += period


class FilePeriod(object):
    """Period for each source file"""
    def __init__(self, file):
        self.file = file
        self.period = Period()
        # Period for each line in the file.
        self.line_dict = {}
        # Period for each function in the source file.
        self.function_dict = {}


    def add_period(self, period):
        self.period += period


    def add_line_period(self, line, period):
        a = self.line_dict.get(line)
        if a is None:
            self.line_dict[line] = a = Period()
        a += period


    def add_function_period(self, function_name, function_start_line, period):
        a = self.function_dict.get(function_name)
        if not a:
            if function_start_line is None:
                function_start_line = -1
            self.function_dict[function_name] = a = [function_start_line, Period()]
        a[1] += period


class SourceFileAnnotator(object):
    """group code for annotating source files"""
    def __init__(self, config):
        # check config variables
        config_names = ['perf_data_list', 'source_dirs', 'comm_filters',
                        'pid_filters', 'tid_filters', 'dso_filters', 'addr2line_path']
        for name in config_names:
            if name not in config:
                log_exit('config [%s] is missing' % name)
        symfs_dir = 'binary_cache'
        if not os.path.isdir(symfs_dir):
            symfs_dir = None
        kallsyms = 'binary_cache/kallsyms'
        if not os.path.isfile(kallsyms):
            kallsyms = None
        source_dirs = config['source_dirs']
        for dir in source_dirs:
            if not os.path.isdir(dir):
                log_exit('[source_dirs] "%s" is not a dir' % dir)
        if not config['source_dirs']:
            log_exit('Please set source directories.')

        # init member variables
        self.config = config
        self.symfs_dir = symfs_dir
        self.kallsyms = kallsyms
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

        config['annotate_dest_dir'] = 'annotated_files'
        output_dir = config['annotate_dest_dir']
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)
        os.makedirs(output_dir)

        self.addr2line = Addr2Line(self.config['addr2line_path'], symfs_dir)


    def annotate(self):
        self._collect_addrs()
        self._convert_addrs_to_lines()
        self._generate_periods()
        self._write_summary()
        self._collect_source_files()
        self._annotate_files()


    def _collect_addrs(self):
        """Read perf.data, collect all addresses we need to convert to
           source file:line.
        """
        for perf_data in self.config['perf_data_list']:
            lib = ReportLib()
            lib.SetRecordFile(perf_data)
            if self.symfs_dir:
                lib.SetSymfs(self.symfs_dir)
            if self.kallsyms:
                lib.SetKallsymsFile(self.kallsyms)
            while True:
                sample = lib.GetNextSample()
                if sample is None:
                    lib.Close()
                    break
                if not self._filter_sample(sample):
                    continue
                symbols = []
                symbols.append(lib.GetSymbolOfCurrentSample())
                callchain = lib.GetCallChainOfCurrentSample()
                for i in range(callchain.nr):
                    symbols.append(callchain.entries[i].symbol)
                for symbol in symbols:
                    if self._filter_symbol(symbol):
                        self.addr2line.add_addr(symbol.dso_name, symbol.vaddr_in_file)
                        self.addr2line.add_addr(symbol.dso_name, symbol.symbol_addr)


    def _filter_sample(self, sample):
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


    def _convert_addrs_to_lines(self):
        self.addr2line.convert_addrs_to_lines()


    def _generate_periods(self):
        """read perf.data, collect Period for all types:
            binaries, source files, functions, lines.
        """
        self.period = 0
        self.dso_periods = dict()
        self.file_periods = dict()
        for perf_data in self.config['perf_data_list']:
            lib = ReportLib()
            lib.SetRecordFile(perf_data)
            if self.symfs_dir:
                lib.SetSymfs(self.symfs_dir)
            if self.kallsyms:
                lib.SetKallsymsFile(self.kallsyms)
            while True:
                sample = lib.GetNextSample()
                if sample is None:
                    lib.Close()
                    break
                if not self._filter_sample(sample):
                    continue
                symbols = []
                symbols.append(lib.GetSymbolOfCurrentSample())
                callchain = lib.GetCallChainOfCurrentSample()
                for i in range(callchain.nr):
                    symbols.append(callchain.entries[i].symbol)
                # Each sample has a callchain, but its period is only used once
                # to add period for each function/source_line/source_file/binary.
                # For example, if more than one entry in the callchain hits a
                # function, the event count of that function is only increased once.
                # Otherwise, we may get periods > 100%.
                is_sample_used = False
                used_dso_dict = dict()
                used_file_dict = dict()
                used_function_dict = dict()
                used_line_dict = dict()
                period = Period(sample.period, sample.period)
                for i in range(len(symbols)):
                    symbol = symbols[i]
                    if i == 1:
                        period = Period(0, sample.period)
                    if not self._filter_symbol(symbol):
                        continue
                    is_sample_used = True
                    # Add period to dso.
                    self._add_dso_period(symbol.dso_name, period, used_dso_dict)
                    # Add period to source file.
                    sources = self.addr2line.get_sources(symbol.dso_name, symbol.vaddr_in_file)
                    for source in sources:
                        if source.file:
                            self._add_file_period(source, period, used_file_dict)
                            # Add period to line.
                            if source.line:
                                self._add_line_period(source, period, used_line_dict)
                    # Add period to function.
                    sources = self.addr2line.get_sources(symbol.dso_name, symbol.symbol_addr)
                    for source in sources:
                        if source.file:
                            self._add_file_period(source, period, used_file_dict)
                            if source.function:
                                self._add_function_period(source, period, used_function_dict)

                if is_sample_used:
                    self.period += sample.period


    def _add_dso_period(self, dso_name, period, used_dso_dict):
        if dso_name not in used_dso_dict:
            used_dso_dict[dso_name] = True
            dso_period = self.dso_periods.get(dso_name)
            if dso_period is None:
                dso_period = self.dso_periods[dso_name] = DsoPeriod(dso_name)
            dso_period.add_period(period)


    def _add_file_period(self, source, period, used_file_dict):
        if source.file_key not in used_file_dict:
            used_file_dict[source.file_key] = True
            file_period = self.file_periods.get(source.file)
            if file_period is None:
                file_period = self.file_periods[source.file] = FilePeriod(source.file)
            file_period.add_period(period)


    def _add_line_period(self, source, period, used_line_dict):
        if source.line_key not in used_line_dict:
            used_line_dict[source.line_key] = True
            file_period = self.file_periods[source.file]
            file_period.add_line_period(source.line, period)


    def _add_function_period(self, source, period, used_function_dict):
        if source.function_key not in used_function_dict:
            used_function_dict[source.function_key] = True
            file_period = self.file_periods[source.file]
            file_period.add_function_period(source.function, source.line, period)


    def _write_summary(self):
        summary = os.path.join(self.config['annotate_dest_dir'], 'summary')
        with open(summary, 'w') as f:
            f.write('total period: %d\n\n' % self.period)
            dso_periods = sorted(self.dso_periods.values(),
                                 key=lambda x: x.period.acc_period, reverse=True)
            for dso_period in dso_periods:
                f.write('dso %s: %s\n' % (dso_period.dso_name,
                                          self._get_percentage_str(dso_period.period)))
            f.write('\n')

            file_periods = sorted(self.file_periods.values(),
                                  key=lambda x: x.period.acc_period, reverse=True)
            for file_period in file_periods:
                f.write('file %s: %s\n' % (file_period.file,
                                           self._get_percentage_str(file_period.period)))
            for file_period in file_periods:
                f.write('\n\n%s: %s\n' % (file_period.file,
                                          self._get_percentage_str(file_period.period)))
                values = []
                for func_name in file_period.function_dict.keys():
                    func_start_line, period = file_period.function_dict[func_name]
                    values.append((func_name, func_start_line, period))
                values = sorted(values, key=lambda x: x[2].acc_period, reverse=True)
                for value in values:
                    f.write('\tfunction (%s): line %d, %s\n' % (
                        value[0], value[1], self._get_percentage_str(value[2])))
                f.write('\n')
                for line in sorted(file_period.line_dict.keys()):
                    f.write('\tline %d: %s\n' % (
                        line, self._get_percentage_str(file_period.line_dict[line])))


    def _get_percentage_str(self, period, short=False):
        s = 'acc_p: %f%%, p: %f%%' if short else 'accumulated_period: %f%%, period: %f%%'
        return s % self._get_percentage(period)


    def _get_percentage(self, period):
        if self.period == 0:
            return (0, 0)
        acc_p = 100.0 * period.acc_period / self.period
        p = 100.0 * period.period / self.period
        return (acc_p, p)


    def _collect_source_files(self):
        self.source_file_dict = dict()
        source_file_suffix = ['h', 'c', 'cpp', 'cc', 'java', 'kt']
        for source_dir in self.config['source_dirs']:
            for root, _, files in os.walk(source_dir):
                for file in files:
                    if file[file.rfind('.')+1:] in source_file_suffix:
                        entry = self.source_file_dict.get(file)
                        if entry is None:
                            entry = self.source_file_dict[file] = []
                        entry.append(os.path.join(root, file))


    def _find_source_file(self, file):
        filename = file[file.rfind(os.sep)+1:]
        source_files = self.source_file_dict.get(filename)
        if source_files is None:
            return None
        best_path_count = 0
        best_path = None
        best_suffix_len = 0
        for path in source_files:
            suffix_len = len(os.path.commonprefix((path[::-1], file[::-1])))
            if suffix_len > best_suffix_len:
                best_suffix_len = suffix_len
                best_path = path
                best_path_count = 1
            elif suffix_len == best_suffix_len:
                best_path_count += 1
        if best_path_count > 1:
            log_warning('multiple source for %s, select %s' % (file, best_path))
        return best_path


    def _annotate_files(self):
        """Annotate Source files: add acc_period/period for each source file.
           1. Annotate java source files, which have $JAVA_SRC_ROOT prefix.
           2. Annotate c++ source files.
        """
        dest_dir = self.config['annotate_dest_dir']
        for key in self.file_periods.keys():
            is_java = False
            if key.startswith('$JAVA_SRC_ROOT/'):
                path = key[len('$JAVA_SRC_ROOT/'):]
                items = path.split('/')
                path = os.sep.join(items)
                from_path = self._find_source_file(path)
                to_path = os.path.join(dest_dir, 'java', path)
                is_java = True
            elif key.startswith('/') and os.path.isfile(key):
                path = key
                from_path = path
                to_path = os.path.join(dest_dir, path[1:])
            elif is_windows() and ':\\' in key and os.path.isfile(key):
                from_path = key
                to_path = os.path.join(dest_dir, key.replace(':\\', '\\'))
            else:
                path = key[1:] if key.startswith('/') else key
                # Change path on device to path on host
                path = os.sep.join(path.split('/'))
                from_path = self._find_source_file(path)
                to_path = os.path.join(dest_dir, path)
            if from_path is None:
                log_warning("can't find source file for path %s" % key)
                continue
            self._annotate_file(from_path, to_path, self.file_periods[key], is_java)


    def _annotate_file(self, from_path, to_path, file_period, is_java):
        """Annotate a source file.

        Annotate a source file in three steps:
          1. In the first line, show periods of this file.
          2. For each function, show periods of this function.
          3. For each line not hitting the same line as functions, show
             line periods.
        """
        log_info('annotate file %s' % from_path)
        with open(from_path, 'r') as rf:
            lines = rf.readlines()

        annotates = dict()
        for line in file_period.line_dict.keys():
            annotates[line] = self._get_percentage_str(file_period.line_dict[line], True)
        for func_name in file_period.function_dict.keys():
            func_start_line, period = file_period.function_dict[func_name]
            if func_start_line == -1:
                continue
            line = func_start_line - 1 if is_java else func_start_line
            annotates[line] = '[func] ' + self._get_percentage_str(period, True)
        annotates[1] = '[file] ' + self._get_percentage_str(file_period.period, True)

        max_annotate_cols = 0
        for key in annotates.keys():
            max_annotate_cols = max(max_annotate_cols, len(annotates[key]))

        empty_annotate = ' ' * (max_annotate_cols + 6)

        dirname = os.path.dirname(to_path)
        if not os.path.isdir(dirname):
            os.makedirs(dirname)
        with open(to_path, 'w') as wf:
            for line in range(1, len(lines) + 1):
                annotate = annotates.get(line)
                if annotate is None:
                    if not lines[line-1].strip():
                        annotate = ''
                    else:
                        annotate = empty_annotate
                else:
                    annotate = '/* ' + annotate + (
                        ' ' * (max_annotate_cols - len(annotate))) + ' */'
                wf.write(annotate)
                wf.write(lines[line-1])

def main():
    parser = argparse.ArgumentParser(description=
"""Annotate source files based on profiling data. It reads line information from
binary_cache generated by app_profiler.py or binary_cache_builder.py, and
generate annotated source files in annotated_files directory.""")
    parser.add_argument('-i', '--perf_data_list', nargs='+', action='append', help=
"""The paths of profiling data. Default is perf.data.""")
    parser.add_argument('-s', '--source_dirs', nargs='+', action='append', help=
"""Directories to find source files.""")
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
    config = {}
    config['perf_data_list'] = flatten_arg_list(args.perf_data_list)
    if not config['perf_data_list']:
        config['perf_data_list'].append('perf.data')
    config['source_dirs'] = flatten_arg_list(args.source_dirs)
    config['comm_filters'] = flatten_arg_list(args.comm)
    config['pid_filters'] = flatten_arg_list(args.pid)
    config['tid_filters'] = flatten_arg_list(args.tid)
    config['dso_filters'] = flatten_arg_list(args.dso)
    config['addr2line_path'] = args.addr2line

    annotator = SourceFileAnnotator(config)
    annotator.annotate()
    log_info('annotate finish successfully, please check result in annotated_files/.')

if __name__ == '__main__':
    main()
