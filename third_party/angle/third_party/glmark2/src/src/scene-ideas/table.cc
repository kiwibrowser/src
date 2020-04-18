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
#include "table.h"
#include "scene.h"
#include "shader-source.h"
#include "log.h"

using std::string;
using LibMatrix::vec3;
using LibMatrix::Stack4;

const string Table::modelviewName_("modelview");
const string Table::projectionName_("projection");
const string Table::lightPositionName_("lightPosition");
const string Table::logoDirectionName_("logoDirection");
const string Table::curTimeName_("currentTime");
const string Table::vertexAttribName_("vertex");
const unsigned int Table::TABLERES_(12);
const vec3 Table::paperVertices_[4] = {
    vec3(-0.8, 0.0, 0.4),
    vec3(-0.2, 0.0, -1.4),
    vec3(0.4, 0.0, 0.8),
    vec3(1.0, 0.0, -1.0),
};

Table::Table() :
    tableVertexIndex_(0),
    paperVertexIndex_(0),
    textVertexIndex_(0),
    underVertexIndex_(0),
    valid_(false)
{
    tableVertices_.reserve((TABLERES_ + 1) * (TABLERES_ + 1));
    for (unsigned int i = 0; i <= TABLERES_; i++)
    {
        for (unsigned int j = 0; j <= TABLERES_; j++)
        {
            float x((static_cast<float>(i) - static_cast<float>(TABLERES_) * 1.0 / 2.0) / 2.0);
            float z((static_cast<float>(j) - static_cast<float>(TABLERES_) * 1.0 / 2.0) / 2.0);
            tableVertices_.push_back(vec3(x, 0.0, z));
        }
    }

    // Now that we've setup the vertex data, we can setup the map of how
    // that data will be laid out in the buffer object.
    dataMap_.tvOffset = 0;
    dataMap_.tvSize = tableVertices_.size() * sizeof(vec3);
    dataMap_.totalSize = dataMap_.tvSize;
    dataMap_.pvOffset = dataMap_.tvOffset + dataMap_.tvSize;
    dataMap_.pvSize = 4 * sizeof(vec3);
    dataMap_.totalSize += dataMap_.pvSize;

    for (unsigned int i = 0; i < TABLERES_; i++)
    {
        for (unsigned int j = 0; j <= TABLERES_; j++)
        {
            unsigned int curIndex1(i * (TABLERES_ + 1) + j);
            unsigned int curIndex2((i + 1) * (TABLERES_ + 1) + j);
            indexData_.push_back(curIndex1);
            indexData_.push_back(curIndex2);
        }
    }
}

Table::~Table(void)
{
    if (valid_)
    {
        glDeleteBuffers(2, &bufferObjects_[0]);
    }
}

void
Table::init(void)
{
    // Make sure we don't re-initialize...
    if (valid_)
    {
        return;
    }

    // Initialize shader sources from input files and create programs from them
    // Program to render the table with lighting and a time-based fade...
    string table_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-table.vert");
    string table_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-table.frag");
    ShaderSource table_vtx_source(table_vtx_filename);
    ShaderSource table_frg_source(table_frg_filename);
    if (!Scene::load_shaders_from_strings(tableProgram_, table_vtx_source.str(),
                                          table_frg_source.str()))
    {
        Log::error("No valid program for table rendering.\n");
        return;
    }
    tableVertexIndex_ = tableProgram_[vertexAttribName_].location();

    // Program to render the paper with lighting and a time-based fade...
    string paper_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-paper.vert");
    string paper_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-paper.frag");
    ShaderSource paper_vtx_source(paper_vtx_filename);
    ShaderSource paper_frg_source(paper_frg_filename);
    if (!Scene::load_shaders_from_strings(paperProgram_, paper_vtx_source.str(),
                                          paper_frg_source.str()))
    {
        Log::error("No valid program for paper rendering.\n");
        return;
    }
    paperVertexIndex_ = paperProgram_[vertexAttribName_].location();

    // Program to handle the text (time-based color fade)...
    string text_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-text.vert");
    string text_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-text.frag");
    ShaderSource text_vtx_source(text_vtx_filename);
    ShaderSource text_frg_source(text_frg_filename);
    if (!Scene::load_shaders_from_strings(textProgram_, text_vtx_source.str(),
                                          text_frg_source.str()))
    {
        Log::error("No valid program for text rendering.\n");
        return;
    }
    textVertexIndex_ = textProgram_[vertexAttribName_].location();

    // Program for the drawUnder functionality (just paint it black)...
    string under_table_vtx_filename(GLMARK_DATA_PATH"/shaders/ideas-under-table.vert");
    string under_table_frg_filename(GLMARK_DATA_PATH"/shaders/ideas-under-table.frag");
    ShaderSource under_table_vtx_source(under_table_vtx_filename);
    ShaderSource under_table_frg_source(under_table_frg_filename);
    if (!Scene::load_shaders_from_strings(underProgram_, under_table_vtx_source.str(),
                                          under_table_frg_source.str()))
    {
        Log::error("No valid program for under table rendering.\n");
        return;
    }
    underVertexIndex_ = underProgram_[vertexAttribName_].location();

    // Tell all of the characters to initialize themselves...
    i_.init(textVertexIndex_);
    d_.init(textVertexIndex_);
    e_.init(textVertexIndex_);
    a_.init(textVertexIndex_);
    s_.init(textVertexIndex_);
    n_.init(textVertexIndex_);
    m_.init(textVertexIndex_);
    o_.init(textVertexIndex_);
    t_.init(textVertexIndex_);

    // We need 2 buffers for our work here.  One for the vertex data.
    // and one for the index data.
    glGenBuffers(2, &bufferObjects_[0]);

    // First, setup the vertex data by binding the first buffer object, 
    // allocating its data store, and filling it in with our vertex data.
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBufferData(GL_ARRAY_BUFFER, dataMap_.totalSize, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.tvOffset, dataMap_.tvSize, 
                    &tableVertices_.front());
    glBufferSubData(GL_ARRAY_BUFFER, dataMap_.pvOffset, dataMap_.pvSize,
                    &paperVertices_[0]);

    // Now repeat for our index data.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData_.size() * sizeof(unsigned short),
                 &indexData_.front(), GL_STATIC_DRAW);

    // We're ready to go.
    valid_ = true;
}

void
Table::draw(Stack4& modelview,
    Stack4& projection,
    const vec3& lightPos,
    const vec3& logoPos,
    const float& currentTime,
    float& paperAlpha_out)
{
    glDisable(GL_DEPTH_TEST);

    // Compute the light direction with respect to the logo...
    vec3 logoDirection(lightPos.x() - logoPos.x(), lightPos.y() - logoPos.y(), 
        lightPos.z() - logoPos.z());
    logoDirection.normalize();

    // Compute the alpha component based upon drawing the paper (all of this will
    // be done in the shader, but we need to pass this back so that the logo's
    // shadow will look right).
    for (unsigned int i = 0; i < 4; i++)
    {
        vec3 lightDirection(lightPos.x() - paperVertices_[i].x(),
                            lightPos.y() - paperVertices_[i].y(),
                            lightPos.z() - paperVertices_[i].z());
        lightDirection.normalize();
        float c = vec3::dot(lightDirection, logoDirection);
        if (c < 0.0)
        {
            c = 0.0;
        }
        c = c * c * c * lightDirection.y();
        if ((currentTime > 10.0) && (currentTime < 12.0))
        {
            c *= 1.0 - (currentTime - 10.0) * 0.5;
        }
        paperAlpha_out += c;
    }

    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);

    // Draw the table top
    tableProgram_.start();
    tableProgram_[projectionName_] = projection.getCurrent();
    tableProgram_[modelviewName_] = modelview.getCurrent();
    tableProgram_[lightPositionName_] = lightPos;
    tableProgram_[logoDirectionName_] = logoDirection;
    tableProgram_[curTimeName_] = currentTime;
    glVertexAttribPointer(tableVertexIndex_, 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.tvOffset));
    glEnableVertexAttribArray(tableVertexIndex_);
    static const unsigned int twiceRes(2 * (TABLERES_ + 1));
    for (unsigned int i = 0; i < TABLERES_; i++)
    {
        glDrawElements(GL_TRIANGLE_STRIP, twiceRes, GL_UNSIGNED_SHORT,
            reinterpret_cast<const GLvoid*>(i * twiceRes * sizeof(unsigned short)));
    }
    glDisableVertexAttribArray(tableVertexIndex_);
    tableProgram_.stop();

    if (logoPos.y() > -0.33 && logoPos.y() < 0.33)
    {
        glEnable(GL_DEPTH_TEST);
    }

    // Draw the paper lying on the table top
    paperProgram_.start();
    paperProgram_[projectionName_] = projection.getCurrent();
    paperProgram_[modelviewName_] = modelview.getCurrent();
    paperProgram_[lightPositionName_] = lightPos;
    paperProgram_[logoDirectionName_] = logoDirection;
    paperProgram_[curTimeName_] = currentTime;
    glVertexAttribPointer(paperVertexIndex_, 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.pvOffset));
    glEnableVertexAttribArray(paperVertexIndex_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(paperVertexIndex_);
    paperProgram_.stop();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_DEPTH_TEST);

    modelview.push();
    modelview.rotate(-18.4, 0.0, 1.0, 0.0);
    modelview.translate(-0.3, 0.0, -0.8);
    modelview.rotate(-90.0, 1.0, 0.0, 0.0);
    modelview.scale(0.015, 0.015, 0.015);

    // Draw the text on the paper lying on the table top.
    // Each character has its own array and element buffers, and they have
    // been initialized with the vertex attrib location for this program.
    textProgram_.start();
    textProgram_[projectionName_] = projection.getCurrent();
    textProgram_[modelviewName_] = modelview.getCurrent();
    textProgram_[curTimeName_] = currentTime;
    i_.draw();
    modelview.translate(3.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    d_.draw();
    modelview.translate(6.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    e_.draw();
    modelview.translate(5.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    a_.draw();
    modelview.translate(6.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    s_.draw();
    modelview.translate(10.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    i_.draw();
    modelview.translate(3.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    n_.draw();
    modelview.translate(-31.0, -13.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    m_.draw();
    modelview.translate(10.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    o_.draw();
    modelview.translate(5.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    t_.draw();
    modelview.translate(4.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    i_.draw();
    modelview.translate(3.5, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    o_.draw();
    modelview.translate(5.0, 0.0, 0.0);
    textProgram_[modelviewName_] = modelview.getCurrent();
    n_.draw();
    textProgram_.stop();

    modelview.pop();
}

void
Table::drawUnder(Stack4& modelview, Stack4& projection)
{
    glDisable(GL_DEPTH_TEST);

    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects_[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects_[1]);

    underProgram_.start();  
    underProgram_[modelviewName_] = modelview.getCurrent();
    underProgram_[projectionName_] = projection.getCurrent();
    glVertexAttribPointer(underVertexIndex_, 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<const GLvoid*>(dataMap_.tvOffset));
    glEnableVertexAttribArray(underVertexIndex_);
    static const unsigned int twiceRes(2 * (TABLERES_ + 1));
    for (unsigned int i = 0; i < TABLERES_; i++)
    {
        glDrawElements(GL_TRIANGLE_STRIP, twiceRes, GL_UNSIGNED_SHORT,
            reinterpret_cast<const GLvoid*>(i * twiceRes * sizeof(unsigned short)));
    }
    glDisableVertexAttribArray(underVertexIndex_);
    underProgram_.stop();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glEnable(GL_DEPTH_TEST); 
}
