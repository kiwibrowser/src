# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Program to run emerge in parallel, for significant speedup.

Usage:
 ./parallel_emerge [--board=BOARD] [--workon=PKGS]
                   [--force-remote-binary=PKGS] [emerge args] package

This script runs multiple emerge processes in parallel, using appropriate
Portage APIs. It is faster than standard emerge because it has a
multiprocess model instead of an asynchronous model.
"""

from __future__ import print_function

import codecs
import copy
import errno
import gc
import heapq
import multiprocessing
import os
try:
  import Queue
except ImportError:
  # Python-3 renamed to "queue".  We still use Queue to avoid collisions
  # with naming variables as "queue".  Maybe we'll transition at some point.
  # pylint: disable=F0401
  import queue as Queue
import signal
import subprocess
import sys
import tempfile
import threading
import time
import traceback

from chromite.lib import cros_build_lib
from chromite.lib import cros_event
from chromite.lib import osutils
from chromite.lib import portage_util
from chromite.lib import process_util
from chromite.lib import proctitle

# If PORTAGE_USERNAME isn't specified, scrape it from the $HOME variable. On
# Chromium OS, the default "portage" user doesn't have the necessary
# permissions. It'd be easier if we could default to $USERNAME, but $USERNAME
# is "root" here because we get called through sudo.
#
# We need to set this before importing any portage modules, because portage
# looks up "PORTAGE_USERNAME" at import time.
#
# NOTE: .bashrc sets PORTAGE_USERNAME = $USERNAME, so most people won't
# encounter this case unless they have an old chroot or blow away the
# environment by running sudo without the -E specifier.
if "PORTAGE_USERNAME" not in os.environ:
  homedir = os.environ.get("HOME")
  if homedir:
    os.environ["PORTAGE_USERNAME"] = os.path.basename(homedir)

# Wrap Popen with a lock to ensure no two Popen are executed simultaneously in
# the same process.
# Two Popen call at the same time might be the cause for crbug.com/433482.
_popen_lock = threading.Lock()
_old_popen = subprocess.Popen

def _LockedPopen(*args, **kwargs):
  with _popen_lock:
    return _old_popen(*args, **kwargs)

subprocess.Popen = _LockedPopen

# Portage doesn't expose dependency trees in its public API, so we have to
# make use of some private APIs here. These modules are found under
# /usr/lib/portage/pym/.
#
# TODO(davidjames): Update Portage to expose public APIs for these features.
# pylint: disable=F0401
from _emerge.actions import adjust_configs
from _emerge.actions import load_emerge_config
from _emerge.create_depgraph_params import create_depgraph_params
from _emerge.depgraph import backtrack_depgraph
from _emerge.main import emerge_main
from _emerge.main import parse_opts
from _emerge.Package import Package
from _emerge.post_emerge import clean_logs
from _emerge.Scheduler import Scheduler
from _emerge.stdout_spinner import stdout_spinner
from portage._global_updates import _global_updates
import portage
import portage.debug
# pylint: enable=F0401


def Usage():
  """Print usage."""
  print("Usage:")
  print(" ./parallel_emerge [--board=BOARD] [--workon=PKGS] [--rebuild]")
  print("                   [--eventlogfile=FILE] [emerge args] package")
  print()
  print("Packages specified as workon packages are always built from source.")
  print()
  print("The --workon argument is mainly useful when you want to build and")
  print("install packages that you are working on unconditionally, but do not")
  print("to have to rev the package to indicate you want to build it from")
  print("source. The build_packages script will automatically supply the")
  print("workon argument to emerge, ensuring that packages selected using")
  print("cros-workon are rebuilt.")
  print()
  print("The --rebuild option rebuilds packages whenever their dependencies")
  print("are changed. This ensures that your build is correct.")
  print()
  print("The --eventlogfile writes events to the given file. File is")
  print("is overwritten if it exists.")


# Global start time
GLOBAL_START = time.time()

# Whether process has been killed by a signal.
KILLED = multiprocessing.Event()


class EmergeData(object):
  """This simple struct holds various emerge variables.

  This struct helps us easily pass emerge variables around as a unit.
  These variables are used for calculating dependencies and installing
  packages.
  """

  __slots__ = ["action", "cmdline_packages", "depgraph", "favorites",
               "mtimedb", "opts", "root_config", "scheduler_graph",
               "settings", "spinner", "trees"]

  def __init__(self):
    # The action the user requested. If the user is installing packages, this
    # is None. If the user is doing anything other than installing packages,
    # this will contain the action name, which will map exactly to the
    # long-form name of the associated emerge option.
    #
    # Example: If you call parallel_emerge --unmerge package, the action name
    #          will be "unmerge"
    self.action = None

    # The list of packages the user passed on the command-line.
    self.cmdline_packages = None

    # The emerge dependency graph. It'll contain all the packages involved in
    # this merge, along with their versions.
    self.depgraph = None

    # The list of candidates to add to the world file.
    self.favorites = None

    # A dict of the options passed to emerge. This dict has been cleaned up
    # a bit by parse_opts, so that it's a bit easier for the emerge code to
    # look at the options.
    #
    # Emerge takes a few shortcuts in its cleanup process to make parsing of
    # the options dict easier. For example, if you pass in "--usepkg=n", the
    # "--usepkg" flag is just left out of the dictionary altogether. Because
    # --usepkg=n is the default, this makes parsing easier, because emerge
    # can just assume that if "--usepkg" is in the dictionary, it's enabled.
    #
    # These cleanup processes aren't applied to all options. For example, the
    # --with-bdeps flag is passed in as-is.  For a full list of the cleanups
    # applied by emerge, see the parse_opts function in the _emerge.main
    # package.
    self.opts = None

    # A dictionary used by portage to maintain global state. This state is
    # loaded from disk when portage starts up, and saved to disk whenever we
    # call mtimedb.commit().
    #
    # This database contains information about global updates (i.e., what
    # version of portage we have) and what we're currently doing. Portage
    # saves what it is currently doing in this database so that it can be
    # resumed when you call it with the --resume option.
    #
    # parallel_emerge does not save what it is currently doing in the mtimedb,
    # so we do not support the --resume option.
    self.mtimedb = None

    # The portage configuration for our current root. This contains the portage
    # settings (see below) and the three portage trees for our current root.
    # (The three portage trees are explained below, in the documentation for
    #  the "trees" member.)
    self.root_config = None

    # The scheduler graph is used by emerge to calculate what packages to
    # install. We don't actually install any deps, so this isn't really used,
    # but we pass it in to the Scheduler object anyway.
    self.scheduler_graph = None

    # Portage settings for our current session. Most of these settings are set
    # in make.conf inside our current install root.
    self.settings = None

    # The spinner, which spews stuff to stdout to indicate that portage is
    # doing something. We maintain our own spinner, so we set the portage
    # spinner to "silent" mode.
    self.spinner = None

    # The portage trees. There are separate portage trees for each root. To get
    # the portage tree for the current root, you can look in self.trees[root],
    # where root = self.settings["ROOT"].
    #
    # In each root, there are three trees: vartree, porttree, and bintree.
    #  - vartree: A database of the currently-installed packages.
    #  - porttree: A database of ebuilds, that can be used to build packages.
    #  - bintree: A database of binary packages.
    self.trees = None


class DepGraphGenerator(object):
  """Grab dependency information about packages from portage.

  Typical usage:
    deps = DepGraphGenerator()
    deps.Initialize(sys.argv[1:])
    deps_tree, deps_info = deps.GenDependencyTree()
    deps_graph = deps.GenDependencyGraph(deps_tree, deps_info)
    deps.PrintTree(deps_tree)
    PrintDepsMap(deps_graph)
  """

  __slots__ = ["board", "emerge", "package_db", "show_output", "sysroot",
               "unpack_only", "max_retries", "install_plan_filename"]

  def __init__(self):
    self.board = None
    self.emerge = EmergeData()
    self.package_db = {}
    self.show_output = False
    self.sysroot = None
    self.unpack_only = False
    self.max_retries = 1
    self.install_plan_filename = None

  def ParseParallelEmergeArgs(self, argv):
    """Read the parallel emerge arguments from the command-line.

    We need to be compatible with emerge arg format.  We scrape arguments that
    are specific to parallel_emerge, and pass through the rest directly to
    emerge.

    Args:
      argv: arguments list

    Returns:
      Arguments that don't belong to parallel_emerge
    """
    emerge_args = []
    for arg in argv:
      # Specifically match arguments that are specific to parallel_emerge, and
      # pass through the rest.
      if arg.startswith("--board="):
        self.board = arg.replace("--board=", "")
      elif arg.startswith("--sysroot="):
        self.sysroot = arg.replace("--sysroot=", "")
      elif arg.startswith("--workon="):
        workon_str = arg.replace("--workon=", "")
        emerge_args.append("--reinstall-atoms=%s" % workon_str)
        emerge_args.append("--usepkg-exclude=%s" % workon_str)
      elif arg.startswith("--force-remote-binary="):
        force_remote_binary = arg.replace("--force-remote-binary=", "")
        emerge_args.append("--useoldpkg-atoms=%s" % force_remote_binary)
      elif arg.startswith("--retries="):
        self.max_retries = int(arg.replace("--retries=", ""))
      elif arg == "--show-output":
        self.show_output = True
      elif arg == "--rebuild":
        emerge_args.append("--rebuild-if-unbuilt")
      elif arg == "--unpackonly":
        emerge_args.append("--fetchonly")
        self.unpack_only = True
      elif arg.startswith("--eventlogfile="):
        log_file_name = arg.replace("--eventlogfile=", "")
        event_logger = cros_event.getEventFileLogger(log_file_name)
        event_logger.setKind('ParallelEmerge')
        cros_event.setEventLogger(event_logger)
      elif arg.startswith("--install-plan-filename"):
        # No emerge equivalent, used to calculate the list of packages
        # that changed and we will need to calculate reverse dependencies.
        self.install_plan_filename = arg.replace("--install-plan-filename=", "")
      else:
        # Not one of our options, so pass through to emerge.
        emerge_args.append(arg)

    # These packages take a really long time to build, so, for expediency, we
    # are blacklisting them from automatic rebuilds because one of their
    # dependencies needs to be recompiled.
    for pkg in ("chromeos-base/chromeos-chrome",):
      emerge_args.append("--rebuild-exclude=%s" % pkg)

    return emerge_args

  def Initialize(self, args):
    """Initializer. Parses arguments and sets up portage state."""

    # Parse and strip out args that are just intended for parallel_emerge.
    emerge_args = self.ParseParallelEmergeArgs(args)

    if self.sysroot and self.board:
      cros_build_lib.Die("--sysroot and --board are incompatible.")

    # Setup various environment variables based on our current board. These
    # variables are normally setup inside emerge-${BOARD}, but since we don't
    # call that script, we have to set it up here. These variables serve to
    # point our tools at /build/BOARD and to setup cross compiles to the
    # appropriate board as configured in toolchain.conf.
    if self.board:
      self.sysroot = os.environ.get('SYSROOT',
                                    cros_build_lib.GetSysroot(self.board))

    if self.sysroot:
      os.environ["PORTAGE_CONFIGROOT"] = self.sysroot
      os.environ["SYSROOT"] = self.sysroot

    # Turn off interactive delays
    os.environ["EBEEP_IGNORE"] = "1"
    os.environ["EPAUSE_IGNORE"] = "1"
    os.environ["CLEAN_DELAY"] = "0"

    # Parse the emerge options.
    action, opts, cmdline_packages = parse_opts(emerge_args, silent=True)

    # Set environment variables based on options. Portage normally sets these
    # environment variables in emerge_main, but we can't use that function,
    # because it also does a bunch of other stuff that we don't want.
    # TODO(davidjames): Patch portage to move this logic into a function we can
    # reuse here.
    if "--debug" in opts:
      os.environ["PORTAGE_DEBUG"] = "1"
    if "--config-root" in opts:
      os.environ["PORTAGE_CONFIGROOT"] = opts["--config-root"]
    if "--root" in opts:
      os.environ["ROOT"] = opts["--root"]
    if "--accept-properties" in opts:
      os.environ["ACCEPT_PROPERTIES"] = opts["--accept-properties"]

    # If we're installing packages to the board, we can disable vardb locks.
    # This is safe because we only run up to one instance of parallel_emerge in
    # parallel.
    # TODO(davidjames): Enable this for the host too.
    if self.sysroot:
      os.environ.setdefault("PORTAGE_LOCKS", "false")

    # Now that we've setup the necessary environment variables, we can load the
    # emerge config from disk.
    # pylint: disable=unpacking-non-sequence
    settings, trees, mtimedb = load_emerge_config()

    # Add in EMERGE_DEFAULT_OPTS, if specified.
    tmpcmdline = []
    if "--ignore-default-opts" not in opts:
      tmpcmdline.extend(settings["EMERGE_DEFAULT_OPTS"].split())
    tmpcmdline.extend(emerge_args)
    action, opts, cmdline_packages = parse_opts(tmpcmdline)

    # If we're installing to the board, we want the --root-deps option so that
    # portage will install the build dependencies to that location as well.
    if self.sysroot:
      opts.setdefault("--root-deps", True)

    # Check whether our portage tree is out of date. Typically, this happens
    # when you're setting up a new portage tree, such as in setup_board and
    # make_chroot. In that case, portage applies a bunch of global updates
    # here. Once the updates are finished, we need to commit any changes
    # that the global update made to our mtimedb, and reload the config.
    #
    # Portage normally handles this logic in emerge_main, but again, we can't
    # use that function here.
    if _global_updates(trees, mtimedb["updates"]):
      mtimedb.commit()
      # pylint: disable=unpacking-non-sequence
      settings, trees, mtimedb = load_emerge_config(trees=trees)

    # Setup implied options. Portage normally handles this logic in
    # emerge_main.
    if "--buildpkgonly" in opts or "buildpkg" in settings.features:
      opts.setdefault("--buildpkg", True)
    if "--getbinpkgonly" in opts:
      opts.setdefault("--usepkgonly", True)
      opts.setdefault("--getbinpkg", True)
    if "getbinpkg" in settings.features:
      # Per emerge_main, FEATURES=getbinpkg overrides --getbinpkg=n
      opts["--getbinpkg"] = True
    if "--getbinpkg" in opts or "--usepkgonly" in opts:
      opts.setdefault("--usepkg", True)
    if "--fetch-all-uri" in opts:
      opts.setdefault("--fetchonly", True)
    if "--skipfirst" in opts:
      opts.setdefault("--resume", True)
    if "--buildpkgonly" in opts:
      # --buildpkgonly will not merge anything, so it overrides all binary
      # package options.
      for opt in ("--getbinpkg", "--getbinpkgonly",
                  "--usepkg", "--usepkgonly"):
        opts.pop(opt, None)
    if (settings.get("PORTAGE_DEBUG", "") == "1" and
        "python-trace" in settings.features):
      portage.debug.set_trace(True)

    # Complain about unsupported options
    for opt in ("--ask", "--ask-enter-invalid", "--resume", "--skipfirst"):
      if opt in opts:
        print("%s is not supported by parallel_emerge" % opt)
        sys.exit(1)

    # Make emerge specific adjustments to the config (e.g. colors!)
    adjust_configs(opts, trees)

    # Save our configuration so far in the emerge object
    emerge = self.emerge
    emerge.action, emerge.opts = action, opts
    emerge.settings, emerge.trees, emerge.mtimedb = settings, trees, mtimedb
    emerge.cmdline_packages = cmdline_packages
    root = settings["ROOT"]
    emerge.root_config = trees[root]["root_config"]

    if "--usepkg" in opts:
      emerge.trees[root]["bintree"].populate("--getbinpkg" in opts)

  def CreateDepgraph(self, emerge, packages):
    """Create an emerge depgraph object."""
    # Setup emerge options.
    emerge_opts = emerge.opts.copy()

    # Ask portage to build a dependency graph. with the options we specified
    # above.
    params = create_depgraph_params(emerge_opts, emerge.action)
    success, depgraph, favorites = backtrack_depgraph(
        emerge.settings, emerge.trees, emerge_opts, params, emerge.action,
        packages, emerge.spinner)
    emerge.depgraph = depgraph

    # Is it impossible to honor the user's request? Bail!
    if not success:
      depgraph.display_problems()
      sys.exit(1)

    emerge.depgraph = depgraph
    emerge.favorites = favorites

    # Prime and flush emerge caches.
    root = emerge.settings["ROOT"]
    vardb = emerge.trees[root]["vartree"].dbapi
    if "--pretend" not in emerge.opts:
      vardb.counter_tick()
    vardb.flush_cache()

  def GenDependencyTree(self):
    """Get dependency tree info from emerge.

    Returns:
      Dependency tree
    """
    start = time.time()

    emerge = self.emerge

    # Create a list of packages to merge
    packages = set(emerge.cmdline_packages[:])

    # Tell emerge to be quiet. We print plenty of info ourselves so we don't
    # need any extra output from portage.
    portage.util.noiselimit = -1

    # My favorite feature: The silent spinner. It doesn't spin. Ever.
    # I'd disable the colors by default too, but they look kind of cool.
    emerge.spinner = stdout_spinner()
    emerge.spinner.update = emerge.spinner.update_quiet

    if "--quiet" not in emerge.opts:
      print("Calculating deps...")

    with cros_event.newEvent(task_name="GenerateDepTree"):
      self.CreateDepgraph(emerge, packages)
      depgraph = emerge.depgraph

    # Build our own tree from the emerge digraph.
    deps_tree = {}
    # pylint: disable=W0212
    digraph = depgraph._dynamic_config.digraph
    root = emerge.settings["ROOT"]
    final_db = depgraph._dynamic_config._filtered_trees[root]['graph_db']
    for node, node_deps in digraph.nodes.items():
      # Calculate dependency packages that need to be installed first. Each
      # child on the digraph is a dependency. The "operation" field specifies
      # what we're doing (e.g. merge, uninstall, etc.). The "priorities" array
      # contains the type of dependency (e.g. build, runtime, runtime_post,
      # etc.)
      #
      # Portage refers to the identifiers for packages as a CPV. This acronym
      # stands for Component/Path/Version.
      #
      # Here's an example CPV: chromeos-base/power_manager-0.0.1-r1
      # Split up, this CPV would be:
      #   C -- Component: chromeos-base
      #   P -- Path:      power_manager
      #   V -- Version:   0.0.1-r1
      #
      # We just refer to CPVs as packages here because it's easier.
      deps = {}
      for child, priorities in node_deps[0].items():
        if isinstance(child, Package) and child.root == root:
          cpv = str(child.cpv)
          action = str(child.operation)

          # If we're uninstalling a package, check whether Portage is
          # installing a replacement. If so, just depend on the installation
          # of the new package, because the old package will automatically
          # be uninstalled at that time.
          if action == "uninstall":
            for pkg in final_db.match_pkgs(child.slot_atom):
              cpv = str(pkg.cpv)
              action = "merge"
              break

          deps[cpv] = dict(action=action,
                           deptypes=[str(x) for x in priorities],
                           deps={})

      # We've built our list of deps, so we can add our package to the tree.
      if isinstance(node, Package) and node.root == root:
        deps_tree[str(node.cpv)] = dict(action=str(node.operation),
                                        deps=deps)

    # Ask portage for its install plan, so that we can only throw out
    # dependencies that portage throws out.
    deps_info = {}
    for pkg in depgraph.altlist():
      if isinstance(pkg, Package):
        assert pkg.root == root
        self.package_db[pkg.cpv] = pkg

        # Save off info about the package
        deps_info[str(pkg.cpv)] = {"idx": len(deps_info)}

    seconds = time.time() - start
    if "--quiet" not in emerge.opts:
      print("Deps calculated in %dm%.1fs" % (seconds / 60, seconds % 60))

    # Calculate the install plan packages and append to temp file. They will be
    # used to calculate all the reverse dependencies on these change packages.
    if self.install_plan_filename:
      # Always write the file even if nothing to do, scripts expect existence.
      output = '\n'.join(deps_info)
      if len(output) > 0:
        # add a trailing newline only if the output is not empty.
        output += '\n'
      osutils.WriteFile(self.install_plan_filename,
                        output,
                        mode='a')
    return deps_tree, deps_info

  def PrintTree(self, deps, depth=""):
    """Print the deps we have seen in the emerge output.

    Args:
      deps: Dependency tree structure.
      depth: Allows printing the tree recursively, with indentation.
    """
    for entry in sorted(deps):
      action = deps[entry]["action"]
      print("%s %s (%s)" % (depth, entry, action))
      self.PrintTree(deps[entry]["deps"], depth=depth + "  ")

  def GenDependencyGraph(self, deps_tree, deps_info):
    """Generate a doubly linked dependency graph.

    Args:
      deps_tree: Dependency tree structure.
      deps_info: More details on the dependencies.

    Returns:
      Deps graph in the form of a dict of packages, with each package
      specifying a "needs" list and "provides" list.
    """
    emerge = self.emerge

    # deps_map is the actual dependency graph.
    #
    # Each package specifies a "needs" list and a "provides" list. The "needs"
    # list indicates which packages we depend on. The "provides" list
    # indicates the reverse dependencies -- what packages need us.
    #
    # We also provide some other information in the dependency graph:
    #  - action: What we're planning on doing with this package. Generally,
    #            "merge", "nomerge", or "uninstall"
    deps_map = {}

    def ReverseTree(packages):
      """Convert tree to digraph.

      Take the tree of package -> requirements and reverse it to a digraph of
      buildable packages -> packages they unblock.

      Args:
        packages: Tree(s) of dependencies.

      Returns:
        Unsanitized digraph.
      """
      binpkg_phases = set(["setup", "preinst", "postinst"])
      needed_dep_types = set(["blocker", "buildtime", "buildtime_slot_op",
                              "runtime", "runtime_slot_op"])
      ignored_dep_types = set(["ignored", "runtime_post", "soft"])

      # There's a bug in the Portage library where it always returns 'optional'
      # and never 'buildtime' for the digraph while --usepkg is enabled; even
      # when the package is being rebuilt. To work around this, we treat
      # 'optional' as needed when we are using --usepkg. See crbug.com/756240 .
      if "--usepkg" in self.emerge.opts:
        needed_dep_types.add("optional")
      else:
        ignored_dep_types.add("optional")

      all_dep_types = ignored_dep_types | needed_dep_types
      for pkg in packages:

        # Create an entry for the package
        action = packages[pkg]["action"]
        default_pkg = {"needs": {}, "provides": set(), "action": action,
                       "nodeps": False, "binary": False}
        this_pkg = deps_map.setdefault(pkg, default_pkg)

        if pkg in deps_info:
          this_pkg["idx"] = deps_info[pkg]["idx"]

        # If a package doesn't have any defined phases that might use the
        # dependent packages (i.e. pkg_setup, pkg_preinst, or pkg_postinst),
        # we can install this package before its deps are ready.
        emerge_pkg = self.package_db.get(pkg)
        if emerge_pkg and emerge_pkg.type_name == "binary":
          this_pkg["binary"] = True
          defined_phases = emerge_pkg.defined_phases
          defined_binpkg_phases = binpkg_phases.intersection(defined_phases)
          if not defined_binpkg_phases:
            this_pkg["nodeps"] = True

        # Create entries for dependencies of this package first.
        ReverseTree(packages[pkg]["deps"])

        # Add dependencies to this package.
        for dep, dep_item in packages[pkg]["deps"].iteritems():
          # We only need to enforce strict ordering of dependencies if the
          # dependency is a blocker, or is a buildtime or runtime dependency.
          # (I.e., ignored, optional, and runtime_post dependencies don't
          # depend on ordering.)
          dep_types = dep_item["deptypes"]
          if needed_dep_types.intersection(dep_types):
            deps_map[dep]["provides"].add(pkg)
            this_pkg["needs"][dep] = "/".join(dep_types)

          # Verify we processed all appropriate dependency types.
          unknown_dep_types = set(dep_types) - all_dep_types
          if unknown_dep_types:
            print("Unknown dependency types found:")
            print("  %s -> %s (%s)" % (pkg, dep, "/".join(unknown_dep_types)))
            sys.exit(1)

          # If there's a blocker, Portage may need to move files from one
          # package to another, which requires editing the CONTENTS files of
          # both packages. To avoid race conditions while editing this file,
          # the two packages must not be installed in parallel, so we can't
          # safely ignore dependencies. See http://crosbug.com/19328
          if "blocker" in dep_types:
            this_pkg["nodeps"] = False

    def FindCycles():
      """Find cycles in the dependency tree.

      Returns:
        A dict mapping cyclic packages to a dict of the deps that cause
        cycles. For each dep that causes cycles, it returns an example
        traversal of the graph that shows the cycle.
      """

      def FindCyclesAtNode(pkg, cycles, unresolved, resolved):
        """Find cycles in cyclic dependencies starting at specified package.

        Args:
          pkg: Package identifier.
          cycles: A dict mapping cyclic packages to a dict of the deps that
                  cause cycles. For each dep that causes cycles, it returns an
                  example traversal of the graph that shows the cycle.
          unresolved: Nodes that have been visited but are not fully processed.
          resolved: Nodes that have been visited and are fully processed.
        """
        pkg_cycles = cycles.get(pkg)
        if pkg in resolved and not pkg_cycles:
          # If we already looked at this package, and found no cyclic
          # dependencies, we can stop now.
          return
        unresolved.append(pkg)
        for dep in deps_map[pkg]["needs"]:
          if dep in unresolved:
            idx = unresolved.index(dep)
            mycycle = unresolved[idx:] + [dep]
            for i in xrange(len(mycycle) - 1):
              pkg1, pkg2 = mycycle[i], mycycle[i+1]
              cycles.setdefault(pkg1, {}).setdefault(pkg2, mycycle)
          elif not pkg_cycles or dep not in pkg_cycles:
            # Looks like we haven't seen this edge before.
            FindCyclesAtNode(dep, cycles, unresolved, resolved)
        unresolved.pop()
        resolved.add(pkg)

      cycles, unresolved, resolved = {}, [], set()
      for pkg in deps_map:
        FindCyclesAtNode(pkg, cycles, unresolved, resolved)
      return cycles

    def RemoveUnusedPackages():
      """Remove installed packages, propagating dependencies."""
      # Schedule packages that aren't on the install list for removal
      rm_pkgs = set(deps_map.keys()) - set(deps_info.keys())

      # Remove the packages we don't want, simplifying the graph and making
      # it easier for us to crack cycles.
      for pkg in sorted(rm_pkgs):
        this_pkg = deps_map[pkg]
        needs = this_pkg["needs"]
        provides = this_pkg["provides"]
        for dep in needs:
          dep_provides = deps_map[dep]["provides"]
          dep_provides.update(provides)
          dep_provides.discard(pkg)
          dep_provides.discard(dep)
        for target in provides:
          target_needs = deps_map[target]["needs"]
          target_needs.update(needs)
          target_needs.pop(pkg, None)
          target_needs.pop(target, None)
        del deps_map[pkg]

    def PrintCycleBreak(basedep, dep, mycycle):
      """Print details about a cycle that we are planning on breaking.

      We are breaking a cycle where dep needs basedep. mycycle is an
      example cycle which contains dep -> basedep.
      """

      needs = deps_map[dep]["needs"]
      depinfo = needs.get(basedep, "deleted")

      # It's OK to swap install order for blockers, as long as the two
      # packages aren't installed in parallel. If there is a cycle, then
      # we know the packages depend on each other already, so we can drop the
      # blocker safely without printing a warning.
      if depinfo == "blocker":
        return

      # Notify the user that we're breaking a cycle.
      print("Breaking %s -> %s (%s)" % (dep, basedep, depinfo))

      # Show cycle.
      for i in xrange(len(mycycle) - 1):
        pkg1, pkg2 = mycycle[i], mycycle[i+1]
        needs = deps_map[pkg1]["needs"]
        depinfo = needs.get(pkg2, "deleted")
        if pkg1 == dep and pkg2 == basedep:
          depinfo = depinfo + ", deleting"
        print("  %s -> %s (%s)" % (pkg1, pkg2, depinfo))

    def SanitizeTree():
      """Remove circular dependencies.

      We prune all dependencies involved in cycles that go against the emerge
      ordering. This has a nice property: we're guaranteed to merge
      dependencies in the same order that portage does.

      Because we don't treat any dependencies as "soft" unless they're killed
      by a cycle, we pay attention to a larger number of dependencies when
      merging. This hurts performance a bit, but helps reliability.
      """
      start = time.time()
      cycles = FindCycles()
      while cycles:
        for dep, mycycles in cycles.iteritems():
          for basedep, mycycle in mycycles.iteritems():
            if deps_info[basedep]["idx"] >= deps_info[dep]["idx"]:
              if "--quiet" not in emerge.opts:
                PrintCycleBreak(basedep, dep, mycycle)
              del deps_map[dep]["needs"][basedep]
              deps_map[basedep]["provides"].remove(dep)
        cycles = FindCycles()
      seconds = time.time() - start
      if "--quiet" not in emerge.opts and seconds >= 0.1:
        print("Tree sanitized in %dm%.1fs" % (seconds / 60, seconds % 60))

    def FindRecursiveProvides(pkg, seen):
      """Find all nodes that require a particular package.

      Assumes that graph is acyclic.

      Args:
        pkg: Package identifier.
        seen: Nodes that have been visited so far.
      """
      if pkg in seen:
        return
      seen.add(pkg)
      info = deps_map[pkg]
      info["tprovides"] = info["provides"].copy()
      for dep in info["provides"]:
        FindRecursiveProvides(dep, seen)
        info["tprovides"].update(deps_map[dep]["tprovides"])

    ReverseTree(deps_tree)

    # We need to remove unused packages so that we can use the dependency
    # ordering of the install process to show us what cycles to crack.
    RemoveUnusedPackages()
    SanitizeTree()
    seen = set()
    for pkg in deps_map:
      FindRecursiveProvides(pkg, seen)
    return deps_map

  def PrintInstallPlan(self, deps_map):
    """Print an emerge-style install plan.

    The install plan lists what packages we're installing, in order.
    It's useful for understanding what parallel_emerge is doing.

    Args:
      deps_map: The dependency graph.
    """

    def InstallPlanAtNode(target, deps_map):
      nodes = []
      nodes.append(target)
      for dep in deps_map[target]["provides"]:
        del deps_map[dep]["needs"][target]
        if not deps_map[dep]["needs"]:
          nodes.extend(InstallPlanAtNode(dep, deps_map))
      return nodes

    deps_map = copy.deepcopy(deps_map)
    install_plan = []
    plan = set()
    for target, info in deps_map.iteritems():
      if not info["needs"] and target not in plan:
        for item in InstallPlanAtNode(target, deps_map):
          plan.add(item)
          install_plan.append(self.package_db[item])

    for pkg in plan:
      del deps_map[pkg]

    if deps_map:
      print("Cyclic dependencies:", " ".join(deps_map))
      PrintDepsMap(deps_map)
      sys.exit(1)

    self.emerge.depgraph.display(install_plan)


def PrintDepsMap(deps_map):
  """Print dependency graph, for each package list it's prerequisites."""
  for i in sorted(deps_map):
    print("%s: (%s) needs" % (i, deps_map[i]["action"]))
    needs = deps_map[i]["needs"]
    for j in sorted(needs):
      print("    %s" % (j))
    if not needs:
      print("    no dependencies")


class EmergeJobState(object):
  """Structure describing the EmergeJobState."""

  __slots__ = ["done", "filename", "last_notify_timestamp", "last_output_seek",
               "last_output_timestamp", "pkgname", "retcode", "start_timestamp",
               "target", "try_count", "fetch_only", "unpack_only"]

  def __init__(self, target, pkgname, done, filename, start_timestamp,
               retcode=None, fetch_only=False, try_count=0, unpack_only=False):

    # The full name of the target we're building (e.g.
    # virtual/target-os-1-r60)
    self.target = target

    # The short name of the target we're building (e.g. target-os-1-r60)
    self.pkgname = pkgname

    # Whether the job is done. (True if the job is done; false otherwise.)
    self.done = done

    # The filename where output is currently stored.
    self.filename = filename

    # The timestamp of the last time we printed the name of the log file. We
    # print this at the beginning of the job, so this starts at
    # start_timestamp.
    self.last_notify_timestamp = start_timestamp

    # The location (in bytes) of the end of the last complete line we printed.
    # This starts off at zero. We use this to jump to the right place when we
    # print output from the same ebuild multiple times.
    self.last_output_seek = 0

    # The timestamp of the last time we printed output. Since we haven't
    # printed output yet, this starts at zero.
    self.last_output_timestamp = 0

    # The return code of our job, if the job is actually finished.
    self.retcode = retcode

    # Number of tries for this job
    self.try_count = try_count

    # Was this just a fetch job?
    self.fetch_only = fetch_only

    # The timestamp when our job started.
    self.start_timestamp = start_timestamp

    # No emerge, only unpack packages.
    self.unpack_only = unpack_only


def KillHandler(_signum, _frame):
  # Kill self and all subprocesses.
  os.killpg(0, signal.SIGKILL)


def SetupWorkerSignals():
  def ExitHandler(_signum, _frame):
    # Set KILLED flag.
    KILLED.set()

    # Remove our signal handlers so we don't get called recursively.
    signal.signal(signal.SIGINT, KillHandler)
    signal.signal(signal.SIGTERM, KillHandler)

  # Ensure that we exit quietly and cleanly, if possible, when we receive
  # SIGTERM or SIGINT signals. By default, when the user hits CTRL-C, all
  # of the child processes will print details about KeyboardInterrupt
  # exceptions, which isn't very helpful.
  signal.signal(signal.SIGINT, ExitHandler)
  signal.signal(signal.SIGTERM, ExitHandler)


def EmergeProcess(output, job_state, *args, **kwargs):
  """Merge a package in a subprocess.

  Args:
    output: Temporary file to write output.
    job_state: Stored state of package
    *args: Arguments to pass to Scheduler constructor.
    **kwargs: Keyword arguments to pass to Scheduler constructor.

  Returns:
    The exit code returned by the subprocess.
  """

  target = job_state.target

  job_state.try_count += 1

  cpv = portage_util.SplitCPV(target)

  event = cros_event.newEvent(task_name="EmergePackage",
                              name=cpv.package,
                              category=cpv.category,
                              version=cpv.version,
                              try_count=job_state.try_count)
  pid = os.fork()
  if pid == 0:
    try:
      proctitle.settitle('EmergeProcess', target)

      # Sanity checks.
      if sys.stdout.fileno() != 1:
        raise Exception("sys.stdout.fileno() != 1")
      if sys.stderr.fileno() != 2:
        raise Exception("sys.stderr.fileno() != 2")

      # - Redirect 1 (stdout) and 2 (stderr) at our temporary file.
      # - Redirect 0 to point at sys.stdin. In this case, sys.stdin
      #   points at a file reading os.devnull, because multiprocessing mucks
      #   with sys.stdin.
      # - Leave the sys.stdin and output filehandles alone.
      fd_pipes = {0: sys.stdin.fileno(),
                  1: output.fileno(),
                  2: output.fileno(),
                  sys.stdin.fileno(): sys.stdin.fileno(),
                  output.fileno(): output.fileno()}
      # pylint: disable=W0212
      portage.process._setup_pipes(fd_pipes, close_fds=False)

      # Portage doesn't like when sys.stdin.fileno() != 0, so point sys.stdin
      # at the filehandle we just created in _setup_pipes.
      if sys.stdin.fileno() != 0:
        sys.__stdin__ = sys.stdin = os.fdopen(0, "r")

      scheduler = Scheduler(*args, **kwargs)

      # Enable blocker handling even though we're in --nodeps mode. This
      # allows us to unmerge the blocker after we've merged the replacement.
      scheduler._opts_ignore_blockers = frozenset()

      # Actually do the merge.
      with event:
        job_state.retcode = scheduler.merge()
        if job_state.retcode != 0:
          event.fail(message="non-zero value returned")

    # We catch all exceptions here (including SystemExit, KeyboardInterrupt,
    # etc) so as to ensure that we don't confuse the multiprocessing module,
    # which expects that all forked children exit with os._exit().
    # pylint: disable=W0702
    except:
      traceback.print_exc(file=output)
      job_state.retcode = 1
    sys.stdout.flush()
    sys.stderr.flush()
    output.flush()
    # pylint: disable=W0212
    os._exit(job_state.retcode)
  else:
    # Return the exit code of the subprocess.
    return os.waitpid(pid, 0)[1]


def UnpackPackage(pkg_state):
  """Unpacks package described by pkg_state.

  Args:
    pkg_state: EmergeJobState object describing target.

  Returns:
    Exit code returned by subprocess.
  """
  pkgdir = os.environ.get("PKGDIR",
                          os.path.join(os.environ["SYSROOT"], "packages"))
  root = os.environ.get("ROOT", os.environ["SYSROOT"])
  path = os.path.join(pkgdir, pkg_state.target + ".tbz2")
  comp = cros_build_lib.FindCompressor(cros_build_lib.COMP_BZIP2)
  cmd = [comp, "-dc"]
  if comp.endswith("pbzip2"):
    cmd.append("--ignore-trailing-garbage=1")
  cmd.append(path)

  with cros_event.newEvent(task_name="UnpackPackage", **pkg_state) as event:
    result = cros_build_lib.RunCommand(cmd, cwd=root, stdout_to_pipe=True,
                                       print_cmd=False, error_code_ok=True)

    # If we were not successful, return now and don't attempt untar.
    if result.returncode != 0:
      event.fail("error compressing: returned {}".format(result.returncode))
      return result.returncode

    cmd = ["sudo", "tar", "-xf", "-", "-C", root]

    result = cros_build_lib.RunCommand(cmd, cwd=root, input=result.output,
                                       print_cmd=False, error_code_ok=True)
    if result.returncode != 0:
      event.fail("error extracting:returned {}".format(result.returncode))

    return result.returncode


def EmergeWorker(task_queue, job_queue, emerge, package_db, fetch_only=False,
                 unpack_only=False):
  """This worker emerges any packages given to it on the task_queue.

  Args:
    task_queue: The queue of tasks for this worker to do.
    job_queue: The queue of results from the worker.
    emerge: An EmergeData() object.
    package_db: A dict, mapping package ids to portage Package objects.
    fetch_only: A bool, indicating if we should just fetch the target.
    unpack_only: A bool, indicating if we should just unpack the target.

  It expects package identifiers to be passed to it via task_queue. When
  a task is started, it pushes the (target, filename) to the started_queue.
  The output is stored in filename. When a merge starts or finishes, we push
  EmergeJobState objects to the job_queue.
  """
  if fetch_only:
    mode = 'fetch'
  elif unpack_only:
    mode = 'unpack'
  else:
    mode = 'emerge'
  proctitle.settitle('EmergeWorker', mode, '[idle]')

  SetupWorkerSignals()
  settings, trees, mtimedb = emerge.settings, emerge.trees, emerge.mtimedb

  # Disable flushing of caches to save on I/O.
  root = emerge.settings["ROOT"]
  vardb = emerge.trees[root]["vartree"].dbapi
  vardb._flush_cache_enabled = False  # pylint: disable=protected-access
  bindb = emerge.trees[root]["bintree"].dbapi
  # Might be a set, might be a list, might be None; no clue, just use shallow
  # copy to ensure we can roll it back.
  # pylint: disable=W0212
  original_remotepkgs = copy.copy(bindb.bintree._remotepkgs)

  opts, spinner = emerge.opts, emerge.spinner
  opts["--nodeps"] = True
  if fetch_only:
    opts["--fetchonly"] = True

  while True:
    # Wait for a new item to show up on the queue. This is a blocking wait,
    # so if there's nothing to do, we just sit here.
    pkg_state = task_queue.get()
    if pkg_state is None:
      # If target is None, this means that the main thread wants us to quit.
      # The other workers need to exit too, so we'll push the message back on
      # to the queue so they'll get it too.
      task_queue.put(None)
      return
    if KILLED.is_set():
      return

    target = pkg_state.target
    proctitle.settitle('EmergeWorker', mode, target)

    db_pkg = package_db[target]

    if db_pkg.type_name == "binary":
      if not fetch_only and pkg_state.fetched_successfully:
        # Ensure portage doesn't think our pkg is remote- else it'll force
        # a redownload of it (even if the on-disk file is fine).  In-memory
        # caching basically, implemented dumbly.
        bindb.bintree._remotepkgs = None
    else:
      bindb.bintree_remotepkgs = original_remotepkgs

    db_pkg.root_config = emerge.root_config
    install_list = [db_pkg]
    pkgname = db_pkg.pf
    output = tempfile.NamedTemporaryFile(prefix=pkgname + "-", delete=False)
    os.chmod(output.name, 644)
    start_timestamp = time.time()
    job = EmergeJobState(target, pkgname, False, output.name, start_timestamp,
                         fetch_only=fetch_only, unpack_only=unpack_only)
    job_queue.put(job)
    if "--pretend" in opts:
      job.retcode = 0
    else:
      try:
        emerge.scheduler_graph.mergelist = install_list
        if unpack_only:
          job.retcode = UnpackPackage(pkg_state)
        else:
          job.retcode = EmergeProcess(output, job, settings, trees, mtimedb,
                                      opts, spinner,
                                      favorites=emerge.favorites,
                                      graph_config=emerge.scheduler_graph)
      except Exception:
        traceback.print_exc(file=output)
        job.retcode = 1
      output.close()

    if KILLED.is_set():
      return

    job = EmergeJobState(target, pkgname, True, output.name, start_timestamp,
                         job.retcode, fetch_only=fetch_only,
                         try_count=job.try_count, unpack_only=unpack_only)
    job_queue.put(job)

    # Set the title back to idle as the multiprocess pool won't destroy us;
    # when another job comes up, it'll re-use this process.
    proctitle.settitle('EmergeWorker', mode, '[idle]')


class LinePrinter(object):
  """Helper object to print a single line."""

  def __init__(self, line):
    self.line = line

  def Print(self, _seek_locations):
    print(self.line)


class JobPrinter(object):
  """Helper object to print output of a job."""

  def __init__(self, job, unlink=False):
    """Print output of job.

    If unlink is True, unlink the job output file when done.
    """
    self.current_time = time.time()
    self.job = job
    self.unlink = unlink

  def Print(self, seek_locations):

    job = self.job

    # Calculate how long the job has been running.
    seconds = self.current_time - job.start_timestamp

    # Note that we've printed out the job so far.
    job.last_output_timestamp = self.current_time

    # Note that we're starting the job
    info = "job %s (%dm%.1fs)" % (job.pkgname, seconds / 60, seconds % 60)
    last_output_seek = seek_locations.get(job.filename, 0)
    if last_output_seek:
      print("=== Continue output for %s ===" % info)
    else:
      print("=== Start output for %s ===" % info)

    # Print actual output from job
    f = codecs.open(job.filename, encoding='utf-8', errors='replace')
    f.seek(last_output_seek)
    prefix = job.pkgname + ":"
    for line in f:

      # Save off our position in the file
      if line and line[-1] == "\n":
        last_output_seek = f.tell()
        line = line[:-1]

      # Print our line
      print(prefix, line.encode('utf-8', 'replace'))
    f.close()

    # Save our last spot in the file so that we don't print out the same
    # location twice.
    seek_locations[job.filename] = last_output_seek

    # Note end of output section
    if job.done:
      print("=== Complete: %s ===" % info)
    else:
      print("=== Still running: %s ===" % info)

    if self.unlink:
      os.unlink(job.filename)


def PrintWorker(queue):
  """A worker that prints stuff to the screen as requested."""
  proctitle.settitle('PrintWorker')

  def ExitHandler(_signum, _frame):
    # Set KILLED flag.
    KILLED.set()

    # Switch to default signal handlers so that we'll die after two signals.
    signal.signal(signal.SIGINT, KillHandler)
    signal.signal(signal.SIGTERM, KillHandler)

  # Don't exit on the first SIGINT / SIGTERM, because the parent worker will
  # handle it and tell us when we need to exit.
  signal.signal(signal.SIGINT, ExitHandler)
  signal.signal(signal.SIGTERM, ExitHandler)

  # seek_locations is a map indicating the position we are at in each file.
  # It starts off empty, but is set by the various Print jobs as we go along
  # to indicate where we left off in each file.
  seek_locations = {}
  while True:
    try:
      job = queue.get()
      if job:
        job.Print(seek_locations)
        sys.stdout.flush()
      else:
        break
    except IOError as ex:
      if ex.errno == errno.EINTR:
        # Looks like we received a signal. Keep printing.
        continue
      raise


class TargetState(object):
  """Structure describing the TargetState."""

  __slots__ = ("target", "info", "score", "prefetched", "fetched_successfully")

  def __init__(self, target, info):
    self.target, self.info = target, info
    self.fetched_successfully = False
    self.prefetched = False
    self.score = None
    self.update_score()

  def __cmp__(self, other):
    return cmp(self.score, other.score)

  def update_score(self):
    self.score = (
        -len(self.info["tprovides"]),
        len(self.info["needs"]),
        not self.info["binary"],
        -len(self.info["provides"]),
        self.info["idx"],
        self.target,
        )


class ScoredHeap(object):
  """Implementation of a general purpose scored heap."""

  __slots__ = ("heap", "_heap_set")

  def __init__(self, initial=()):
    self.heap = list()
    self._heap_set = set()
    if initial:
      self.multi_put(initial)

  def get(self):
    item = heapq.heappop(self.heap)
    self._heap_set.remove(item.target)
    return item

  def put(self, item):
    if not isinstance(item, TargetState):
      raise ValueError("Item %r isn't a TargetState" % (item,))
    heapq.heappush(self.heap, item)
    self._heap_set.add(item.target)

  def multi_put(self, sequence):
    sequence = list(sequence)
    self.heap.extend(sequence)
    self._heap_set.update(x.target for x in sequence)
    self.sort()

  def sort(self):
    heapq.heapify(self.heap)

  def __contains__(self, target):
    return target in self._heap_set

  def __nonzero__(self):
    return bool(self.heap)

  def __len__(self):
    return len(self.heap)


class EmergeQueue(object):
  """Class to schedule emerge jobs according to a dependency graph."""

  def __init__(self, deps_map, emerge, package_db, show_output, unpack_only,
               max_retries):
    # Store the dependency graph.
    self._deps_map = deps_map
    self._state_map = {}
    # Initialize the running queue to empty
    self._build_jobs = {}
    self._build_ready = ScoredHeap()
    self._fetch_jobs = {}
    self._fetch_ready = ScoredHeap()
    self._unpack_jobs = {}
    self._unpack_ready = ScoredHeap()
    # List of total package installs represented in deps_map.
    install_jobs = [x for x in deps_map if deps_map[x]["action"] == "merge"]
    self._total_jobs = len(install_jobs)
    self._show_output = show_output
    self._unpack_only = unpack_only
    self._max_retries = max_retries

    if "--pretend" in emerge.opts:
      print("Skipping merge because of --pretend mode.")
      sys.exit(0)

    # Set up a session so we can easily terminate all children.
    self._SetupSession()

    # Setup scheduler graph object. This is used by the child processes
    # to help schedule jobs.
    emerge.scheduler_graph = emerge.depgraph.schedulerGraph()

    # Calculate how many jobs we can run in parallel. We don't want to pass
    # the --jobs flag over to emerge itself, because that'll tell emerge to
    # hide its output, and said output is quite useful for debugging hung
    # jobs.
    procs = min(self._total_jobs,
                emerge.opts.pop("--jobs", multiprocessing.cpu_count()))
    self._build_procs = self._unpack_procs = max(1, procs)
    # Fetch is IO bound, we can use more processes.
    self._fetch_procs = max(4, procs)
    self._load_avg = emerge.opts.pop("--load-average", None)
    self._job_queue = multiprocessing.Queue()
    self._print_queue = multiprocessing.Queue()

    self._fetch_queue = multiprocessing.Queue()
    args = (self._fetch_queue, self._job_queue, emerge, package_db, True)
    self._fetch_pool = multiprocessing.Pool(self._fetch_procs, EmergeWorker,
                                            args)

    self._build_queue = multiprocessing.Queue()
    args = (self._build_queue, self._job_queue, emerge, package_db)
    self._build_pool = multiprocessing.Pool(self._build_procs, EmergeWorker,
                                            args)

    if self._unpack_only:
      # Unpack pool only required on unpack_only jobs.
      self._unpack_queue = multiprocessing.Queue()
      args = (self._unpack_queue, self._job_queue, emerge, package_db, False,
              True)
      self._unpack_pool = multiprocessing.Pool(self._unpack_procs, EmergeWorker,
                                               args)

    self._print_worker = multiprocessing.Process(target=PrintWorker,
                                                 args=[self._print_queue])
    self._print_worker.start()

    # Initialize the failed queue to empty.
    self._retry_queue = []
    self._failed_count = dict()

    # Setup an exit handler so that we print nice messages if we are
    # terminated.
    self._SetupExitHandler()

    # Schedule our jobs.
    self._state_map.update(
        (pkg, TargetState(pkg, data)) for pkg, data in deps_map.iteritems())
    self._fetch_ready.multi_put(self._state_map.itervalues())

  def _SetupSession(self):
    """Set up a session so we can easily terminate all children."""
    # When we call os.setsid(), this sets up a session / process group for this
    # process and all children. These session groups are needed so that we can
    # easily kill all children (including processes launched by emerge) before
    # we exit.
    #
    # One unfortunate side effect of os.setsid() is that it blocks CTRL-C from
    # being received. To work around this, we only call os.setsid() in a forked
    # process, so that the parent can still watch for CTRL-C. The parent will
    # just sit around, watching for signals and propagating them to the child,
    # until the child exits.
    #
    # TODO(davidjames): It would be nice if we could replace this with cgroups.
    pid = os.fork()
    if pid == 0:
      os.setsid()
    else:
      proctitle.settitle('SessionManager')

      def PropagateToChildren(signum, _frame):
        # Just propagate the signals down to the child. We'll exit when the
        # child does.
        try:
          os.kill(pid, signum)
        except OSError as ex:
          if ex.errno != errno.ESRCH:
            raise
      signal.signal(signal.SIGINT, PropagateToChildren)
      signal.signal(signal.SIGTERM, PropagateToChildren)

      def StopGroup(_signum, _frame):
        # When we get stopped, stop the children.
        try:
          os.killpg(pid, signal.SIGSTOP)
          os.kill(0, signal.SIGSTOP)
        except OSError as ex:
          if ex.errno != errno.ESRCH:
            raise
      signal.signal(signal.SIGTSTP, StopGroup)

      def ContinueGroup(_signum, _frame):
        # Launch the children again after being stopped.
        try:
          os.killpg(pid, signal.SIGCONT)
        except OSError as ex:
          if ex.errno != errno.ESRCH:
            raise
      signal.signal(signal.SIGCONT, ContinueGroup)

      # Loop until the children exit. We exit with os._exit to be sure we
      # don't run any finalizers (those will be run by the child process.)
      # pylint: disable=W0212
      while True:
        try:
          # Wait for the process to exit. When it does, exit with the return
          # value of the subprocess.
          os._exit(process_util.GetExitStatus(os.waitpid(pid, 0)[1]))
        except OSError as ex:
          if ex.errno == errno.EINTR:
            continue
          traceback.print_exc()
          os._exit(1)
        except BaseException:
          traceback.print_exc()
          os._exit(1)

  def _SetupExitHandler(self):

    def ExitHandler(signum, _frame):
      # Set KILLED flag.
      KILLED.set()

      # Kill our signal handlers so we don't get called recursively
      signal.signal(signal.SIGINT, KillHandler)
      signal.signal(signal.SIGTERM, KillHandler)

      # Print our current job status
      for job in self._build_jobs.itervalues():
        if job:
          self._print_queue.put(JobPrinter(job, unlink=True))

      # Notify the user that we are exiting
      self._Print("Exiting on signal %s" % signum)
      self._print_queue.put(None)
      self._print_worker.join()

      # Kill child threads, then exit.
      os.killpg(0, signal.SIGKILL)
      sys.exit(1)

    # Print out job status when we are killed
    signal.signal(signal.SIGINT, ExitHandler)
    signal.signal(signal.SIGTERM, ExitHandler)

  def _ScheduleUnpack(self, pkg_state):
    self._unpack_jobs[pkg_state.target] = None
    self._unpack_queue.put(pkg_state)

  def _Schedule(self, pkg_state):
    # We maintain a tree of all deps, if this doesn't need
    # to be installed just free up its children and continue.
    # It is possible to reinstall deps of deps, without reinstalling
    # first level deps, like so:
    # virtual/target-os (merge) -> eselect (nomerge) -> python (merge)
    this_pkg = pkg_state.info
    target = pkg_state.target
    if pkg_state.info is not None:
      if this_pkg["action"] == "nomerge":
        self._Finish(target)
      elif target not in self._build_jobs:
        # Kick off the build if it's marked to be built.
        self._build_jobs[target] = None
        self._build_queue.put(pkg_state)
        return True

  def _ScheduleLoop(self, unpack_only=False):
    if unpack_only:
      ready_queue = self._unpack_ready
      jobs_queue = self._unpack_jobs
      procs = self._unpack_procs
    else:
      ready_queue = self._build_ready
      jobs_queue = self._build_jobs
      procs = self._build_procs

    # If the current load exceeds our desired load average, don't schedule
    # more than one job.
    if self._load_avg and os.getloadavg()[0] > self._load_avg:
      needed_jobs = 1
    else:
      needed_jobs = procs

    # Schedule more jobs.
    while ready_queue and len(jobs_queue) < needed_jobs:
      state = ready_queue.get()
      if unpack_only:
        self._ScheduleUnpack(state)
      else:
        if state.target not in self._failed_count:
          self._Schedule(state)

  def _Print(self, line):
    """Print a single line."""
    self._print_queue.put(LinePrinter(line))

  def _Status(self):
    """Print status."""
    current_time = time.time()
    current_time_struct = time.localtime(current_time)
    no_output = True

    # Print interim output every minute if --show-output is used. Otherwise,
    # print notifications about running packages every 2 minutes, and print
    # full output for jobs that have been running for 60 minutes or more.
    if self._show_output:
      interval = 60
      notify_interval = 0
    else:
      interval = 60 * 60
      notify_interval = 60 * 2
    for job in self._build_jobs.itervalues():
      if job:
        last_timestamp = max(job.start_timestamp, job.last_output_timestamp)
        if last_timestamp + interval < current_time:
          self._print_queue.put(JobPrinter(job))
          job.last_output_timestamp = current_time
          no_output = False
        elif (notify_interval and
              job.last_notify_timestamp + notify_interval < current_time):
          job_seconds = current_time - job.start_timestamp
          args = (job.pkgname, job_seconds / 60, job_seconds % 60, job.filename)
          info = "Still building %s (%dm%.1fs). Logs in %s" % args
          job.last_notify_timestamp = current_time
          self._Print(info)
          no_output = False

    # If we haven't printed any messages yet, print a general status message
    # here.
    if no_output:
      seconds = current_time - GLOBAL_START
      fjobs, fready = len(self._fetch_jobs), len(self._fetch_ready)
      ujobs, uready = len(self._unpack_jobs), len(self._unpack_ready)
      bjobs, bready = len(self._build_jobs), len(self._build_ready)
      retries = len(self._retry_queue)
      pending = max(0, len(self._deps_map) - fjobs - bjobs)
      line = "Pending %s/%s, " % (pending, self._total_jobs)
      if fjobs or fready:
        line += "Fetching %s/%s, " % (fjobs, fready + fjobs)
      if ujobs or uready:
        line += "Unpacking %s/%s, " % (ujobs, uready + ujobs)
      if bjobs or bready or retries:
        line += "Building %s/%s, " % (bjobs, bready + bjobs)
        if retries:
          line += "Retrying %s, " % (retries,)
      load = " ".join(str(x) for x in os.getloadavg())
      line += ("[Time %s | Elapsed %dm%.1fs | Load %s]" % (
          time.strftime('%H:%M:%S', current_time_struct), seconds / 60,
          seconds % 60, load))
      self._Print(line)

  def _Finish(self, target):
    """Mark a target as completed and unblock dependencies."""
    this_pkg = self._deps_map[target]
    if this_pkg["needs"] and this_pkg["nodeps"]:
      # We got installed, but our deps have not been installed yet. Dependent
      # packages should only be installed when our needs have been fully met.
      this_pkg["action"] = "nomerge"
    else:
      for dep in this_pkg["provides"]:
        dep_pkg = self._deps_map[dep]
        state = self._state_map[dep]
        del dep_pkg["needs"][target]
        state.update_score()
        if not state.prefetched:
          if dep in self._fetch_ready:
            # If it's not currently being fetched, update the prioritization
            self._fetch_ready.sort()
        elif not dep_pkg["needs"]:
          if dep_pkg["nodeps"] and dep_pkg["action"] == "nomerge":
            self._Finish(dep)
          else:
            self._build_ready.put(self._state_map[dep])
      self._deps_map.pop(target)

  def _Retry(self):
    while self._retry_queue:
      state = self._retry_queue.pop(0)
      if self._Schedule(state):
        self._Print("Retrying emerge of %s." % state.target)
        break

  def _Shutdown(self):
    # Tell emerge workers to exit. They all exit when 'None' is pushed
    # to the queue.

    # Shutdown the workers first; then jobs (which is how they feed things back)
    # then finally the print queue.

    def _stop(queue, pool):
      if pool is None:
        return
      try:
        queue.put(None)
        pool.close()
        pool.join()
      finally:
        pool.terminate()

    _stop(self._fetch_queue, self._fetch_pool)
    self._fetch_queue = self._fetch_pool = None

    _stop(self._build_queue, self._build_pool)
    self._build_queue = self._build_pool = None

    if self._unpack_only:
      _stop(self._unpack_queue, self._unpack_pool)
      self._unpack_queue = self._unpack_pool = None

    if self._job_queue is not None:
      self._job_queue.close()
      self._job_queue = None

    # Now that our workers are finished, we can kill the print queue.
    if self._print_worker is not None:
      try:
        self._print_queue.put(None)
        self._print_queue.close()
        self._print_worker.join()
      finally:
        self._print_worker.terminate()
    self._print_queue = self._print_worker = None

  def Run(self):
    """Run through the scheduled ebuilds.

    Keep running so long as we have uninstalled packages in the
    dependency graph to merge.
    """
    if not self._deps_map:
      return

    # Start the fetchers.
    for _ in xrange(min(self._fetch_procs, len(self._fetch_ready))):
      state = self._fetch_ready.get()
      self._fetch_jobs[state.target] = None
      self._fetch_queue.put(state)

    # Print an update, then get going.
    self._Status()

    while self._deps_map:
      # Check here that we are actually waiting for something.
      if (self._build_queue.empty() and
          self._job_queue.empty() and
          not self._fetch_jobs and
          not self._fetch_ready and
          not self._unpack_jobs and
          not self._unpack_ready and
          not self._build_jobs and
          not self._build_ready and
          self._deps_map):
        # If we have failed on a package, retry it now.
        if self._retry_queue:
          self._Retry()
        else:
          # Tell the user why we're exiting.
          if self._failed_count:
            print('Packages failed:\n\t%s' %
                  '\n\t'.join(self._failed_count.iterkeys()))
            status_file = os.environ.get("PARALLEL_EMERGE_STATUS_FILE")
            if status_file:
              failed_pkgs = set(portage.versions.cpv_getkey(x)
                                for x in self._failed_count.iterkeys())
              with open(status_file, "a") as f:
                f.write("%s\n" % " ".join(failed_pkgs))
          else:
            print("Deadlock! Circular dependencies!")
          sys.exit(1)

      for _ in xrange(12):
        try:
          job = self._job_queue.get(timeout=5)
          break
        except Queue.Empty:
          # Check if any more jobs can be scheduled.
          self._ScheduleLoop()
      else:
        # Print an update every 60 seconds.
        self._Status()
        continue

      target = job.target

      if job.fetch_only:
        if not job.done:
          self._fetch_jobs[job.target] = job
        else:
          state = self._state_map[job.target]
          state.prefetched = True
          state.fetched_successfully = (job.retcode == 0)
          del self._fetch_jobs[job.target]
          self._Print("Fetched %s in %2.2fs"
                      % (target, time.time() - job.start_timestamp))

          if self._show_output or job.retcode != 0:
            self._print_queue.put(JobPrinter(job, unlink=True))
          else:
            os.unlink(job.filename)
          # Failure or not, let build work with it next.
          if not self._deps_map[job.target]["needs"]:
            self._build_ready.put(state)
            self._ScheduleLoop()

          if self._unpack_only and job.retcode == 0:
            self._unpack_ready.put(state)
            self._ScheduleLoop(unpack_only=True)

          if self._fetch_ready:
            state = self._fetch_ready.get()
            self._fetch_queue.put(state)
            self._fetch_jobs[state.target] = None
          else:
            # Minor optimization; shut down fetchers early since we know
            # the queue is empty.
            self._fetch_queue.put(None)
        continue

      if job.unpack_only:
        if not job.done:
          self._unpack_jobs[target] = job
        else:
          del self._unpack_jobs[target]
          self._Print("Unpacked %s in %2.2fs"
                      % (target, time.time() - job.start_timestamp))
          if self._show_output or job.retcode != 0:
            self._print_queue.put(JobPrinter(job, unlink=True))
          else:
            os.unlink(job.filename)
          if self._unpack_ready:
            state = self._unpack_ready.get()
            self._unpack_queue.put(state)
            self._unpack_jobs[state.target] = None
        continue

      if not job.done:
        self._build_jobs[target] = job
        self._Print("Started %s (logged in %s)" % (target, job.filename))
        continue

      # Print output of job
      if self._show_output or job.retcode != 0:
        self._print_queue.put(JobPrinter(job, unlink=True))
      else:
        os.unlink(job.filename)
      del self._build_jobs[target]

      seconds = time.time() - job.start_timestamp
      details = "%s (in %dm%.1fs)" % (target, seconds / 60, seconds % 60)

      # Complain if necessary.
      if job.retcode != 0:
        # Handle job failure.
        failed_count = self._failed_count.get(target, 0)
        if failed_count >= self._max_retries:
          # If this job has failed and can't be retried, give up.
          self._Print("Failed %s. Your build has failed." % details)
        else:
          # Queue up this build to try again after a long while.
          self._retry_queue.append(self._state_map[target])
          self._failed_count[target] = failed_count + 1
          self._Print("Failed %s, retrying later." % details)
      else:
        self._Print("Completed %s" % details)

        # Mark as completed and unblock waiting ebuilds.
        self._Finish(target)

        if target in self._failed_count and self._retry_queue:
          # If we have successfully retried a failed package, and there
          # are more failed packages, try the next one. We will only have
          # one retrying package actively running at a time.
          self._Retry()


      # Schedule pending jobs and print an update.
      self._ScheduleLoop()
      self._Status()

    # If packages were retried, output a warning.
    if self._failed_count:
      self._Print("")
      self._Print("WARNING: The following packages failed once or more,")
      self._Print("but succeeded upon retry. This might indicate incorrect")
      self._Print("dependencies.")
      for pkg in self._failed_count.iterkeys():
        self._Print("  %s" % pkg)
      self._Print("@@@STEP_WARNINGS@@@")
      self._Print("")

    # Tell child threads to exit.
    self._Print("Merge complete")


def main(argv):
  try:
    return real_main(argv)
  finally:
    # Work around multiprocessing sucking and not cleaning up after itself.
    # http://bugs.python.org/issue4106;
    # Step one; ensure GC is ran *prior* to the VM starting shutdown.
    gc.collect()
    # Step two; go looking for those threads and try to manually reap
    # them if we can.
    for x in threading.enumerate():
      # Filter on the name, and ident; if ident is None, the thread
      # wasn't started.
      if x.name == 'QueueFeederThread' and x.ident is not None:
        x.join(1)


def real_main(argv):
  parallel_emerge_args = argv[:]
  deps = DepGraphGenerator()
  deps.Initialize(parallel_emerge_args)
  emerge = deps.emerge

  if emerge.action is not None:
    argv = deps.ParseParallelEmergeArgs(argv)
    return emerge_main(argv)
  elif not emerge.cmdline_packages:
    Usage()
    return 1

  # Unless we're in pretend mode, there's not much point running without
  # root access. We need to be able to install packages.
  #
  # NOTE: Even if you're running --pretend, it's a good idea to run
  #       parallel_emerge with root access so that portage can write to the
  #       dependency cache. This is important for performance.
  if "--pretend" not in emerge.opts and portage.data.secpass < 2:
    print("parallel_emerge: superuser access is required.")
    return 1

  if "--quiet" not in emerge.opts:
    cmdline_packages = " ".join(emerge.cmdline_packages)
    print("Starting fast-emerge.")
    print(" Building package %s on %s" % (cmdline_packages,
                                          deps.sysroot or "root"))

  deps_tree, deps_info = deps.GenDependencyTree()

  # You want me to be verbose? I'll give you two trees! Twice as much value.
  if "--tree" in emerge.opts and "--verbose" in emerge.opts:
    deps.PrintTree(deps_tree)

  deps_graph = deps.GenDependencyGraph(deps_tree, deps_info)

  # OK, time to print out our progress so far.
  deps.PrintInstallPlan(deps_graph)
  if "--tree" in emerge.opts:
    PrintDepsMap(deps_graph)

  # Are we upgrading portage? If so, and there are more packages to merge,
  # schedule a restart of parallel_emerge to merge the rest. This ensures that
  # we pick up all updates to portage settings before merging any more
  # packages.
  portage_upgrade = False
  root = emerge.settings["ROOT"]
  # pylint: disable=W0212
  if root == "/":
    final_db = emerge.depgraph._dynamic_config._filtered_trees[root]['graph_db']
    for db_pkg in final_db.cp_list("sys-apps/portage"):
      portage_pkg = deps_graph.get(db_pkg.cpv)
      if portage_pkg:
        portage_upgrade = True
        if "--quiet" not in emerge.opts:
          print("Upgrading portage first, then restarting...")

  # Upgrade Portage first, then the rest of the packages.
  #
  # In order to grant the child permission to run setsid, we need to run sudo
  # again. We preserve SUDO_USER here in case an ebuild depends on it.
  if portage_upgrade:
    # Calculate what arguments to use when re-invoking.
    args = ["sudo", "-E", "SUDO_USER=%s" % os.environ.get("SUDO_USER", "")]
    args += [os.path.abspath(sys.argv[0])] + parallel_emerge_args
    args += ["--exclude=sys-apps/portage"]

    # First upgrade Portage.
    passthrough_args = ("--quiet", "--pretend", "--verbose")
    emerge_args = [k for k in emerge.opts if k in passthrough_args]
    ret = emerge_main(emerge_args + ["portage"])
    if ret != 0:
      return ret

    # Now upgrade the rest.
    os.execvp(args[0], args)

  # Attempt to solve crbug.com/433482
  # The file descriptor error appears only when getting userpriv_groups
  # (lazily generated). Loading userpriv_groups here will reduce the number of
  # calls from few hundreds to one.
  portage.data._get_global('userpriv_groups')

  # Run the queued emerges.
  scheduler = EmergeQueue(deps_graph, emerge, deps.package_db, deps.show_output,
                          deps.unpack_only, deps.max_retries)
  try:
    scheduler.Run()
  finally:
    # pylint: disable=W0212
    scheduler._Shutdown()
  scheduler = None

  clean_logs(emerge.settings)

  print("Done")
  return 0
