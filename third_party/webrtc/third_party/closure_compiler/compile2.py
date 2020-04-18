#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs Closure compiler on JavaScript files to check for errors and produce
minified output."""

import argparse
import os
import re
import subprocess
import sys
import tempfile

import processor


_CURRENT_DIR = os.path.join(os.path.dirname(__file__))


class Checker(object):
  """Runs the Closure compiler on given source files to typecheck them
  and produce minified output."""

  _JAR_COMMAND = [
    "java",
    "-jar",
    "-Xms1024m",
    "-client",
    "-XX:+TieredCompilation"
  ]

  _POLYMER_EXTERNS = os.path.join(_CURRENT_DIR, "externs", "polymer-1.0.js")

  def __init__(self, verbose=False):
    """
    Args:
      verbose: Whether this class should output diagnostic messages.
    """
    self._compiler_jar = os.path.join(_CURRENT_DIR, "compiler", "compiler.jar")
    self._target = None
    self._temp_files = []
    self._verbose = verbose

  def _nuke_temp_files(self):
    """Deletes any temp files this class knows about."""
    if not self._temp_files:
      return

    self._log_debug("Deleting temp files: %s" % ", ".join(self._temp_files))
    for f in self._temp_files:
      os.remove(f)
    self._temp_files = []

  def _log_debug(self, msg, error=False):
    """Logs |msg| to stdout if --verbose/-v is passed when invoking this script.

    Args:
      msg: A debug message to log.
    """
    if self._verbose:
      print "(INFO) %s" % msg

  def _log_error(self, msg):
    """Logs |msg| to stderr regardless of --flags.

    Args:
      msg: An error message to log.
    """
    print >> sys.stderr, "(ERROR) %s" % msg

  def run_jar(self, jar, args):
    """Runs a .jar from the command line with arguments.

    Args:
      jar: A file path to a .jar file
      args: A list of command line arguments to be passed when running the .jar.

    Return:
      (exit_code, stderr) The exit code of the command (e.g. 0 for success) and
          the stderr collected while running |jar| (as a string).
    """
    shell_command = " ".join(self._JAR_COMMAND + [jar] + args)
    self._log_debug("Running jar: %s" % shell_command)

    devnull = open(os.devnull, "w")
    kwargs = {"stdout": devnull, "stderr": subprocess.PIPE, "shell": True}
    process = subprocess.Popen(shell_command, **kwargs)
    _, stderr = process.communicate()
    return process.returncode, stderr

  def _get_line_number(self, match):
    """When chrome is built, it preprocesses its JavaScript from:

      <include src="blah.js">
      alert(1);

    to:

      /* contents of blah.js inlined */
      alert(1);

    Because Closure Compiler requires this inlining already be done (as
    <include> isn't valid JavaScript), this script creates temporary files to
    expand all the <include>s.

    When type errors are hit in temporary files, a developer doesn't know the
    original source location to fix. This method maps from /tmp/file:300 back to
    /original/source/file:100 so fixing errors is faster for developers.

    Args:
      match: A re.MatchObject from matching against a line number regex.

    Returns:
      The fixed up /file and :line number.
    """
    real_file = self._processor.get_file_from_line(match.group(1))
    return "%s:%d" % (os.path.abspath(real_file.file), real_file.line_number)

  def _clean_up_error(self, error):
    """Reverse the effects that funky <include> preprocessing steps have on
    errors messages.

    Args:
      error: A Closure compiler error (2 line string with error and source).

    Return:
      The fixed up error string.
    """
    assert self._target
    assert self._expanded_file
    expanded_file = self._expanded_file
    fixed = re.sub("%s:(\d+)" % expanded_file, self._get_line_number, error)
    return fixed.replace(expanded_file, os.path.abspath(self._target))

  def _format_errors(self, errors):
    """Formats Closure compiler errors to easily spot compiler output.

    Args:
      errors: A list of strings extracted from the Closure compiler's output.

    Returns:
      A formatted output string.
    """
    contents = "\n## ".join("\n\n".join(errors).splitlines())
    return "## %s" % contents if contents else ""

  def _create_temp_file(self, contents):
    """Creates an owned temporary file with |contents|.

    Args:
      content: A string of the file contens to write to a temporary file.

    Return:
      The filepath of the newly created, written, and closed temporary file.
    """
    with tempfile.NamedTemporaryFile(mode="wt", delete=False) as tmp_file:
      self._temp_files.append(tmp_file.name)
      tmp_file.write(contents)
    return tmp_file.name

  def check(self, sources, out_file, closure_args=None,
            custom_sources=False, custom_includes=False):
    """Closure compile |sources| while checking for errors.

    Args:
      sources: Files to check. sources[0] is the typically the target file.
          sources[1:] are externs and dependencies in topological order. Order
          is not guaranteed if custom_sources is True.
      out_file: A file where the compiled output is written to.
      closure_args: Arguments passed directly to the Closure compiler.
      custom_sources: Whether |sources| was customized by the target (e.g. not
          in GYP dependency order).
      custom_includes: Whether <include>s are processed when |custom_sources|
          is True.

    Returns:
      (found_errors, stderr) A boolean indicating whether errors were found and
          the raw Closure compiler stderr (as a string).
    """
    is_extern = lambda f: 'externs' in f
    externs_and_deps = [self._POLYMER_EXTERNS]

    if custom_sources:
      if custom_includes:
        # TODO(dbeam): this is fairly hacky. Can we just remove custom_sources
        # soon when all the things kept on life support using it die?
        self._target = sources.pop()
      externs_and_deps += sources
    else:
      self._target = sources[0]
      externs_and_deps += sources[1:]

    externs = filter(is_extern, externs_and_deps)
    deps = filter(lambda f: not is_extern(f), externs_and_deps)

    assert externs or deps or self._target

    self._log_debug("Externs: %s" % externs)
    self._log_debug("Dependencies: %s" % deps)
    self._log_debug("Target: %s" % self._target)

    js_args = deps + ([self._target] if self._target else [])

    process_includes = custom_includes or not custom_sources
    if process_includes:
      # TODO(dbeam): compiler.jar automatically detects "@externs" in a --js arg
      # and moves these files to a different AST tree. However, because we use
      # one big funky <include> meta-file, it thinks all the code is one big
      # externs. Just use --js when <include> dies.

      cwd, tmp_dir = os.getcwd(), tempfile.gettempdir()
      rel_path = lambda f: os.path.join(os.path.relpath(cwd, tmp_dir), f)
      contents = ['<include src="%s">' % rel_path(f) for f in js_args]
      meta_file = self._create_temp_file("\n".join(contents))
      self._log_debug("Meta file: %s" % meta_file)

      self._processor = processor.Processor(meta_file)
      self._expanded_file = self._create_temp_file(self._processor.contents)
      self._log_debug("Expanded file: %s" % self._expanded_file)

      js_args = [self._expanded_file]

    closure_args = closure_args or []
    closure_args += ["summary_detail_level=3", "continue_after_errors"]

    args = ["--externs=%s" % e for e in externs] + \
           ["--js=%s" % s for s in js_args] + \
           ["--%s" % arg for arg in closure_args]

    assert out_file

    out_dir = os.path.dirname(out_file)
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)

    checks_only = 'checks_only' in closure_args

    if not checks_only:
      args += ["--js_output_file=%s" % out_file]

    self._log_debug("Args: %s" % " ".join(args))

    return_code, stderr = self.run_jar(self._compiler_jar, args)

    errors = stderr.strip().split("\n\n")
    maybe_summary = errors.pop()

    summary = re.search("(?P<error_count>\d+).*error.*warning", maybe_summary)
    if summary:
      self._log_debug("Summary: %s" % maybe_summary)
    else:
      # Not a summary. Running the jar failed. Bail.
      self._log_error(stderr)
      self._nuke_temp_files()
      sys.exit(1)

    if summary.group('error_count') != "0":
      if os.path.exists(out_file):
        os.remove(out_file)
    elif checks_only and return_code == 0:
      # Compile succeeded but --checks_only disables --js_output_file from
      # actually writing a file. Write a file ourselves so incremental builds
      # still work.
      with open(out_file, 'w') as f:
        f.write('')

    if process_includes:
      errors = map(self._clean_up_error, errors)
      output = self._format_errors(errors)

      if errors:
        prefix = "\n" if output else ""
        self._log_error("Error in: %s%s%s" % (self._target, prefix, output))
      elif output:
        self._log_debug("Output: %s" % output)

    self._nuke_temp_files()
    return bool(errors) or return_code > 0, stderr


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
      description="Typecheck JavaScript using Closure compiler")
  parser.add_argument("sources", nargs=argparse.ONE_OR_MORE,
                      help="Path to a source file to typecheck")
  parser.add_argument("--custom_sources", action="store_true",
                      help="Whether this rules has custom sources.")
  parser.add_argument("--custom_includes", action="store_true",
                      help="If present, <include>s are processed when "
                           "using --custom_sources.")
  parser.add_argument("-o", "--out_file", required=True,
                      help="A file where the compiled output is written to")
  parser.add_argument("-c", "--closure_args", nargs=argparse.ZERO_OR_MORE,
                      help="Arguments passed directly to the Closure compiler")
  parser.add_argument("-v", "--verbose", action="store_true",
                      help="Show more information as this script runs")
  opts = parser.parse_args()

  checker = Checker(verbose=opts.verbose)

  found_errors, stderr = checker.check(opts.sources, out_file=opts.out_file,
                                       closure_args=opts.closure_args,
                                       custom_sources=opts.custom_sources,
                                       custom_includes=opts.custom_includes)

  if found_errors:
    if opts.custom_sources:
      print stderr
    sys.exit(1)
