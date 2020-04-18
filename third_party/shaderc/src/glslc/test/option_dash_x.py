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
from glslc_test_framework import inside_glslc_testsuite
from placeholder import FileShader

MINIMAL_SHADER = "#version 140\nvoid main(){}"

@inside_glslc_testsuite('OptionDashX')
class TestDashXNoArg(expect.ErrorMessage):
    """Tests -x with nothing."""

    glslc_args = ['-x']
    expected_error = [
        "glslc: error: argument to '-x' is missing (expected 1 value)\n",
        'glslc: error: no input files\n']


@inside_glslc_testsuite('OptionDashX')
class TestDashXGlsl(expect.ValidObjectFile):
    """Tests -x glsl."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-x', 'glsl', '-c', shader]


@inside_glslc_testsuite('OptionDashX')
class TestDashXWrongParam(expect.ErrorMessage):
    """Tests -x with wrong parameter."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-x', 'gl', shader]
    expected_error = ["glslc: error: language not recognized: 'gl'\n"]


@inside_glslc_testsuite('OptionDashX')
class TestMultipleDashX(expect.ValidObjectFile):
    """Tests that multiple -x glsl works."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-c', '-x', 'glsl', '-x', 'glsl', shader, '-x', 'glsl']


@inside_glslc_testsuite('OptionDashX')
class TestMultipleDashXCorrectWrong(expect.ErrorMessage):
    """Tests -x glsl -x [wrong-language]."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-x', 'glsl', '-x', 'foo', shader]
    expected_error = ["glslc: error: language not recognized: 'foo'\n"]


@inside_glslc_testsuite('OptionDashX')
class TestMultipleDashXWrongCorrect(expect.ErrorMessage):
    """Tests -x [wrong-language] -x glsl."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-xbar', '-x', 'glsl', shader]
    expected_error = ["glslc: error: language not recognized: 'bar'\n"]


@inside_glslc_testsuite('OptionDashX')
class TestDashXGlslConcatenated(expect.ValidObjectFile):
    """Tests -xglsl."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-xglsl', shader, '-c']


@inside_glslc_testsuite('OptionDashX')
class TestDashXWrongParamConcatenated(expect.ErrorMessage):
    """Tests -x concatenated with a wrong language."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-xsl', shader]
    expected_error = ["glslc: error: language not recognized: 'sl'\n"]


@inside_glslc_testsuite('OptionDashX')
class TestDashXEmpty(expect.ErrorMessage):
    """Tests -x ''."""

    shader = FileShader(MINIMAL_SHADER, '.vert')
    glslc_args = ['-x', '', shader]
    expected_error = ["glslc: error: language not recognized: ''\n"]
