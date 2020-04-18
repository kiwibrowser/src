# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

_backends = {}  # Maps a string (backend name) to a |Backend| instance.


def Register(backend):
  """Called by each backend module to register upon initialization."""
  assert(isinstance(backend, Backend))
  _backends[backend.name] = backend


def ListBackends():
  """Enumerates all the backends."""
  return _backends.itervalues()


def ListDevices():
  """Enumerates all the devices from all the registered backends."""
  for backend in _backends.itervalues():
    for device in backend.EnumerateDevices():
      assert(isinstance(device, Device))
      yield device


def GetBackend(backend_name):
  """Retrieves a specific backend given its name."""
  return _backends.get(backend_name, None)


def GetDevice(backend_name, device_id):
  """Retrieves a specific device given its backend name and device id."""
  backend = GetBackend(backend_name)
  if not backend:
    return None
  for device in backend.EnumerateDevices():
    if device.id == device_id:
      return device
  return None


# The classes below model the contract interfaces exposed to the frontends and
# implemented by each concrete backend.

class Backend(object):
  """Base class for backends.

  This is the extension point for the OS-specific profiler implementations.
  """

  def __init__(self, settings=None):
    # Initialize with empty settings if not required by the overriding backend.
    self.settings = settings or Settings()

  def EnumerateDevices(self):
    """Enumeates the devices discovered and supported by the backend.

    Returns:
        A sequence of |Device| instances.
    """
    raise NotImplementedError()

  def ExtractSymbols(self, native_heaps, sym_paths):
    """Performs symbolization. Returns a |symbol.Symbols| from |NativeHeap|s."""
    raise NotImplementedError()

  @property
  def name(self):
    """A unique name which identifies the backend.

    Typically this will just return the target OS name, e.g., 'Android'."""
    raise NotImplementedError()


class Device(object):
  """Interface contract for devices enumerated by a backend."""

  def __init__(self, backend, settings=None):
    self.backend = backend
    # Initialize with empty settings if not required by the overriding device.
    self.settings = settings or Settings()

  def Initialize(self):
    """Called before anything else, for initial provisioning."""
    raise NotImplementedError()

  def IsNativeTracingEnabled(self):
    """Check if the device is ready to capture native allocation traces."""
    raise NotImplementedError()

  def EnableNativeTracing(self, enabled):
    """Provision the device and make it ready to trace native allocations."""
    raise NotImplementedError()

  def ListProcesses(self):
    """Returns a sequence of |Process|."""
    raise NotImplementedError()

  def GetProcess(self, pid):
    """Returns an instance of |Process| or None (if not found)."""
    raise NotImplementedError()

  def GetStats(self):
    """Returns an instance of |DeviceStats|."""
    raise NotImplementedError()

  @property
  def name(self):
    """Friendly name of the target device (e.g., phone model)."""
    raise NotImplementedError()

  @property
  def id(self):
    """Unique identifier (within the backend) of the device (e.g., S/N)."""
    raise NotImplementedError()


class Process(object):
  """Interface contract for each running process."""

  def __init__(self, device, pid, name):
    assert(isinstance(device, Device))
    assert(isinstance(pid, int))
    self.device = device
    self.pid = pid
    self.name = name

  def DumpMemoryMaps(self):
    """Returns an instance of |memory_map.Map|."""
    raise NotImplementedError()

  def DumpNativeHeap(self):
    """Returns an instance of |native_heap.NativeHeap|."""
    raise NotImplementedError()

  def Freeze(self):
    """Stops the process and all its threads."""
    raise NotImplementedError()

  def Unfreeze(self):
    """Resumes the process."""
    raise NotImplementedError()

  def GetStats(self):
    """Returns an instance of |ProcessStats|."""
    raise NotImplementedError()

  def __str__(self):
    return '[%d] %s' % (self.pid, self.name)


class DeviceStats(object):
  """CPU/Memory stats for a |Device|."""

  def __init__(self, uptime, cpu_times, memory_stats):
    """Args:
      uptime: uptime in seconds.
      cpu_times: array (CPUs) of dicts (cpu times since last call).
          e.g., [{'User': 10, 'System': 80, 'Idle': 10}, ... ]
      memory_stats: Dictionary of memory stats. e.g., {'Free': 1, 'Cached': 10}
    """
    assert(isinstance(cpu_times, list) and isinstance(cpu_times[0], dict))
    assert(isinstance(memory_stats, dict))
    self.uptime = uptime
    self.cpu_times = cpu_times
    self.memory_stats = memory_stats


class ProcessStats(object):
  """CPU/Memory stats for a |Process|."""

  def __init__(self, threads, run_time, cpu_usage, vm_rss, page_faults):
    """Args:
      threads: Number of threads.
      run_time: Total process uptime in seconds.
      cpu_usage: CPU usage [0-100] since the last GetStats call.
      vm_rss_kb: Resident Memory Set in Kb.
      page_faults: Number of VM page faults (hard + soft).
    """
    self.threads = threads
    self.run_time = run_time
    self.cpu_usage = cpu_usage
    self.vm_rss = vm_rss
    self.page_faults = page_faults


class Settings(object):
  """Models user-definable settings for backends and devices."""

  def __init__(self, expected_keys=None):
    """Args:
      expected_keys: A dict. (key-name -> description) of expected settings
    """
    self.expected_keys = expected_keys or {}
    self.values = dict((k, '') for k in self.expected_keys.iterkeys())

  def __getitem__(self, key):
    assert(key in self.expected_keys), 'Unexpected setting: ' + key
    return self.values.get(key)

  def __setitem__(self, key, value):
    assert(key in self.expected_keys), 'Unexpected setting: ' + key
    self.values[key] = value
