#!/usr/bin/env python3
# Copyright 2019 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Module to create generators that render multiple Jinja2 templates for GN.

A helper module that can be used to create generator scripts (clients)
that expand one or more Jinja2 templates, without outputs usable from
GN and Ninja build-based systems. See generator_lib.gni as well.

Clients should create a Generator sub-class, then call run_generator()
with a proper derived class instance.

Clients specify a list of FileRender operations, each one of them will
output a file into a temporary output directory through Jinja2 expansion.
All temporary output files are then grouped and written to into a single JSON
file, that acts as a convenient single GN output target. Use extract_json.py
to extract the output files from the JSON tarball in another GN action.

--depfile can be used to specify an output Ninja dependency file for the
JSON tarball, to ensure it is regenerated any time one of its dependencies
changes.

Finally, --expected-output-files can be used to check the list of generated
output files.
"""

import argparse, json, os, re, sys
from collections import namedtuple

# A FileRender represents a single Jinja2 template render operation:
#
#   template: Jinja2 template name, relative to --template-dir path.
#
#   output: Output file path, relative to temporary output directory.
#
#   params_dicts: iterable of (name:string -> value:string) dictionaries.
#       All of them will be merged before being sent as Jinja2 template
#       expansion parameters.
#
# Example:
#   FileRender('api.c', 'src/project_api.c', [{'PROJECT_VERSION': '1.0.0'}])
#
FileRender = namedtuple('FileRender', ['template', 'output', 'params_dicts'])


# The interface that must be implemented by generators.
class Generator:
    def get_description(self):
        """Return generator description for --help."""
        return ""

    def add_commandline_arguments(self, parser):
        """Add generator-specific argparse arguments."""
        pass

    def get_file_renders(self, args):
        """Return the list of FileRender objects to process."""
        return []

    def get_dependencies(self, args):
        """Return a list of extra input dependencies."""
        return []


# Allow custom Jinja2 installation path through an additional python
# path from the arguments if present. This isn't done through the regular
# argparse because PreprocessingLoader uses jinja2 in the global scope before
# "main" gets to run.
#
# NOTE: If this argument appears several times, this only uses the first
#       value, while argparse would typically keep the last one!
kJinja2Path = '--jinja2-path'
try:
    jinja2_path_argv_index = sys.argv.index(kJinja2Path)
    # Add parent path for the import to succeed.
    path = os.path.join(sys.argv[jinja2_path_argv_index + 1], os.pardir)
    sys.path.insert(1, path)
except ValueError:
    # --jinja2-path isn't passed, ignore the exception and just import Jinja2
    # assuming it already is in the Python PATH.
    pass

import jinja2


# A custom Jinja2 template loader that removes the extra indentation
# of the template blocks so that the output is correctly indented
class _PreprocessingLoader(jinja2.BaseLoader):
    def __init__(self, path):
        self.path = path

    def get_source(self, environment, template):
        path = os.path.join(self.path, template)
        if not os.path.exists(path):
            raise jinja2.TemplateNotFound(template)
        mtime = os.path.getmtime(path)
        with open(path) as f:
            source = self.preprocess(f.read())
        return source, path, lambda: mtime == os.path.getmtime(path)

    blockstart = re.compile('{%-?\s*(if|elif|else|for|block|macro)[^}]*%}')
    blockend = re.compile('{%-?\s*(end(if|for|block|macro)|elif|else)[^}]*%}')

    def preprocess(self, source):
        lines = source.split('\n')

        # Compute the current indentation level of the template blocks and
        # remove their indentation
        result = []
        indentation_level = 0

        # Filter lines that are pure comments. line_comment_prefix is not
        # enough because it removes the comment but doesn't completely remove
        # the line, resulting in more verbose output.
        lines = filter(lambda line: not line.strip().startswith('//*'), lines)

        # Remove indentation templates have for the Jinja control flow.
        for line in lines:
            # The capture in the regex adds one element per block start or end,
            # so we divide by two. There is also an extra line chunk
            # corresponding to the line end, so we subtract it.
            numends = (len(self.blockend.split(line)) - 1) // 2
            indentation_level -= numends

            result.append(self.remove_indentation(line, indentation_level))

            numstarts = (len(self.blockstart.split(line)) - 1) // 2
            indentation_level += numstarts

        return '\n'.join(result) + '\n'

    def remove_indentation(self, line, n):
        for _ in range(n):
            if line.startswith(' '):
                line = line[4:]
            elif line.startswith('\t'):
                line = line[1:]
            else:
                assert line.strip() == ''
        return line


_FileOutput = namedtuple('FileOutput', ['name', 'content'])


def _do_renders(renders, template_dir):
    loader = _PreprocessingLoader(template_dir)
    env = jinja2.Environment(loader=loader,
                             lstrip_blocks=True,
                             trim_blocks=True,
                             line_comment_prefix='//*')

    def do_assert(expr):
        assert expr
        return ''

    def debug(text):
        print(text)

    base_params = {
        'enumerate': enumerate,
        'format': format,
        'len': len,
        'debug': debug,
        'assert': do_assert,
    }

    outputs = []
    for render in renders:
        params = {}
        params.update(base_params)
        for param_dict in render.params_dicts:
            params.update(param_dict)
        content = env.get_template(render.template).render(**params)
        outputs.append(_FileOutput(render.output, content))

    return outputs


# Compute the list of imported, non-system Python modules.
# It assumes that any path outside of the root directory is system.
def _compute_python_dependencies(root_dir=None):
    if not root_dir:
        # Assume this script is under generator/ by default.
        root_dir = os.path.join(os.path.dirname(__file__), os.pardir)
    root_dir = os.path.abspath(root_dir)

    module_paths = (module.__file__ for module in sys.modules.values()
                    if module and hasattr(module, '__file__'))

    paths = set()
    for path in module_paths:
        path = os.path.abspath(path)

        if not path.startswith(root_dir):
            continue

        if (path.endswith('.pyc')
                or (path.endswith('c') and not os.path.splitext(path)[1])):
            path = path[:-1]

        paths.add(path)

    return paths


def run_generator(generator):
    parser = argparse.ArgumentParser(
        description=generator.get_description(),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    generator.add_commandline_arguments(parser)
    parser.add_argument('--template-dir',
                        default='templates',
                        type=str,
                        help='Directory with template files.')
    parser.add_argument(
        kJinja2Path,
        default=None,
        type=str,
        help='Additional python path to set before loading Jinja2')
    parser.add_argument(
        '--output-json-tarball',
        default=None,
        type=str,
        help=('Name of the "JSON tarball" to create (tar is too annoying '
              'to use in python).'))
    parser.add_argument(
        '--depfile',
        default=None,
        type=str,
        help='Name of the Ninja depfile to create for the JSON tarball')
    parser.add_argument(
        '--expected-outputs-file',
        default=None,
        type=str,
        help="File to compare outputs with and fail if it doesn't match")
    parser.add_argument(
        '--root-dir',
        default=None,
        type=str,
        help=('Optional source root directory for Python dependency '
              'computations'))
    parser.add_argument(
        '--allowed-output-dirs-file',
        default=None,
        type=str,
        help=("File containing a list of allowed directories where files "
              "can be output."))
    parser.add_argument(
        '--print-cmake-dependencies',
        default=False,
        action="store_true",
        help=("Prints a semi-colon separated list of dependencies to "
              "stdout and exits."))
    parser.add_argument(
        '--print-cmake-outputs',
        default=False,
        action="store_true",
        help=("Prints a semi-colon separated list of outputs to "
              "stdout and exits."))
    parser.add_argument('--output-dir',
                        default=None,
                        type=str,
                        help='Directory where to output generate files.')

    args = parser.parse_args()

    renders = generator.get_file_renders(args)

    # Output a list of all dependencies for CMake or the tarball for GN/Ninja.
    if args.depfile != None or args.print_cmake_dependencies:
        dependencies = generator.get_dependencies(args)
        dependencies += [
            args.template_dir + os.path.sep + render.template
            for render in renders
        ]
        dependencies += _compute_python_dependencies(args.root_dir)

        if args.depfile != None:
            with open(args.depfile, 'w') as f:
                f.write(args.output_json_tarball + ": " +
                        " ".join(dependencies))

        if args.print_cmake_dependencies:
            sys.stdout.write(";".join(dependencies))
            return 0

    # The caller wants to assert that the outputs are what it expects.
    # Load the file and compare with our renders.
    if args.expected_outputs_file != None:
        with open(args.expected_outputs_file) as f:
            expected = set([line.strip() for line in f.readlines()])

        actual = {render.output for render in renders}

        if actual != expected:
            print("Wrong expected outputs, caller expected:\n    " +
                  repr(sorted(expected)))
            print("Actual output:\n    " + repr(sorted(actual)))
            return 1

    # Print the list of all the outputs for cmake.
    if args.print_cmake_outputs:
        sys.stdout.write(";".join([
            os.path.join(args.output_dir, render.output) for render in renders
        ]))
        return 0

    outputs = _do_renders(renders, args.template_dir)

    # The caller wants to assert that the outputs are only in specific
    # directories.
    if args.allowed_output_dirs_file != None:
        with open(args.allowed_output_dirs_file) as f:
            allowed_dirs = set([line.strip() for line in f.readlines()])

        for directory in allowed_dirs:
            if not directory.endswith('/'):
                print('Allowed directory entry "{}" doesn\'t '
                      'end with /'.format(directory))
                return 1

        def check_in_subdirectory(path, directory):
            return path.startswith(
                directory) and not '/' in path[len(directory):]

        for render in renders:
            if not any(
                    check_in_subdirectory(render.output, directory)
                    for directory in allowed_dirs):
                print('Output file "{}" is not in the allowed directory '
                      'list below:'.format(render.output))
                for directory in sorted(allowed_dirs):
                    print('    "{}"'.format(directory))
                return 1

    # Output the JSON tarball
    if args.output_json_tarball != None:
        json_root = {}
        for output in outputs:
            json_root[output.name] = output.content

        with open(args.output_json_tarball, 'w') as f:
            f.write(json.dumps(json_root))

    # Output the files directly.
    if args.output_dir != None:
        for output in outputs:
            output_path = os.path.join(args.output_dir, output.name)

            directory = os.path.dirname(output_path)
            if not os.path.exists(directory):
                os.makedirs(directory)

            with open(output_path, 'w') as outfile:
                outfile.write(output.content)
