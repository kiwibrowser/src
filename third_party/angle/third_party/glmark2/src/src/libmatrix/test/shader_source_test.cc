//
// Copyright (c) 2012 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Jesse Barker - original implementation.
//
#include <string>
#include "libmatrix_test.h"
#include "shader_source_test.h"
#include "../shader-source.h"
#include "../vec.h"

using std::string;
using LibMatrix::vec4;

void
ShaderSourceBasic::run(const Options& options)
{
    static const string vtx_shader_filename("test/basic.vert");

    ShaderSource vtx_source(vtx_shader_filename);
    ShaderSource vtx_source2(vtx_shader_filename);
    
    pass_ = (vtx_source.str() == vtx_source2.str());
}

void
ShaderSourceAddConstGlobal::run(const Options& options)
{
    // Load the original shader source.
    static const string src_shader_filename("test/basic.vert");
    ShaderSource src_shader(src_shader_filename);

    // Add constant at global scope
    static const vec4 constantColor(1.0, 1.0, 1.0, 1.0);
    src_shader.add_const("ConstantColor", constantColor);

    // Load the pre-modified shader
    static const string result_shader_filename("test/basic-global-const.vert");
    ShaderSource result_shader(result_shader_filename);

    // Compare the output strings to confirm the results.
    pass_ = (src_shader.str() == result_shader.str());
}
