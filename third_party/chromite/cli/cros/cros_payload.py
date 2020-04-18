# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros payload: Show information about an update payload."""

from __future__ import print_function

import itertools
import sys
import textwrap

from chromite.lib import constants
from chromite.cli import command

# Needed for the update_payload import below.
sys.path.insert(0, constants.UPDATE_ENGINE_SCRIPTS_PATH)

MAJOR_PAYLOAD_VERSION_CHROMEOS = 1
MAJOR_PAYLOAD_VERSION_BRILLO = 2

def DisplayValue(key, value):
  """Print out a key, value pair with values left-aligned."""
  if value != None:
    print('%-*s %s' % (24, key + ':', value))
  else:
    raise ValueError('Cannot display an empty value.')


def DisplayHexData(data, indent=0):
  """Print out binary data as a hex values."""
  for off in range(0, len(data), 16):
    chunk = data[off:off + 16]
    print(' ' * indent +
          ' '.join('%.2x' % ord(c) for c in chunk) +
          '   ' * (16 - len(chunk)) +
          ' | ' +
          ''.join(c if 32 <= ord(c) and ord(c) < 127 else '.' for c in chunk))


@command.CommandDecorator('payload')
class PayloadCommand(command.CliCommand):
  """Show basic information about an update payload.

  This command parses an update payload and displays information from
  its header and manifest.
  """

  EPILOG = """
Example:
  cros payload show chromeos_6716.0.0_daisy_canary-channel_full_snow-mp-v3.bin
"""

  def __init__(self, options):
    # TODO(alliewood)(chromium:454629) update once update_payload is moved
    # into chromite. google.protobuf may not be available outside the chroot.
    import update_payload
    self._update_payload = update_payload

    super(PayloadCommand, self).__init__(options)
    self.payload = None

  @classmethod
  def AddParser(cls, parser):
    super(PayloadCommand, cls).AddParser(parser)
    parser.add_argument(
        'action', choices=['show'],
        help='Show information about an update payload.')
    parser.add_argument(
        'payload_file', type=file,
        help='The payload file that you want information from.')
    parser.add_argument('--list_ops', default=False, action='store_true',
                        help='List the install operations and their extents.')
    parser.add_argument('--stats', default=False, action='store_true',
                        help='Show information about overall input/output.')
    parser.add_argument('--signatures', default=False, action='store_true',
                        help='Show signatures stored in the payload.')

  def _DisplayHeader(self):
    """Show information from the payload header."""
    header = self.payload.header
    DisplayValue('Payload version', header.version)
    DisplayValue('Manifest length', header.manifest_len)

  def _DisplayManifest(self):
    """Show information from the payload manifest."""
    manifest = self.payload.manifest
    if self.payload.header.version == MAJOR_PAYLOAD_VERSION_BRILLO:
      DisplayValue('Number of partitions', len(manifest.partitions))
      for partition in manifest.partitions:
        DisplayValue('  Number of "%s" ops' % partition.partition_name,
                     len(partition.operations))
    else:
      DisplayValue('Number of operations', len(manifest.install_operations))
      DisplayValue('Number of kernel ops',
                   len(manifest.kernel_install_operations))
    DisplayValue('Block size', manifest.block_size)
    DisplayValue('Minor version', manifest.minor_version)

  def _DisplaySignatures(self):
    """Show information about the signatures from the manifest."""
    header = self.payload.header
    if header.metadata_signature_len:
      offset = header.size + header.manifest_len
      DisplayValue('Metadata signatures blob',
                   'file_offset=%d (%d bytes)' %
                   (offset, header.metadata_signature_len))
      signatures_blob = self.payload.ReadDataBlob(
          -header.metadata_signature_len,
          header.metadata_signature_len)
      self._DisplaySignaturesBlob('Metadata', signatures_blob)
    else:
      print('No metadata signatures stored in the payload')

    manifest = self.payload.manifest
    if manifest.HasField('signatures_offset'):
      signature_msg = 'blob_offset=%d' % manifest.signatures_offset
      if manifest.signatures_size:
        signature_msg += ' (%d bytes)' % manifest.signatures_size
      DisplayValue('Payload signatures blob', signature_msg)
      signatures_blob = self.payload.ReadDataBlob(manifest.signatures_offset,
                                                  manifest.signatures_size)
      self._DisplaySignaturesBlob('Payload', signatures_blob)
    else:
      print('No payload signatures stored in the payload')

  @staticmethod
  def _DisplaySignaturesBlob(signature_name, signatures_blob):
    from update_payload import update_metadata_pb2
    signatures = update_metadata_pb2.Signatures()
    signatures.ParseFromString(signatures_blob)
    print('%s signatures: (%d entries)' %
          (signature_name, len(signatures.signatures)))
    for signature in signatures.signatures:
      print('  version=%s, hex_data: (%d bytes)' %
            (signature.version if signature.HasField('version') else None,
             len(signature.data)))
      DisplayHexData(signature.data, indent=4)


  def _DisplayOps(self, name, operations):
    """Show information about the install operations from the manifest.

    The list shown includes operation type, data offset, data length, source
    extents, source length, destination extents, and destinations length.

    Args:
      name: The name you want displayed above the operation table.
      operations: The install_operations object that you want to display
                  information about.
    """
    def _DisplayExtents(extents, name):
      num_blocks = sum([ext.num_blocks for ext in extents])
      ext_str = ' '.join(
          '(%s,%s)' % (ext.start_block, ext.num_blocks) for ext in extents)
      # Make extent list wrap around at 80 chars.
      ext_str = '\n      '.join(textwrap.wrap(ext_str, 74))
      extent_plural = 's' if len(extents) > 1 else ''
      block_plural = 's' if num_blocks > 1 else ''
      print('    %s: %d extent%s (%d block%s)' %
            (name, len(extents), extent_plural, num_blocks, block_plural))
      print('      %s' % ext_str)

    op_dict = self._update_payload.common.OpType.NAMES
    print('%s:' % name)
    for op, op_count in itertools.izip(operations, itertools.count()):
      print('  %d: %s' % (op_count, op_dict[op.type]))
      if op.HasField('data_offset'):
        print('    Data offset: %s' % op.data_offset)
      if op.HasField('data_length'):
        print('    Data length: %s' % op.data_length)
      if len(op.src_extents):
        _DisplayExtents(op.src_extents, 'Source')
      if len(op.dst_extents):
        _DisplayExtents(op.dst_extents, 'Destination')

  def _GetStats(self, manifest):
    """Returns various statistics about a payload file.

    Returns a dictionary containing the number of blocks read during payload
    application, the number of blocks written, and the number of seeks done
    when writing during operation application.
    """
    read_blocks = 0
    written_blocks = 0
    num_write_seeks = 0
    if self.payload.header.version == MAJOR_PAYLOAD_VERSION_BRILLO:
      partitions_operations = [part.operations for part in manifest.partitions]
    else:
      partitions_operations = [manifest.install_operations,
                               manifest.kernel_install_operations]
    for operations in partitions_operations:
      last_ext = None
      for curr_op in operations:
        read_blocks += sum([ext.num_blocks for ext in curr_op.src_extents])
        written_blocks += sum([ext.num_blocks for ext in curr_op.dst_extents])
        for curr_ext in curr_op.dst_extents:
          # See if the extent is contiguous with the last extent seen.
          if last_ext and (curr_ext.start_block !=
                           last_ext.start_block + last_ext.num_blocks):
            num_write_seeks += 1
          last_ext = curr_ext

    if manifest.minor_version == 1:
      # Rootfs and kernel are written during the filesystem copy in version 1.
      written_blocks += manifest.old_rootfs_info.size / manifest.block_size
      written_blocks += manifest.old_kernel_info.size / manifest.block_size
    # Old and new rootfs and kernel are read once during verification
    read_blocks += manifest.old_rootfs_info.size / manifest.block_size
    read_blocks += manifest.old_kernel_info.size / manifest.block_size
    read_blocks += manifest.new_rootfs_info.size / manifest.block_size
    read_blocks += manifest.new_kernel_info.size / manifest.block_size
    stats = {'read_blocks': read_blocks,
             'written_blocks': written_blocks,
             'num_write_seeks': num_write_seeks}
    return stats

  def _DisplayStats(self, manifest):
    stats = self._GetStats(manifest)
    DisplayValue('Blocks read', stats['read_blocks'])
    DisplayValue('Blocks written', stats['written_blocks'])
    DisplayValue('Seeks when writing', stats['num_write_seeks'])

  def Run(self):
    """Parse the update payload and display information from it."""
    self.payload = self._update_payload.Payload(self.options.payload_file)
    self.payload.Init()
    self._DisplayHeader()
    self._DisplayManifest()
    if self.options.signatures:
      self._DisplaySignatures()
    if self.options.stats:
      self._DisplayStats(self.payload.manifest)
    if self.options.list_ops:
      print()
      if self.payload.header.version == MAJOR_PAYLOAD_VERSION_BRILLO:
        for partition in self.payload.manifest.partitions:
          self._DisplayOps('%s install operations' % partition.partition_name,
                           partition.operations)
      else:
        self._DisplayOps('Install operations',
                         self.payload.manifest.install_operations)
        self._DisplayOps('Kernel install operations',
                         self.payload.manifest.kernel_install_operations)
