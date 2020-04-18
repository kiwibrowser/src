#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Optional usage logging for Software Construction Toolkit."""

import atexit
import os
import platform
import sys
import time
import xml.dom
import SCons
import SCons.Script


chain_build_targets = None      # Previous SCons _build_targets function

#------------------------------------------------------------------------------
# Wrappers and hooks into SCons


class ProgressDisplayWrapper(object):
  """Wrapper around SCons.Util.DisplayEngine.

  Needs to be has-a not is-a, since DisplayEngine.set_mode() overrides the
  __call__ member.
  """

  def __init__(self, old_display):
    """Constructor.

    Args:
      old_display: Old display object to chain to.
    """
    self.old_display = old_display

  def __call__(self, text, append_newline=1):
    """Display progress.

    Args:
      text: Text to display.
      append_newline: Append newline to text if non-zero.

    Returns:
      Passthru from old display object.
    """
    log.AddEntry('progress %s' % text)
    return self.old_display(text, append_newline)

  def set_mode(self, mode):
    """Passthru to DisplayEngine.setmode().

    Args:
      mode: If non-zero, print progress.

    Returns:
      Passthru from old display object.
    """
    return self.old_display.set_mode(mode)


def BuildTargetsWrapper(fs, options, targets, target_top):
  """Wrapper around SCons.Script.Main._build_targets().

  Args:
    fs: Filesystem object.
    options: SCons options (after modification by SConscripts.
    targets: Targets to build.
    target_top: Passed through to _build_targets().
  """
  log.AddEntry('build_targets start')
  log.SetParam('build_targets.targets', map(str, targets))

  # Get list of non-default options.  SConscript settings override defaults.
  build_opts = dict(options.__SConscript_settings__)
  # Command line settings are direct attrs, and override SConscript settings.
  for key in dir(options):
    if key.startswith('__') or key == 'settable':
      continue
    value = getattr(options, key)
    if callable(value):
      continue
    build_opts[key] = value

  for key, value in build_opts.items():
    log.SetParam('build_targets.option.%s' % key, value)

  try:
    returnval = None
    if chain_build_targets:
      returnval = chain_build_targets(fs, options, targets, target_top)
    return returnval
  finally:
    log.AddEntry('build_targets done')


def PrecmdWrapper(self, line):
  """Pre-command handler for SCons.Script.Interactive() to support logging.

  Args:
    self: cmd object.
    line: Command line which will be executed.

  Returns:
    Passthru value of line.
  """
  log.AddEntry('Interactive start')
  log.SetParam('interactive.command', line or self.lastcmd)
  return line


def PostcmdWrapper(self, stop, line):
  """Post-command handler for SCons.Script.Interactive() to support logging.

  Args:
    self: cmd object.
    stop: Will execution stop after this function exits?
    line: Command line which was executed.

  Returns:
    Passthru value of stop.
  """
  log.AddEntry('Interactive done')
  log.Dump()
  return stop


#------------------------------------------------------------------------------
# Usage log object


class Log(object):
  """Usage log object."""

  def __init__(self):
    """Constructor."""
    self.params = {}
    self.entries = []
    self.dump_writer = None
    self.time = time.time

  def SetParam(self, key, value):
    """Sets a parameter.

    Args:
      key: Parameter name (string).
      value: Value for parameter.
    """
    self.params[key] = value

  def AddEntry(self, text):
    """Adds a timestamped log entry.

    Args:
      text: Text of log entry.
    """
    self.entries.append((self.time(), text))

  def ConvertToXml(self):
    """Converts the usage log to XML.

    Returns:
      An xml.dom.minidom.Document object with the usage log contents.
    """
    xml_impl = xml.dom.getDOMImplementation()
    xml_doc = xml_impl.createDocument(None, 'usage_log', None)

    # List build params
    xml_param_list = xml_doc.createElement('param_list')
    xml_doc.documentElement.appendChild(xml_param_list)
    for key in sorted(self.params):
      xml_param = xml_doc.createElement('param')
      xml_param.setAttribute('name', str(key))
      xml_param_list.appendChild(xml_param)

      value = self.params[key]
      if hasattr(value, '__iter__'):
        # Iterable value, so list items
        for v in value:
          xml_item = xml_doc.createElement('item')
          xml_item.setAttribute('value', str(v))
          xml_param.appendChild(xml_item)
      else:
        # Non-iterable, so convert to string
        xml_param.setAttribute('value', str(value))

    # List log entries
    xml_entry_list = xml_doc.createElement('entry_list')
    xml_doc.documentElement.appendChild(xml_entry_list)
    for entry_time, entry_text in self.entries:
      xml_entry = xml_doc.createElement('entry')
      xml_entry.setAttribute('time', str(entry_time))
      xml_entry.setAttribute('text', str(entry_text))
      xml_entry_list.appendChild(xml_entry)

    return xml_doc

  def Dump(self):
    """Dumps the log by calling self.dump_writer(), then clears the log."""
    if self.dump_writer:
      self.dump_writer(self)

    # Clear log entries (but not params, since they can be used again if SCons
    # is in interactive mode).
    self.entries = []


  def SetOutputFile(self, filename):
    """Sets the output filename for usage log dumps.

    Args:
      filename: Name of output file.
    """
    self.dump_to_file = filename
    self.dump_writer = FileDumpWriter

#------------------------------------------------------------------------------
# Usage log methods

def AddSystemParams():
  """Prints system stats."""
  log.SetParam('sys.argv', sys.argv)
  log.SetParam('sys.executable', sys.executable)
  log.SetParam('sys.version', sys.version)
  log.SetParam('sys.version_info', sys.version_info)
  log.SetParam('sys.path', sys.path)
  log.SetParam('sys.platform', sys.platform)
  log.SetParam('platform.uname', platform.uname())
  log.SetParam('platform.platform', platform.platform())

  for e in ['PATH', 'INCLUDE', 'LIB', 'HAMMER_OPTS', 'HAMMER_XGE']:
    log.SetParam('shell.%s' % e, os.environ.get(e, ''))

  log.SetParam('scons.version', SCons.__version__)


def AtExit():
  """Usage log cleanup at exit."""
  log.AddEntry('usage_log exit')
  log.Dump()


def AtModuleLoad():
  """Code executed at module load time."""
  AddSystemParams()

  # Wrap SCons' progress display wrapper
  SCons.Script.Main.progress_display = ProgressDisplayWrapper(
      SCons.Script.Main.progress_display)

  # Wrap SCons' _build_targets()
  global chain_build_targets
  chain_build_targets = SCons.Script.Main._build_targets
  SCons.Script.Main._build_targets = BuildTargetsWrapper

  # Hook SCons interactive mode
  SCons.Script.Interactive.SConsInteractiveCmd.precmd = PrecmdWrapper
  SCons.Script.Interactive.SConsInteractiveCmd.postcmd = PostcmdWrapper

  # Make sure we get called at exit
  atexit.register(AtExit)


def FileDumpWriter(log):
  """Dumps the log to the specified file."""
  print 'Writing usage log to %s...' % log.dump_to_file
  f = open(log.dump_to_file, 'wt')
  doc = log.ConvertToXml()
  doc.writexml(f, encoding='UTF-8', addindent='  ', newl='\n')
  doc.unlink()
  f.close()
  print 'Done writing log.'


# Create the initial log (can't do this in AtModuleLoad() without 'global')
log = Log()
log.AddEntry('usage_log loaded')

# Do other work at module load time
AtModuleLoad()
