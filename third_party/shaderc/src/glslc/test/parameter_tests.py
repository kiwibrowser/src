# Copyright 2015 The Shaderc Authors. All rights reserved.
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

import expect
import os.path
from glslc_test_framework import inside_glslc_testsuite
from placeholder import FileShader, StdinShader, TempFileName


@inside_glslc_testsuite('File')
class SimpleFileCompiled(expect.ValidObjectFile):
    """Tests whether or not a simple glsl file compiles."""

    shader = FileShader('#version 310 es\nvoid main() {}', '.frag')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('File')
class NotSpecifyingOutputName(expect.SuccessfulReturn,
                              expect.CorrectObjectFilePreamble):
    """Tests that when there is no -o and -E/-S/-c specified, output as a.spv."""

    shader = FileShader('#version 140\nvoid main() {}', '.frag')
    glslc_args = [shader]

    def check_output_a_spv(self, status):
        output_name = os.path.join(status.directory, 'a.spv')
        return self.verify_object_file_preamble(output_name)


@inside_glslc_testsuite('Parameters')
class HelpParameters(
    expect.ReturnCodeIsZero, expect.StdoutMatch, expect.StderrMatch):
    """Tests the --help flag outputs correctly and does not produce and error."""

    glslc_args = ['--help']

    expected_stdout = '''glslc - Compile shaders into SPIR-V

Usage: glslc [options] file...

An input file of - represents standard input.

Options:
  -c                Only run preprocess, compile, and assemble steps.
  -Dmacro[=defn]    Add an implicit macro definition.
  -E                Outputs only the results of the preprocessing step.
                    Output defaults to standard output.
  -fshader-stage=<stage>
                    Treat subsequent input files as having stage <stage>.
                    Valid stages are vertex, fragment, tesscontrol, tesseval,
                    geometry, and compute.
  -g                Generate source-level debug information.
                    Currently this option has no effect.
  --help            Display available options.
  --version         Display compiler version information.
  -I <value>        Add directory to include search path.
  -o <file>         Write output to <file>.
                    A file name of '-' represents standard output.
  -std=<value>      Version and profile for input files. Possible values
                    are concatenations of version and profile, e.g. 310es,
                    450core, etc.
  -M                Generate make dependencies. Implies -E and -w.
  -MM               An alias for -M.
  -MD               Generate make dependencies and compile.
  -MF <file>        Write dependency output to the given file.
  -MT <target>      Specify the target of the rule emitted by dependency
                    generation.
  -S                Only run preprocess and compilation steps.
  --target-env=<environment>
                    Set the target shader environment, and the semantics
                    of warnings and errors. Valid values are 'opengl',
                    'opengl_compat' and 'vulkan'. The default value is 'vulkan'.
  -w                Suppresses all warning messages.
  -Werror           Treat all warnings as errors.
  -x <language>     Treat subsequent input files as having type <language>.
                    The only supported language is glsl.
'''

    expected_stderr = ''


@inside_glslc_testsuite('Parameters')
class HelpIsNotTooWide(expect.StdoutNoWiderThan80Columns):
    """Tests that --help output is not too wide."""

    glslc_args = ['--help']


@inside_glslc_testsuite('Parameters')
class UnknownSingleLetterArgument(expect.ErrorMessage):
    """Tests that an unknown argument triggers an error message."""

    glslc_args = ['-a']
    expected_error = ["glslc: error: unknown argument: '-a'\n"]


@inside_glslc_testsuite('Parameters')
class UnknownMultiLetterArgument(expect.ErrorMessage):
    """Tests that an unknown argument triggers an error message."""

    glslc_args = ['-zzz']
    expected_error = ["glslc: error: unknown argument: '-zzz'\n"]


@inside_glslc_testsuite('Parameters')
class UnsupportedOption(expect.ErrorMessage):
    """Tests that an unsupported option triggers an error message."""

    glslc_args = ['--unsupported-option']
    expected_error = [
        "glslc: error: unsupported option: '--unsupported-option'\n"]


@inside_glslc_testsuite('File')
class FileNotFound(expect.ErrorMessage):
    """Tests the error message if a file cannot be found."""

    blabla_file = TempFileName('blabla.frag')
    glslc_args = [blabla_file]
    expected_error = [
        "glslc: error: cannot open input file: '", blabla_file,
        "': No such file or directory\n"]


@inside_glslc_testsuite('Unsupported')
class LinkingNotSupported(expect.ErrorMessage):
    """Tests the error message generated by linking not supported yet."""

    shader1 = FileShader('#version 140\nvoid main() {}', '.vert')
    shader2 = FileShader('#version 140\nvoid main() {}', '.frag')
    glslc_args = [shader1, shader2]
    expected_error = [
        'glslc: error: linking multiple files is not supported yet. ',
        'Use -c to compile files individually.\n']


@inside_glslc_testsuite('Unsupported')
class MultipleStdinUnsupported(expect.ErrorMessage):
    """Tests the error message generated by having more than one - input."""

    glslc_args = ['-c', '-fshader-stage=vertex', '-', '-']
    expected_error = [
        'glslc: error: specifying standard input "-" as input more'
        ' than once is not allowed.\n']


@inside_glslc_testsuite('Parameters')
class StdinWithoutShaderStage(expect.StdoutMatch, expect.StderrMatch):
    """Tests that you must use -fshader-stage when specifying - as input."""
    shader = StdinShader(
        """#version 140
    int a() {
    }
    void main() {
      int x = a();
    }
    """)
    glslc_args = [shader]

    expected_stdout = ''
    expected_stderr = [
        "glslc: error: '-': -fshader-stage required when input is from "
        'standard input "-"\n']
