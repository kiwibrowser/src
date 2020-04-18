# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Android-specific implementation of the core backend interfaces.

See core/backends.py for more docs.
"""

import datetime
import glob
import hashlib
import json
import os
import posixpath
import tempfile
import time

from memory_inspector import constants
from memory_inspector.backends import adb_client
from memory_inspector.backends import memdump_parser
from memory_inspector.backends import native_heap_dump_parser
from memory_inspector.backends import prebuilts_fetcher
from memory_inspector.core import backends
from memory_inspector.core import exceptions
from memory_inspector.core import native_heap
from memory_inspector.core import symbol


_SUPPORTED_32BIT_ABIS = {'armeabi': 'arm', 'armeabi-v7a': 'arm', 'x86': 'x86'}
_SUPPORTED_64BIT_ABIS = {'arm64-v8a': 'arm64', 'x86_64': 'x86_64'}
_MEMDUMP_PREBUILT_PATH = os.path.join(constants.PREBUILTS_PATH,
                                      'memdump-android-%(arch)s')
_MEMDUMP_PATH_ON_DEVICE = '/data/local/tmp/memdump'
_PSEXT_PREBUILT_PATH = os.path.join(constants.PREBUILTS_PATH,
                                    'ps_ext-android-%(arch)s')
_PSEXT_PATH_ON_DEVICE = '/data/local/tmp/ps_ext'
_HEAP_DUMP_PREBUILT_PATH = os.path.join(constants.PREBUILTS_PATH,
                                        'heap_dump-android-%(arch)s')
_HEAP_DUMP_PATH_ON_DEVICE = '/data/local/tmp/heap_dump'
_LIBHEAPPROF_PREBUILT_PATH = os.path.join(constants.PREBUILTS_PATH,
                                          'libheap_profiler-android-%(arch)s')
_LIBHEAPPROF_FILE_NAME = 'libheap_profiler.so'


class AndroidBackend(backends.Backend):
  """Android-specific implementation of the core |Backend| interface."""

  _SETTINGS_KEYS = {
      'toolchain_path': 'Path of toolchain (for addr2line)'}

  def __init__(self):
    super(AndroidBackend, self).__init__(
        settings=backends.Settings(AndroidBackend._SETTINGS_KEYS))
    self._devices = {}  # 'device id' -> |Device|.

  def EnumerateDevices(self):
    for adb_device in adb_client.ListDevices():
      device = self._devices.get(adb_device.serial)
      if not device:
        device = AndroidDevice(self, adb_device)
        self._devices[adb_device.serial] = device
      yield device

  def ExtractSymbols(self, native_heaps, sym_paths):
    """Performs symbolization. Returns a |symbol.Symbols| from |NativeHeap|s.

    This method performs the symbolization but does NOT decorate (i.e. add
    symbol/source info) to the stack frames of |native_heaps|. The heaps
    can be decorated as needed using the native_heap.SymbolizeUsingSymbolDB()
    method. Rationale: the most common use case in this application is:
    symbolize-and-store-symbols and load-symbols-and-decorate-heaps (in two
    different stages at two different times).

    Args:
      native_heaps: a collection of native_heap.NativeHeap instances.
      sym_paths: either a list of or a string of semicolon-sep. symbol paths.
    """
    assert(all(isinstance(x, native_heap.NativeHeap) for x in native_heaps))
    symbols = symbol.Symbols()

    # Find addr2line in toolchain_path.
    if isinstance(sym_paths, basestring):
      sym_paths = sym_paths.split(';')
    matches = glob.glob(os.path.join(self.settings['toolchain_path'],
                                     '*addr2line'))
    if not matches:
      raise exceptions.MemoryInspectorException('Cannot find addr2line')
    addr2line_path = matches[0]

    # First group all the stack frames together by lib path.
    frames_by_lib = {}
    for nheap in native_heaps:
      for stack_frame in nheap.stack_frames.itervalues():
        frames = frames_by_lib.setdefault(stack_frame.exec_file_rel_path, set())
        frames.add(stack_frame)

    # The symbolization process is asynchronous (but yet single-threaded). This
    # callback is invoked every time the symbol info for a stack frame is ready.
    def SymbolizeAsyncCallback(sym_info, stack_frame):
      if not sym_info.name:
        return
      sym = symbol.Symbol(name=sym_info.name,
                          source_file_path=sym_info.source_path,
                          line_number=sym_info.source_line)
      symbols.Add(stack_frame.exec_file_rel_path, stack_frame.offset, sym)
      # TODO(primiano): support inline sym info (i.e. |sym_info.inlined_by|).

    # Perform the actual symbolization (ordered by lib).
    for exec_file_rel_path, frames in frames_by_lib.iteritems():
      # Look up the full path of the symbol in the sym paths.
      exec_file_name = posixpath.basename(exec_file_rel_path)
      if exec_file_rel_path.startswith('/'):
        exec_file_rel_path = exec_file_rel_path[1:]
      if not exec_file_rel_path:
        continue
      exec_file_abs_path = ''
      for sym_path in sym_paths:
        # First try to locate the symbol file following the full relative path
        # e.g. /host/syms/ + /system/lib/foo.so => /host/syms/system/lib/foo.so.
        exec_file_abs_path = os.path.join(sym_path, exec_file_rel_path)
        if os.path.exists(exec_file_abs_path):
          break

        # If no luck, try looking just for the file name in the sym path,
        # e.g. /host/syms/ + (/system/lib/)foo.so => /host/syms/foo.so.
        exec_file_abs_path = os.path.join(sym_path, exec_file_name)
        if os.path.exists(exec_file_abs_path):
          break

        # In the case of a Chrome component=shared_library build, the libs are
        # renamed to .cr.so. Look for foo.so => foo.cr.so.
        exec_file_abs_path = os.path.join(
            sym_path, exec_file_name.replace('.so', '.cr.so'))
        if os.path.exists(exec_file_abs_path):
          break

      if not os.path.isfile(exec_file_abs_path):
        continue

      # The memory_inspector/__init__ module will add the /src/build/android
      # deps to the PYTHONPATH for pylib.
      from pylib.symbols import elf_symbolizer
      symbolizer = elf_symbolizer.ELFSymbolizer(
          elf_file_path=exec_file_abs_path,
          addr2line_path=addr2line_path,
          callback=SymbolizeAsyncCallback,
          inlines=False)

      # Kick off the symbolizer and then wait that all callbacks are issued.
      for stack_frame in sorted(frames, key=lambda x: x.offset):
        symbolizer.SymbolizeAsync(stack_frame.offset, stack_frame)
      symbolizer.Join()

    return symbols

  @property
  def name(self):
    return 'Android'


class AndroidDevice(backends.Device):
  """Android-specific implementation of the core |Device| interface."""

  _SETTINGS_KEYS = {
      'native_symbol_paths': 'Semicolon-sep. list of native libs search path'}

  def __init__(self, backend, adb):
    super(AndroidDevice, self).__init__(
        backend=backend,
        settings=backends.Settings(AndroidDevice._SETTINGS_KEYS))
    self.adb = adb
    self._name = '%s %s' % (adb.GetProp('ro.product.model', cached=True),
                            adb.GetProp('ro.build.id', cached=True))
    self._id = adb.serial
    self._sys_stats = None
    self._last_device_stats = None
    self._sys_stats_last_update = None
    self._processes = {}  # pid (int) -> |Process|
    self._initialized = False

    # Determine the available ABIs, |_arch| will contain the primary ABI.
    # TODO(primiano): For the moment we support only one ABI per device (i.e. we
    # assume that all processes are 64 bit on 64 bit device, failing to profile
    # 32 bit ones). Dealing properly with multi-ABIs requires work on ps_ext and
    # at the moment is not an interesting use case.
    self._arch = None
    self._arch32 = None
    self._arch64 = None
    abi = adb.GetProp('ro.product.cpu.abi', cached=True)
    if abi in _SUPPORTED_64BIT_ABIS:
      self._arch = self._arch64 = _SUPPORTED_64BIT_ABIS[abi]
    elif abi in _SUPPORTED_32BIT_ABIS:
      self._arch = self._arch32 = _SUPPORTED_32BIT_ABIS[abi]
    else:
      raise exceptions.MemoryInspectorException('ABI %s not supported' % abi)

  def Initialize(self):
    """Starts adb root and deploys the prebuilt binaries on initialization."""
    try:
      self.adb.RestartShellAsRoot()
      self.adb.WaitForDevice()
    except adb_client.ADBClientError:
      raise exceptions.MemoryInspectorException(
          'The device must be adb root-able in order to use memory_inspector')

    # Download (from GCS) and deploy prebuilt helper binaries on the device.
    self._DeployPrebuiltOnDeviceIfNeeded(
        _MEMDUMP_PREBUILT_PATH % {'arch': self._arch}, _MEMDUMP_PATH_ON_DEVICE)
    self._DeployPrebuiltOnDeviceIfNeeded(
        _PSEXT_PREBUILT_PATH % {'arch': self._arch}, _PSEXT_PATH_ON_DEVICE)
    self._DeployPrebuiltOnDeviceIfNeeded(
        _HEAP_DUMP_PREBUILT_PATH % {'arch': self._arch},
        _HEAP_DUMP_PATH_ON_DEVICE)

    self._initialized = True

  def IsNativeTracingEnabled(self):
    """Checks whether the libheap_profiler is preloaded in the zygote."""
    zygote_name = 'zygote64' if self._arch64 else 'zygote'
    zygote_process = [p for p in self.ListProcesses() if p.name == zygote_name]
    if not zygote_process:
      raise exceptions.MemoryInspectorException('Zygote process not found')
    zygote_pid = zygote_process[0].pid
    zygote_maps = self.adb.Shell(['cat', '/proc/%d/maps' % zygote_pid])
    return 'libheap_profiler' in zygote_maps

  def EnableNativeTracing(self, enabled):
    """Installs libheap_profiler in and injects it in the Zygote."""

    def WrapZygote(app_process):
      self.adb.Shell(['mv', app_process, app_process + '.real'])
      with tempfile.NamedTemporaryFile() as wrapper_file:
        wrapper_file.write('#!/system/bin/sh\n'
                           'LD_PRELOAD="libheap_profiler.so:$LD_PRELOAD" '
                           'exec %s.real "$@"\n' % app_process)
        wrapper_file.flush()
        self.adb.Push(wrapper_file.name, app_process)
      self.adb.Shell(['chown', 'root.shell', app_process])
      self.adb.Shell(['chmod', '755', app_process])

    def UnwrapZygote():
      for suffix in ('', '32', '64'):
        # We don't really care if app_processX.real doesn't exists and mv fails.
        # If app_processX.real doesn't exists, either app_processX is already
        # unwrapped or it doesn't exists for the current arch.
        app_process = '/system/bin/app_process' + suffix
        self.adb.Shell(['mv', app_process + '.real', app_process])

    assert(self._initialized)
    self.adb.RemountSystemPartition()

    # Start restoring the original state in any case.
    UnwrapZygote()

    if enabled:
      # Temporarily disable SELinux (until next reboot).
      self.adb.Shell(['setenforce', '0'])

      # Wrap the Zygote startup binary (app_process) with a script which
      # LD_PRELOADs libheap_profiler and invokes the original Zygote process.
      if self._arch64:
        app_process = '/system/bin/app_process64'
        assert(self.adb.FileExists(app_process))
        self._DeployPrebuiltOnDeviceIfNeeded(
            _LIBHEAPPROF_PREBUILT_PATH % {'arch': self._arch64},
            '/system/lib64/' + _LIBHEAPPROF_FILE_NAME)
        WrapZygote(app_process)

      if self._arch32:
        # Path is app_process32 for Android >= L, app_process when < L.
        app_process = '/system/bin/app_process32'
        if not self.adb.FileExists(app_process):
          app_process = '/system/bin/app_process'
          assert(self.adb.FileExists(app_process))
        self._DeployPrebuiltOnDeviceIfNeeded(
            _LIBHEAPPROF_PREBUILT_PATH % {'arch': self._arch32},
            '/system/lib/' + _LIBHEAPPROF_FILE_NAME)
        WrapZygote(app_process)

    # Respawn the zygote (the device will kind of reboot at this point).
    self.adb.Shell('stop')
    self.adb.Shell('start')

    # Wait for the package manger to come back.
    for _ in xrange(10):
      found_pm = 'package:' in self.adb.Shell(['pm', 'path', 'android'])
      if found_pm:
        break
      time.sleep(3)
    if not found_pm:
      raise exceptions.MemoryInspectorException('Device unresponsive (no pm)')

    # Remove the wrapper. This won't have effect until the next reboot, when
    # the profiler will be automatically disarmed.
    UnwrapZygote()

    # We can also unlink the lib files at this point. Once the Zygote has
    # started it will keep the inodes refcounted anyways through its lifetime.
    self.adb.Shell(['rm', '/system/lib*/' + _LIBHEAPPROF_FILE_NAME])

  def ListProcesses(self):
    """Returns a sequence of |AndroidProcess|."""
    self._RefreshProcessesList()
    return self._processes.itervalues()

  def GetProcess(self, pid):
    """Returns an instance of |AndroidProcess| (None if not found)."""
    assert(isinstance(pid, int))
    self._RefreshProcessesList()
    return self._processes.get(pid)

  def GetStats(self):
    """Returns an instance of |DeviceStats| with the OS CPU/Memory stats."""
    cur = self.UpdateAndGetSystemStats()
    old = self._last_device_stats or cur  # Handle 1st call case.
    uptime = cur['time']['ticks'] / cur['time']['rate']
    ticks = max(1, cur['time']['ticks'] - old['time']['ticks'])

    cpu_times = []
    for i in xrange(len(cur['cpu'])):
      cpu_time = {
          'usr': 100 * (cur['cpu'][i]['usr'] - old['cpu'][i]['usr']) / ticks,
          'sys': 100 * (cur['cpu'][i]['sys'] - old['cpu'][i]['sys']) / ticks,
          'idle': 100 * (cur['cpu'][i]['idle'] - old['cpu'][i]['idle']) / ticks}
      # The idle tick count on many Linux kernels is frozen when the CPU is
      # offline, and bumps up (compensating all the offline period) when it
      # reactivates. For this reason it needs to be saturated at [0, 100].
      cpu_time['idle'] = max(0, min(cpu_time['idle'],
                                    100 - cpu_time['usr'] - cpu_time['sys']))

      cpu_times.append(cpu_time)

    memory_stats = {'Free': cur['mem']['MemFree:'],
                    'Cache': cur['mem']['Buffers:'] + cur['mem']['Cached:'],
                    'Swap': cur['mem']['SwapCached:'],
                    'Anonymous': cur['mem']['AnonPages:'],
                    'Kernel': cur['mem']['VmallocUsed:']}
    self._last_device_stats = cur

    return backends.DeviceStats(uptime=uptime,
                                cpu_times=cpu_times,
                                memory_stats=memory_stats)

  def UpdateAndGetSystemStats(self):
    """Grabs and caches system stats through ps_ext (max cache TTL = 0.5s).

    Rationale of caching: avoid invoking adb too often, it is slow.
    """
    assert(self._initialized)
    max_ttl = datetime.timedelta(seconds=0.5)
    if (self._sys_stats_last_update and
        datetime.datetime.now() - self._sys_stats_last_update <= max_ttl):
      return self._sys_stats

    dump_out = self.adb.Shell(_PSEXT_PATH_ON_DEVICE)
    stats = json.loads(dump_out)
    assert(all([x in stats for x in ['cpu', 'processes', 'time', 'mem']])), (
        'ps_ext returned a malformed JSON dictionary.')
    self._sys_stats = stats
    self._sys_stats_last_update = datetime.datetime.now()
    return self._sys_stats

  def _RefreshProcessesList(self):
    sys_stats = self.UpdateAndGetSystemStats()
    processes_to_delete = set(self._processes.keys())
    for pid, proc in sys_stats['processes'].iteritems():
      pid = int(pid)
      process = self._processes.get(pid)
      if not process or process.name != proc['name']:
        process = AndroidProcess(self, int(pid), proc['name'])
        self._processes[pid] = process
      processes_to_delete.discard(pid)
    for pid in processes_to_delete:
      del self._processes[pid]

  def _DeployPrebuiltOnDeviceIfNeeded(self, local_path, path_on_device):
    # TODO(primiano): check that the md5 binary is built-in also on pre-KK.
    # Alternatively add tools/android/md5sum to prebuilts and use that one.
    prebuilts_fetcher.GetIfChanged(local_path)
    with open(local_path, 'rb') as f:
      local_hash = hashlib.md5(f.read()).hexdigest()
    device_md5_out = self.adb.Shell(['md5', path_on_device])
    if local_hash in device_md5_out:
      return
    self.adb.Push(local_path, path_on_device)
    self.adb.Shell(['chmod', '755', path_on_device])

  @property
  def name(self):
    """Device name, as defined in the |backends.Device| interface."""
    return self._name

  @property
  def id(self):
    """Device id, as defined in the |backends.Device| interface."""
    return self._id


class AndroidProcess(backends.Process):
  """Android-specific implementation of the core |Process| interface."""

  def __init__(self, device, pid, name):
    super(AndroidProcess, self).__init__(device, pid, name)
    self._last_sys_stats = None

  def DumpMemoryMaps(self):
    """Grabs and parses memory maps through memdump."""
    dump_out = self.device.adb.Shell([_MEMDUMP_PATH_ON_DEVICE, str(self.pid)])
    return memdump_parser.Parse(dump_out)

  def DumpNativeHeap(self):
    """Grabs and parses native heap traces using heap_dump."""
    cmd = [_HEAP_DUMP_PATH_ON_DEVICE, '-n', '-x', str(self.pid)]
    dump_out = self.device.adb.Shell(cmd)
    return native_heap_dump_parser.Parse(dump_out)

  def Freeze(self):
    self.device.adb.Shell(['kill', '-STOP', str(self.pid)])

  def Unfreeze(self):
    self.device.adb.Shell(['kill', '-CONT', str(self.pid)])

  def GetStats(self):
    """Calculate process CPU/VM stats (CPU stats are relative to last call)."""
    # Process must retain its own copy of _last_sys_stats because CPU times
    # are calculated relatively to the last GetStats() call (for the process).
    cur_sys_stats = self.device.UpdateAndGetSystemStats()
    old_sys_stats = self._last_sys_stats or cur_sys_stats
    cur_proc_stats = cur_sys_stats['processes'].get(str(self.pid))
    old_proc_stats = old_sys_stats['processes'].get(str(self.pid))

    # The process might have gone in the meanwhile.
    if (not cur_proc_stats or not old_proc_stats):
      return None

    run_time = (((cur_sys_stats['time']['ticks'] -
                cur_proc_stats['start_time']) / cur_sys_stats['time']['rate']))
    ticks = max(1, cur_sys_stats['time']['ticks'] -
                old_sys_stats['time']['ticks'])
    cpu_usage = (100 *
                 ((cur_proc_stats['user_time'] + cur_proc_stats['sys_time']) -
                 (old_proc_stats['user_time'] + old_proc_stats['sys_time'])) /
                 ticks) / len(cur_sys_stats['cpu'])
    proc_stats = backends.ProcessStats(
        threads=cur_proc_stats['n_threads'],
        run_time=run_time,
        cpu_usage=cpu_usage,
        vm_rss=cur_proc_stats['vm_rss'],
        page_faults=(
            (cur_proc_stats['maj_faults'] + cur_proc_stats['min_faults']) -
            (old_proc_stats['maj_faults'] + old_proc_stats['min_faults'])))
    self._last_sys_stats = cur_sys_stats
    return proc_stats
