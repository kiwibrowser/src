#!/usr/bin/env python2
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

import os, sys
from collections import namedtuple
import xml.etree.ElementTree as etree

from generator_lib import Generator, run_generator, FileRender

class Proc:
    def __init__(self, gl_name, proc_name=None):
        assert(gl_name.startswith('gl'))
        if proc_name == None:
            proc_name = gl_name[2:]

        self.gl_name = gl_name
        self.proc_name = proc_name

    def glProcName(self):
        return self.gl_name

    def ProcName(self):
        return self.proc_name

    def PFNPROCNAME(self):
        return 'PFN' + self.gl_name.upper() + 'PROC'

    def __repr__(self):
        return 'Proc("{}", "{}")'.format(self.gl_name, self.proc_name)

Version = namedtuple('Version', ['major', 'minor'])
VersionProcBlock = namedtuple('ProcBlock', ['version', 'procs'])
HeaderProcBlock = namedtuple('ProcBlock', ['description', 'procs'])

def parse_version(version):
    return Version(*map(int, version.split('.')))

def compute_params(root):
    # Add proc blocks for OpenGL ES
    gles_blocks = []
    for gles_section in root.findall('''feature[@api='gles2']'''):
        section_procs = []
        for proc in gles_section.findall('./require/command'):
            section_procs.append(Proc(proc.attrib['name']))
        gles_blocks.append(VersionProcBlock(parse_version(gles_section.attrib['number']), section_procs))

    # Get the list of all Desktop OpenGL function removed by the Core Profile.
    core_removed_procs = set()
    for removed_section in root.findall('feature/remove'):
        assert(removed_section.attrib['profile'] == 'core')
        for proc in removed_section.findall('./command'):
            core_removed_procs.add(proc.attrib['name'])

    # Add proc blocks for Desktop GL
    desktop_gl_blocks = []
    for gl_section in root.findall('''feature[@api='gl']'''):
        section_procs = []
        for proc in gl_section.findall('./require/command'):
            if proc.attrib['name'] not in core_removed_procs:
                section_procs.append(Proc(proc.attrib['name']))
        desktop_gl_blocks.append(VersionProcBlock(parse_version(gl_section.attrib['number']), section_procs))

    already_added_header_procs = set()
    header_blocks = []
    def add_header_block(description, procs):
        block_procs = []
        for proc in procs:
            if not proc.glProcName() in already_added_header_procs:
                already_added_header_procs.add(proc.glProcName())
                block_procs.append(proc)
        if len(block_procs) > 0:
            header_blocks.append(HeaderProcBlock(description, block_procs))

    for block in gles_blocks:
        add_header_block('OpenGL ES {}.{}'.format(block.version.major, block.version.minor), block.procs)

    for block in desktop_gl_blocks:
        add_header_block('Desktop OpenGL {}.{}'.format(block.version.major, block.version.minor), block.procs)

    return {
        'gles_blocks': gles_blocks,
        'desktop_gl_blocks': desktop_gl_blocks,
        'header_blocks': header_blocks,
    }

class OpenGLLoaderGenerator(Generator):
    def get_description(self):
        return 'Generates code to load OpenGL function pointers'

    def add_commandline_arguments(self, parser):
        parser.add_argument('--gl-xml', required=True, type=str, help ='The Khronos gl.xml to use.')

    def get_file_renders(self, args):
        params = compute_params(etree.parse(args.gl_xml).getroot())

        return [
            FileRender('opengl/OpenGLFunctionsBase.cpp', 'dawn_native/opengl/OpenGLFunctionsBase_autogen.cpp', [params]),
            FileRender('opengl/OpenGLFunctionsBase.h', 'dawn_native/opengl/OpenGLFunctionsBase_autogen.h', [params]),
        ]

    def get_dependencies(self, args):
        return [os.path.abspath(args.gl_xml)]

if __name__ == '__main__':
    sys.exit(run_generator(OpenGLLoaderGenerator()))
