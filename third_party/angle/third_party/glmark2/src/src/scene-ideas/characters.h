/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * Copyright © 2012 Linaro Limited
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Jesse Barker
 */
#ifndef CHARACTERS_H_
#define CHARACTERS_H_

#include <vector>
#include "vec.h"
#include "gl-headers.h"

class PrimitiveState
{
public:
    PrimitiveState(unsigned int type, unsigned int count, unsigned int offset) :
        type_(type),
        count_(count),
        bufferOffset_(offset) {}
    ~PrimitiveState() {}
    void issue() const
    {
        glDrawElements(type_, count_, GL_UNSIGNED_SHORT, 
            reinterpret_cast<const GLvoid*>(bufferOffset_));
    }
private:
    PrimitiveState();
    unsigned int type_;         // Primitive type (e.g. GL_TRIANGLE_STRIP)
    unsigned int count_;        // Number of primitives
    unsigned int bufferOffset_; // Offset into the element array buffer
};

struct Character
{
    void draw()
    {
        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);
        glVertexAttribPointer(vertexIndex_, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vertexIndex_);
        for (std::vector<PrimitiveState>::const_iterator primIt = primVec_.begin();
             primIt != primVec_.end();
             primIt++)
        {
            primIt->issue();
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    void init(int vertexAttribIndex) 
    {
        vertexIndex_ = vertexAttribIndex;

        // We need 2 buffers for our work here.  One for the vertex data.
        // and one for the index data.
        glGenBuffers(2, &bufferObjects_[0]);

        // First, setup the vertex data by binding the first buffer object, 
        // allocating its data store, and filling it in with our vertex data.
        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
        glBufferData(GL_ARRAY_BUFFER, vertexData_.size() * sizeof(LibMatrix::vec2), 
            &vertexData_.front(), GL_STATIC_DRAW);

        // Finally, setup the pointer to our vertex data and enable this
        // attribute array.
        glVertexAttribPointer(vertexIndex_, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vertexIndex_);

        // Now repeat for our index data.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
            indexData_.size() * sizeof(unsigned short), &indexData_.front(), 
            GL_STATIC_DRAW);

        // Unbind our vertex buffer objects so that their state isn't affected
        // by other objects.
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    ~Character()
    {
        glDeleteBuffers(2, &bufferObjects_[0]);
    }
    Character() :
        vertexIndex_(0),
        vertexArray_(0) {}
    unsigned int bufferObjects_[2];
    std::vector<LibMatrix::vec2> vertexData_;
    std::vector<unsigned short> indexData_;
    int vertexIndex_;
    unsigned int vertexArray_;
    std::vector<PrimitiveState> primVec_;
};

struct LetterI : Character
{
    LetterI();
};

struct LetterD : Character
{
    LetterD();
};

struct LetterE : Character
{
    LetterE();
};

struct LetterA : Character
{
    LetterA();
};

struct LetterS : Character
{
    LetterS();
};

struct LetterN : Character
{
    LetterN();
};

struct LetterM : Character
{
    LetterM();
};

struct LetterO : Character
{
    LetterO();
};

struct LetterT : Character
{
    LetterT();
};

#endif // CHARACTERS_H_
