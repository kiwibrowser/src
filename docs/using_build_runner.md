# Instructions on how to use the buildrunner to execute builds.

The buildrunner is a script which extracts buildsteps from builders and runs
them locally on the slave. It is being developed to simplify development on and
reduce the complexity of the Chromium build infrastructure. When provided a
master name (with `master.cfg` inside) and a builder, it will either execute
steps sequentially or output information about them.

`runbuild.py` is the main script, while `runit.py` is a convenience script that
sets up `PYTHONPATH` for you. Note that you can use `runit.py` to conveniently
run other scripts in the `build/` directory.

[TOC]

## Master/Builder Selection

    scripts/tools/runit.py scripts/slave/runbuild.py --list-masters

will list all masters in the search path. Select a mastername
(alternatively, use --master-dir to use a specific directory).

Next, we need to pick a builder or slave hostname to build. The slave hostname
is only used to locate a suitable builder, so it need not be the actual hostname
of the slave you're on.

To list all the builders in a master, run:

    scripts/tools/runit.py scripts/slave/runbuild.py mastername --list-builders

Example, if you're in `/home/user/chromium/build/scripts/slave/`:

    scripts/tools/runit.py scripts/slave/runbuild.py chromium --list-builders

will show you which builders are available under the `chromium` master.

## Step Inspection and Execution

You can check out the list of steps without actually running them like so:

    scripts/tools/runit.py scripts/slave/runbuild.py chromium  build56-m1 --list-steps

Note that some exotic steps, such as gclient steps, won't show up in
buildrunner.) You can show the exact commands of most steps with --show-
commands:

    scripts/tools/runit.py scripts/slave/runbuild.py chromium  build56-m1 --show-commands

Finally, you can run the build with:

    scripts/tools/runit.py scripts/slave/runbuild.py mastername buildername/slavehost

Example, if you're in `/home/user/chromium/build/scripts/slave/`:

    scripts/tools/runit.py scripts/slave/runbuild.py chromium  build56-m1

or

    scripts/tools/runit.py scripts/slave/runbuild.py chromium  'Linux x64'

`--stepfilter` and `--stepreject`` can be used to filter steps to execute based
on a regex (you can see which with `--list-steps`). See `-help`for more info.

## Properties

Build properties and factory properties can be specified using `--build-
properties` and `--factory-properties`, respectively. Since build properties
contain a master and builder directive, any master or builder options on the CLI
are ignored. Properties can be inspected with either or both of --output-build-
properties or --output-factory-properties.

## Monitoring

You can specify a log destination (including '`-`' for stdout) with `--logfile`.
Enabling `--annotate` will enable annotator output.

## Using Within a Buildstep

The an annotated buildrunner can be invoked via
chromium_commands.AddBuildStep(). Set the master, builder, and any
stepfilter/reject options in factory_properties. For example usage, see
f_linux_runnertest in master.chromium.fyi/master.cfg and
check_deps2git_runner in chromium_factory.py

## More Information

Running with `--help` provides more detailed usage and options. If you have any
questions or issues please contact xusydoc@chromium.org.
