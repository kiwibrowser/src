# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for systeminfo."""

from __future__ import print_function

import collections
import io
import mock
import os
import time

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.mobmonitor.system import systeminfo

# Strings that are used to mock particular system files for testing.
MOCK_PROC_MEMINFO = u'''
MemTotal:       65897588 kB
MemFree:        24802380 kB
Buffers:         1867288 kB
Cached:         29458948 kB
SwapCached:            0 kB
Active:         34858204 kB
Inactive:        3662692 kB
Active(anon):    7223176 kB
Inactive(anon):   136892 kB
Active(file):   27635028 kB
Inactive(file):  3525800 kB
Unevictable:       27268 kB
Mlocked:           27268 kB
SwapTotal:      67035132 kB
SwapFree:       67035132 kB
Dirty:               304 kB
Writeback:             0 kB
AnonPages:       7221488 kB
Mapped:           593360 kB
Shmem:            139136 kB
Slab:            1740892 kB
SReclaimable:    1539144 kB
SUnreclaim:       201748 kB
KernelStack:       21384 kB
PageTables:       103680 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:    99983924 kB
Committed_AS:   19670964 kB
VmallocTotal:   34359738367 kB
VmallocUsed:      359600 kB
VmallocChunk:   34325703384 kB
HardwareCorrupted:     0 kB
AnonHugePages:   2924544 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
DirectMap4k:     1619840 kB
DirectMap2M:    46536704 kB
DirectMap1G:    18874368 kB
'''

MOCK_PROC_MOUNTS = u'''
rootfs / rootfs rw 0 0
sysfs /sys sysfs rw,nosuid,nodev,noexec,relatime 0 0
proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0
udev /dev devtmpfs rw,relatime,size=32935004k,nr_inodes=8233751,mode=755 0 0
devpts /dev/pts devpts rw,nosuid,noexec,relatime,gid=5,mode=620,ptmxmode=000 0 0
tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=6589760k,mode=755 0 0
securityfs /sys/kernel/security securityfs rw,relatime 0 0
/dev/mapper/dhcp--100--106--128--134--vg-root / ext4 rw,relatime,errors=remount-ro,i_version,data=ordered 0 0
none /sys/fs/cgroup tmpfs rw,relatime,size=4k,mode=755 0 0
none /sys/fs/fuse/connections fusectl rw,relatime 0 0
none /sys/kernel/debug debugfs rw,relatime 0 0
none /run/lock tmpfs rw,nosuid,nodev,noexec,relatime,size=5120k 0 0
none /run/shm tmpfs rw,nosuid,nodev,relatime 0 0
none /run/user tmpfs rw,nosuid,nodev,noexec,relatime,size=102400k,mode=755 0 0
none /sys/fs/pstore pstore rw,relatime 0 0
/dev/sdb1 /work ext4 rw,relatime,data=ordered 0 0
/dev/sda1 /boot ext2 rw,relatime,i_version,stripe=4 0 0
none /dev/cgroup/cpu cgroup rw,relatime,cpuacct,cpu 0 0
none /dev/cgroup/devices cgroup rw,relatime,devices 0 0
/dev/sdb1 /usr/local/autotest ext4 rw,relatime,data=ordered 0 0
rpc_pipefs /run/rpc_pipefs rpc_pipefs rw,relatime 0 0
/dev/mapper/dhcp--100--106--128--134--vg-usr+local+google /usr/local/google ext4 rw,relatime,i_version,data=writeback 0 0
binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw,nosuid,nodev,noexec,relatime 0 0
systemd /sys/fs/cgroup/systemd cgroup rw,nosuid,nodev,noexec,relatime,name=systemd 0 0
/etc/auto.auto /auto autofs rw,relatime,fd=7,pgrp=2440,timeout=600,minproto=5,maxproto=5,indirect 0 0
/etc/auto.home /home autofs rw,relatime,fd=13,pgrp=2440,timeout=600,minproto=5,maxproto=5,indirect 0 0
srcfsd /google/src fuse.srcfsd rw,nosuid,nodev,relatime,user_id=906,group_id=65534,default_permissions,allow_other 0 0
/dev/mapper/dhcp--100--106--128--134--vg-root /home/build ext4 rw,relatime,errors=remount-ro,i_version,data=ordered 0 0
/dev/mapper/dhcp--100--106--128--134--vg-root /auto/buildstatic ext4 rw,relatime,errors=remount-ro,i_version,data=ordered 0 0
cros /sys/fs/cgroup/cros cgroup rw,relatime,cpuset 0 0
gvfsd-fuse /run/user/307266/gvfs fuse.gvfsd-fuse rw,nosuid,nodev,relatime,user_id=307266,group_id=5000 0 0
objfsd /google/obj fuse.objfsd rw,nosuid,nodev,relatime,user_id=903,group_id=65534,default_permissions,allow_other 0 0
x20fsd /google/data fuse.x20fsd rw,nosuid,nodev,relatime,user_id=904,group_id=65534,allow_other 0 0
'''

MOCK_PROC_FILESYSTEMS = u'''
nodev sysfs
nodev rootfs
nodev ramfs
nodev bdev
nodev proc
nodev cgroup
nodev cpuset
nodev tmpfs
nodev devtmpfs
nodev debugfs
nodev securityfs
nodev sockfs
nodev pipefs
nodev anon_inodefs
nodev devpts
      ext3
      ext2
      ext4
nodev hugetlbfs
      vfat
nodev ecryptfs
      fuseblk
nodev fuse
nodev fusectl
nodev pstore
nodev mqueue
nodev rpc_pipefs
nodev nfs
nodev nfs4
nodev nfsd
nodev binfmt_misc
nodev autofs
      xfs
      jfs
      msdos
      ntfs
      minix
      hfs
      hfsplus
      qnx4
      ufs
      btrfs
'''

MOCK_PROC_STAT = u'''
cpu  36790888 1263256 11740848 7297360559 3637601 444 116964 0 0 0
intr 3386552889 27 3 0 0 0 0 0 0 1 0 0 0 4 0 0 0 33 26 0 0 0 0 0 3508578 0 0 0
ctxt 9855640605
btime 1431536959
processes 3593799
procs_running 2
procs_blocked 0
softirq 652938841 3508230 208834832 49297758 52442647 16919039 0 47196001 147658748 3188900 123892686
'''


class SystemInfoStorageTest(cros_test_lib.MockTestCase):
  """Unittests for SystemInfoStorage."""

  def _CreateSystemInfoStorage(self, update_sec):
    """Setup a SystemInfoStorage object."""
    return systeminfo.SystemInfoStorage(update_sec=update_sec)

  def testUpdate(self):
    """Test SystemInfoStorage Update."""
    si = self._CreateSystemInfoStorage(1)
    si.Update('test', 'testvalue')
    self.assertEquals(si.resources.get('test'), 'testvalue')

  def testNeedToUpdateNoData(self):
    """Test NeedToUpdate with no data."""
    si = self._CreateSystemInfoStorage(1)
    self.assertEquals(si.NeedToUpdate('test'), True)

  def testNeedToUpdateNoUpdateTime(self):
    """Test NeedToUpdate with data that has no update time recorded."""
    si = self._CreateSystemInfoStorage(1)
    dataname, data = ('testname', 'testvalue')
    si.resources[dataname] = data
    self.assertEquals(si.NeedToUpdate(dataname), True)

  def testNeedToUpdateStaleData(self):
    """Test NeedToUpdate with data that is out of date."""
    si = self._CreateSystemInfoStorage(1)

    dataname, data = ('testname', 'testvalue')
    curtime = time.time()

    si.resources[dataname] = data
    si.update_times[dataname] = curtime

    self.StartPatcher(mock.patch('time.time'))
    time.time.return_value = curtime + 2

    self.assertEquals(si.NeedToUpdate(dataname), True)

  def testNeedToUpdateFreshData(self):
    """Test NeedToUpdate with data that is not out of date."""
    si = self._CreateSystemInfoStorage(1)

    dataname, data = ('testname', 'testvalue')
    curtime = time.time()

    si.resources[dataname] = data
    si.update_times[dataname] = curtime

    self.StartPatcher(mock.patch('time.time'))
    time.time.return_value = curtime + 0.5

    self.assertEquals(si.NeedToUpdate(dataname), False)


class MemoryTest(cros_test_lib.MockTestCase):
  """Unittests for Memory."""

  def _CreateMemory(self, update_sec):
    """Setup a Memory object."""
    return systeminfo.Memory(update_sec=update_sec)

  def testMemoryExisting(self):
    """Test memory information collection when a record exists."""
    mem = self._CreateMemory(1)

    dataname, data = (systeminfo.RESOURCENAME_MEMORY, 'testvalue')
    mem.Update(dataname, data)

    self.assertEquals(mem.MemoryUsage(), 'testvalue')

  def testMemory(self):
    """Test memory info when there is no record, or record is stale."""
    mem = self._CreateMemory(1)

    mock_file = io.StringIO(MOCK_PROC_MEMINFO)
    with mock.patch('__builtin__.open', return_value=mock_file, create=True):
      mem.MemoryUsage()

    # The correct values that mem.MemoryUsage() should produce based on the
    # mocked /proc/meminfo file.
    mock_memtotal = 67479130112
    mock_available = 57475702784
    mock_percent_used = 14.824475821482267

    mock_memory = systeminfo.RESOURCE_MEMORY(mock_memtotal, mock_available,
                                             mock_percent_used)

    self.assertEquals(mem.resources.get(systeminfo.RESOURCENAME_MEMORY),
                      mock_memory)


class DiskTest(cros_test_lib.MockTestCase):
  """Unittests for Disk."""

  def _CreateDisk(self, update_sec):
    """Setup a Disk object."""
    return systeminfo.Disk(update_sec=update_sec)

  def testDiskPartitionsExisting(self):
    """Test disk partition information collection when a record exists."""
    disk = self._CreateDisk(1)

    dataname, data = (systeminfo.RESOURCENAME_DISKPARTITIONS, 'testvalue')
    disk.Update(dataname, data)

    self.assertEquals(disk.DiskPartitions(), 'testvalue')

  def testDiskPartitions(self):
    """Test disk partition info when there is no record, or record is stale."""
    disk = self._CreateDisk(1)

    mock_mounts_file = io.StringIO(MOCK_PROC_MOUNTS)
    mock_filesystems_file = io.StringIO(MOCK_PROC_FILESYSTEMS)

    def _file_returner(fname, _mode):
      if systeminfo.SYSTEMFILE_PROC_MOUNTS == fname:
        return mock_mounts_file
      elif systeminfo.SYSTEMFILE_PROC_FILESYSTEMS == fname:
        return mock_filesystems_file

    with mock.patch('__builtin__.open', side_effect=_file_returner,
                    create=True):
      disk.DiskPartitions()

    # The expected return value.
    mock_disk_partitions = [
        systeminfo.RESOURCE_DISKPARTITION(
            '/dev/mapper/dhcp--100--106--128--134--vg-root', '/', 'ext4'),
        systeminfo.RESOURCE_DISKPARTITION('/dev/sdb1', '/work', 'ext4'),
        systeminfo.RESOURCE_DISKPARTITION('/dev/sda1', '/boot', 'ext2'),
        systeminfo.RESOURCE_DISKPARTITION(
            '/dev/sdb1', '/usr/local/autotest', 'ext4'),
        systeminfo.RESOURCE_DISKPARTITION(
            '/dev/mapper/dhcp--100--106--128--134--vg-usr+local+google',
            '/usr/local/google', 'ext4'),
        systeminfo.RESOURCE_DISKPARTITION(
            '/dev/mapper/dhcp--100--106--128--134--vg-root',
            '/home/build', 'ext4'),
        systeminfo.RESOURCE_DISKPARTITION(
            '/dev/mapper/dhcp--100--106--128--134--vg-root',
            '/auto/buildstatic', 'ext4')]

    self.assertEquals(
        disk.resources.get(systeminfo.RESOURCENAME_DISKPARTITIONS),
        mock_disk_partitions)

  def testDiskUsageExisting(self):
    """Test disk usage information collection when a record exists."""
    disk = self._CreateDisk(1)

    partition = 'fakepartition'
    dataname = '%s:%s' % (systeminfo.RESOURCENAME_DISKUSAGE, partition)
    data = 'testvalue'

    disk.Update(dataname, data)

    self.assertEquals(disk.DiskUsage(partition), 'testvalue')

  def testDiskUsage(self):
    """Test disk usage info when there is no record or record is stale."""
    disk = self._CreateDisk(1)

    partition = 'fakepartition'

    # Mock value for test. These values came from statvfs'ing root.
    mock_statvfs_return = collections.namedtuple(
        'mock_disk', ['f_bsize', 'f_frsize', 'f_blocks', 'f_bfree', 'f_bavail',
                      'f_files', 'f_ffree', 'f_favail', 'f_flag', 'f_namemax'])
    mock_value = mock_statvfs_return(4096, 4096, 9578876, 5332865, 4840526,
                                     2441216, 2079830, 2079830, 4096, 255)

    self.StartPatcher(mock.patch('os.statvfs'))
    os.statvfs.return_value = mock_value

    # Expected results of the test.
    mock_total = 39235076096
    mock_free = 21843415040
    mock_used = 17391661056
    mock_percent_used = 44.326818720693325
    mock_diskusage = systeminfo.RESOURCE_DISKUSAGE(mock_total, mock_used,
                                                   mock_free,
                                                   mock_percent_used)

    self.assertEquals(disk.DiskUsage(partition), mock_diskusage)

  def testBlockDevicesExisting(self):
    """Test block device information collection when a record exists."""
    disk = self._CreateDisk(1)

    device = '/dev/sda1'
    dataname = '%s:%s' % (systeminfo.RESOURCENAME_BLOCKDEVICE, device)
    data = 'testvalue'

    disk.Update(dataname, data)

    self.assertEquals(disk.BlockDevices(device), data)

  def testBlockDevice(self):
    """Test block device info when there is no record or a record is stale."""
    disk = self._CreateDisk(1)

    mock_device = '/dev/sda1'
    mock_size = 12345678987654321
    mock_ids = ['ata-ST1000DM003-1ER162_Z4Y3WQDB-part1']
    mock_labels = ['BOOT-PARTITION']
    mock_lsblk = 'NAME="sda1" RM="0" TYPE="part" SIZE="%s"' % mock_size

    self.StartPatcher(mock.patch('chromite.lib.osutils.ResolveSymlink'))
    osutils.ResolveSymlink.return_value = '/dev/sda1'

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(
          partial_mock.In(systeminfo.SYSTEMFILE_DEV_DISKBY['ids']),
          output='\n'.join(mock_ids))
      rc_mock.AddCmdResult(
          partial_mock.In(systeminfo.SYSTEMFILE_DEV_DISKBY['labels']),
          output='\n'.join(mock_labels))
      rc_mock.AddCmdResult(partial_mock.In('lsblk'), output=mock_lsblk)

      mock_blockdevice = [systeminfo.RESOURCE_BLOCKDEVICE(mock_device,
                                                          mock_size,
                                                          mock_ids,
                                                          mock_labels)]

      self.assertEquals(disk.BlockDevices(mock_device), mock_blockdevice)


class Cpu(cros_test_lib.MockTestCase):
  """Unittests for Cpu."""

  def _CreateCpu(self, update_sec):
    """Setup a Cpu object."""
    return systeminfo.Cpu(update_sec=update_sec)

  def testCpuTimeExisting(self):
    """Test cpu timing information collection when a record exists."""
    cpu = self._CreateCpu(1)

    dataname, data = (systeminfo.RESOURCENAME_CPUTIMES, 'testvalue')
    cpu.Update(dataname, data)

    self.assertEquals(cpu.CpuTime(), 'testvalue')

  def testCpuTime(self):
    """Test cpu timing info when there is no record or record is stale."""
    cpu = self._CreateCpu(1)

    mock_file = io.StringIO(MOCK_PROC_STAT)
    with mock.patch('__builtin__.open', return_value=mock_file, create=True):
      cpu.CpuTime()

    # The expected return value.
    mock_cpu_name = 'cpu'
    mock_cpu_total = 7350910560
    mock_cpu_idle = 7300998160
    mock_cpu_nonidle = 49912400

    mock_cputimes = [systeminfo.RESOURCE_CPUTIME(mock_cpu_name, mock_cpu_total,
                                                 mock_cpu_idle,
                                                 mock_cpu_nonidle)]

    self.assertEquals(cpu.resources.get(systeminfo.RESOURCENAME_CPUTIMES),
                      mock_cputimes)

  def testCpuLoadExisting(self):
    """Test cpu load information collection when a record exists."""
    cpu = self._CreateCpu(1)

    dataname, data = (systeminfo.RESOURCENAME_CPULOADS, 'testvalue')
    cpu.Update(dataname, data)

    self.assertEquals(cpu.CpuLoad(), 'testvalue')

  def testCpuLoad(self):
    """Test cpu load collection info when we have previous timing data."""
    cpu = self._CreateCpu(1)

    # Create a cpu timing record.
    mock_cpu_prev_name = 'cpu'
    mock_cpu_prev_total = 7340409560
    mock_cpu_prev_idle = 7300497160
    mock_cpu_prev_nonidle = 39912400

    mock_cputimes_prev = [systeminfo.RESOURCE_CPUTIME(mock_cpu_prev_name,
                                                      mock_cpu_prev_total,
                                                      mock_cpu_prev_idle,
                                                      mock_cpu_prev_nonidle)]

    cpu.Update(systeminfo.RESOURCENAME_CPUPREVTIMES, mock_cputimes_prev)

    # Mock and execute.
    mock_file = io.StringIO(MOCK_PROC_STAT)
    with mock.patch('__builtin__.open', return_value=mock_file, create=True):
      cpu.CpuLoad()

    # The expected return value.
    mock_cpu_name = 'cpu'
    mock_cpu_load = 0.9522902580706599

    mock_cpuloads = [systeminfo.RESOURCE_CPULOAD(mock_cpu_name, mock_cpu_load)]

    self.assertEquals(cpu.resources.get(systeminfo.RESOURCENAME_CPULOADS),
                      mock_cpuloads)


class InfoClassCacheTest(cros_test_lib.MockTestCase):
  """Unittests for checking that information class caching."""

  def testGetCpu(self):
    """Test caching explicitly for Cpu information objects."""
    cpus1 = [systeminfo.GetCpu(), systeminfo.GetCpu(),
             systeminfo.GetCpu(systeminfo.UPDATE_CPU_SEC),
             systeminfo.GetCpu(update_sec=systeminfo.UPDATE_CPU_SEC)]

    cpus2 = [systeminfo.GetCpu(10), systeminfo.GetCpu(10),
             systeminfo.GetCpu(update_sec=10)]

    self.assertTrue(all(id(x) == id(cpus1[0]) for x in cpus1))
    self.assertTrue(all(id(x) == id(cpus2[0]) for x in cpus2))
    self.assertNotEqual(id(cpus1[0]), id(cpus2[0]))

  def testGetMemory(self):
    """Test caching explicitly for Memory information objects."""
    mems1 = [systeminfo.GetMemory(), systeminfo.GetMemory(),
             systeminfo.GetMemory(systeminfo.UPDATE_MEMORY_SEC),
             systeminfo.GetMemory(update_sec=systeminfo.UPDATE_MEMORY_SEC)]

    mems2 = [systeminfo.GetMemory(10), systeminfo.GetMemory(10),
             systeminfo.GetMemory(update_sec=10)]

    self.assertTrue(all(id(x) == id(mems1[0]) for x in mems1))
    self.assertTrue(all(id(x) == id(mems2[0]) for x in mems2))
    self.assertNotEqual(id(mems1[0]), id(mems2[0]))

  def testGetDisk(self):
    """Test caching explicitly for Disk information objects."""
    disks1 = [systeminfo.GetDisk(), systeminfo.GetDisk(),
              systeminfo.GetDisk(systeminfo.UPDATE_MEMORY_SEC),
              systeminfo.GetDisk(update_sec=systeminfo.UPDATE_MEMORY_SEC)]

    disks2 = [systeminfo.GetDisk(10), systeminfo.GetDisk(10),
              systeminfo.GetDisk(update_sec=10)]

    self.assertTrue(all(id(x) == id(disks1[0]) for x in disks1))
    self.assertTrue(all(id(x) == id(disks2[0]) for x in disks2))
    self.assertNotEqual(id(disks1[0]), id(disks2[0]))
