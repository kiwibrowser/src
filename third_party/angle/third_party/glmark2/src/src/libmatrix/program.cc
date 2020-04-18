//
// Copyright (c) 2011-2012 Linaro Limited
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
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include "gl-if.h"
#include "program.h"

using std::string;
using LibMatrix::mat4;
using LibMatrix::mat3;
using LibMatrix::vec2;
using LibMatrix::vec3;
using LibMatrix::vec4;

Shader::Shader(unsigned int type, const string& source) :
    handle_(0),
    type_(type),
    source_(source),
    ready_(false),
    valid_(false)
{
    // Create our shader and setup the source code.
    handle_ = glCreateShader(type);
    if (!handle_)
    {
        message_ = string("Failed to create the new shader.");
        return;
    }
    const GLchar* shaderSource = source_.c_str();
    glShaderSource(handle_, 1, &shaderSource, NULL);
    GLint param = 0;
    glGetShaderiv(handle_, GL_SHADER_SOURCE_LENGTH, &param);
    if (static_cast<unsigned int>(param) != source_.length() + 1)
    {
        std::ostringstream o(string("Expected shader source length "));
        o << source_.length() << ", but got " << param << std::endl;
        message_ = o.str();
        return;
    }
    valid_ = true;
}

Shader::~Shader()
{
    handle_ = 0;
    type_ = 0;
    ready_ = false;
    valid_ = false;
}

void
Shader::compile()
{
    // Make sure we have a good shader and haven't already compiled it.
    if (!valid_ || ready_)
    {
        return;
    }
    glCompileShader(handle_);
    GLint param = 0;
    glGetShaderiv(handle_, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE)
    {
        glGetShaderiv(handle_, GL_INFO_LOG_LENGTH, &param);
        GLchar* infoLog = new GLchar[param + 1];
        glGetShaderInfoLog(handle_, param + 1, NULL, infoLog);
        message_ = infoLog;
        delete [] infoLog;
        return;
    }
    ready_ = true;
}

void
Shader::attach(unsigned int program)
{
    // Shader must be valid and compiled to be attached to a program.
    if (!valid_ || !ready_)
    {
        return;
    }
    glAttachShader(program, handle_);
}

void
Shader::release()
{
    if (handle_)
    {
        glDeleteShader(handle_);
    }
    handle_ = 0;
    type_ = 0;
    ready_ = false;
    valid_ = false;
}

Program::Program() :
    handle_(0),
    ready_(false),
    valid_(false)
{
}

Program::~Program()
{
    // First release all of the shader resources attached to us and clean up
    // our handle.
    release();
}

void
Program::init()
{
    handle_ = glCreateProgram();
    if (!handle_)
    {
        message_ = string("Failed to create the new program");
        return;
    }

    valid_ = true;
}

void
Program::release()
{
    // First delete all of the shader resources attached to us.
    for (std::vector<Shader>::iterator shaderIt = shaders_.begin(); shaderIt != shaders_.end(); shaderIt++)
    {
        shaderIt->release();
    }

    // Clear out the shader vector so we're ready to reuse it.
    shaders_.clear();

    // Clear out the error string to make sure we don't return anything stale.
    message_.clear();

    // Release all of the symbol map resources.
    for (std::map<string, Symbol*>::iterator symbolIt = symbols_.begin(); symbolIt != symbols_.end(); symbolIt++)
    {
        delete (*symbolIt).second;
    }
    symbols_.clear();

    if (handle_)
    {
        glDeleteProgram(handle_);
    }
    handle_ = 0;
    ready_ = false;
    valid_ = false;
}
void
Program::addShader(unsigned int type, const string& source)
{
    if (!valid_)
    {
        return;
    }

    Shader shader(type, source);
    if (!shader.valid())
    {
        message_ = shader.errorMessage();
        valid_ = false;
        return;
    }

    shader.compile();

    if (!shader.ready())
    {
        message_ = shader.errorMessage();
        valid_ = false;
        return;
    }

    shader.attach(handle_);
    shaders_.push_back(shader);
    return;
}

void
Program::build()
{
    if (!valid_ || ready_)
    {
        return;
    }

    if (shaders_.empty())
    {
        message_ = string("There are no shaders attached to this program");
        return;
    }

    glLinkProgram(handle_);
    GLint param = 1;
    glGetProgramiv(handle_, GL_LINK_STATUS, &param);
    if (param == GL_FALSE)
    {
        glGetProgramiv(handle_, GL_INFO_LOG_LENGTH, &param);
        GLchar* infoLog = new GLchar[param + 1];
        glGetProgramInfoLog(handle_, param + 1, NULL, infoLog);
        message_ = infoLog;
        delete [] infoLog;
        return;
    }
    ready_ = true;
}

void
Program::start()
{
    if (!valid_ || !ready_)
    {
        return;
    }
    glUseProgram(handle_);
}

void
Program::stop()
{
    glUseProgram(0);
}


int
Program::getUniformLocation(const string& name)
{
    GLint location = glGetUniformLocation(handle_, name.c_str());
    if (location < 0)
    {
        message_ = string("Failed to get uniform location for \"") + name +
            string("\"");
    }
    return location;
}

int
Program::getAttribIndex(const string& name)
{
    GLint index = glGetAttribLocation(handle_, name.c_str());
    if (index < 0)
    {
        message_ = string("Failed to get attribute location for \"") + name +
            string("\"");
    }
    return index;
}

Program::Symbol&
Program::Symbol::operator=(const mat4& m)
{
    if (type_ == Uniform)
    {
        // Our matrix representation is column-major, so transpose is false here.
        glUniformMatrix4fv(location_, 1, GL_FALSE, m);
    }
    return *this;
}

Program::Symbol&
Program::Symbol::operator=(const mat3& m)
{
    if (type_ == Uniform)
    {
        // Our matrix representation is column-major, so transpose is false here.
        glUniformMatrix3fv(location_, 1, GL_FALSE, m);
    }
    return *this;
}

Program::Symbol&
Program::Symbol::operator=(const vec2& v)
{
    if (type_ == Uniform)
    {
        glUniform2fv(location_, 1, v);
    }
    return *this;
}

Program::Symbol&
Program::Symbol::operator=(const vec3& v)
{
    if (type_ == Uniform)
    {
        glUniform3fv(location_, 1, v);
    }
    return *this;
}

Program::Symbol&
Program::Symbol::operator=(const vec4& v)
{
    if (type_ == Uniform)
    {
        glUniform4fv(location_, 1, v);
    }
    return *this;
}

Program::Symbol&
Program::Symbol::operator=(const float& f)
{
    if (type_ == Uniform)
    {
        glUniform1f(location_, f);
    }
    return *this;
}

Program::Symbol&
Program::Symbol::operator=(const int& i)
{
    if (type_ == Uniform)
    {
        glUniform1i(location_, i);
    }
    return *this;
}

Program::Symbol&
Program::operator[](const std::string& name)
{
    std::map<std::string, Symbol*>::iterator mapIt = symbols_.find(name);
    if (mapIt == symbols_.end())
    {
        Program::Symbol::SymbolType type(Program::Symbol::Attribute);
        int location = getAttribIndex(name);
        if (location < 0)
        {
            // No attribute found by that name.  Let's try a uniform...
            type = Program::Symbol::Uniform;
            location = getUniformLocation(name);
            if (location < 0)
            {
                type = Program::Symbol::None;
            }
        }
        mapIt = symbols_.insert(mapIt, std::make_pair(name, new Symbol(name, location, type)));
    }
    return *(*mapIt).second;
}
