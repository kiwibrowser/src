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
#ifndef PROGRAM_H_
#define PROGRAM_H_

#include <string>
#include <vector>
#include <map>
#include "mat.h"

// Simple shader container.  Abstracts all of the OpenGL bits, but leaves
// much of the semantics intact.  This is typically only referenced directly
// by the program object.
class Shader
{
public:
    Shader() :
        handle_(0),
        type_(0),
        ready_(false),
        valid_(false) {}
    Shader(const Shader& shader) :
        handle_(shader.handle_),
        type_(shader.type_),
        source_(shader.source_),
        message_(shader.message_),
        ready_(shader.ready_),
        valid_(shader.valid_) {}
    Shader(unsigned int type, const std::string& source);
    ~Shader();

    // Compiles the shader source so that it can be linked into a
    // program.
    //
    // Make sure the shader is "valid" before calling this one.
    void compile();

    // Attaches a compiled shader to a program in preparation for
    // linking.
    //
    // Make sure the shader is "ready" before calling this one.
    void attach(unsigned int program);

    // Release any resources associated with this shader back to
    // OpenGL
    void release();

    // If "valid" then the shader has successfully been created.
    // If "ready" then the shader has successfully been compiled.
    // If either is false, then additional information can be obtained
    // from the error message.
    bool valid() const { return valid_; }
    bool ready() const { return ready_; }
    const std::string& errorMessage() const { return message_; }

private:
    unsigned int handle_;
    unsigned int type_;
    std::string source_;
    std::string message_;
    bool ready_;
    bool valid_;
};

// Simple program container.  Abstracts all of the OpenGL bits, but leaves
// much of the semantics intact.
class Program
{
public:
    Program();
    ~Program();

    // Initialize the program object for use.
    void init();

    // Release any resources associated with this program back to
    // OpenGL
    void release();

    // Create a new shader of the given type and source, compile it and
    // attach it to the program.
    //
    // Make sure the program is "valid" before calling this one.
    void addShader(unsigned int type, const std::string& source);

    // Link all of the attached shaders into a runnable program for use
    // in a rendering operation.
    //
    // Make sure the program is "valid" and that at least one shader
    // has been successfully added before calling this one.
    void build();

    // Bind the program for use by the rendering context (i.e. actually
    // run it).
    //
    // Make sure the program is "ready" before calling this one.
    void start();

    // Unbind the program from use by the rendering context (i.e. stop
    // using it).
    void stop();

    class Symbol
    {
public:
        enum SymbolType
        {
            None,
            Attribute,
            Uniform
        };
        Symbol(const std::string& name, int location, SymbolType type) :
            type_(type),
            location_(location),
            name_(name) {}
        int location() const { return location_; }
        // These members cause data to be bound to program variables, so
        // the program must be bound for use for these to be effective.
        Symbol& operator=(const LibMatrix::mat4& m);
        Symbol& operator=(const LibMatrix::mat3& m);
        Symbol& operator=(const LibMatrix::vec2& v);
        Symbol& operator=(const LibMatrix::vec3& v);
        Symbol& operator=(const LibMatrix::vec4& v);
        Symbol& operator=(const float& f);
        Symbol& operator=(const int& i);
private:
        Symbol();
        SymbolType type_;
        GLint location_;
        std::string name_;
    };
    // Get the handle to a named program input (the location in OpenGL
    // vernacular).  Typically used in conjunction with various VertexAttrib
    // interfaces.  Equality operators are used to load uniform data.
    Symbol& operator[](const std::string& name);

    // If "valid" then the program has successfully been created.
    // If "ready" then the program has successfully been built.
    // If either is false, then additional information can be obtained
    // from the error message.
    bool valid() const { return valid_; }
    bool ready() const { return ready_; }
    const std::string& errorMessage() const { return message_; }

private:
    int getAttribIndex(const std::string& name);
    int getUniformLocation(const std::string& name);
    unsigned int handle_;
    std::map<std::string, Symbol*> symbols_;
    std::vector<Shader> shaders_;
    std::string message_;
    bool ready_;
    bool valid_;
};

#endif // PROGRAM_H_
