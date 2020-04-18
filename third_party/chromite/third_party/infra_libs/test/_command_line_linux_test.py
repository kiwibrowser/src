import sys
import unittest

from infra_libs import command_line


@unittest.skipUnless(sys.platform == 'linux2', 'Only supported on Linux')
class CommandLineTest(unittest.TestCase):
  @classmethod
  def read_cmdline(cls):  # pragma: no cover
    with open('/proc/self/cmdline') as fh:
      return fh.read()

  @classmethod
  def setUpClass(cls):  # pragma: no cover
    if sys.platform != 'linux2':
      return

    cls.original = cls.read_cmdline()
    cls.original_len = len(cls.original)

  @classmethod
  def tearDownClass(cls):  # pragma: no cover
    if sys.platform != 'linux2':
      return

    command_line.set_command_line(cls.original)

  def test_set(self):  # pragma: no cover
    # Shorter than the original.
    cmdline = 'x' * (self.original_len - 1)
    command_line.set_command_line(cmdline)
    self.assertEqual(cmdline, self.read_cmdline().rstrip('\0'))

    # Same length as the original.
    cmdline = 'x' * self.original_len
    command_line.set_command_line(cmdline)
    self.assertEqual(cmdline[:-1], self.read_cmdline().rstrip('\0'))

    # Longer than the original.
    cmdline = 'x' * (self.original_len + 1)
    command_line.set_command_line(cmdline)
    self.assertEqual(cmdline[:-2], self.read_cmdline().rstrip('\0'))
