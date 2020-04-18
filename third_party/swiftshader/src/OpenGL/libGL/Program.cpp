// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Program.cpp: Implements the Program class. Implements GL program objects
// and related functionality.

#include "Program.h"

#include "main.h"
#include "Shader.h"
#include "utilities.h"
#include "common/debug.h"
#include "Shader/PixelShader.hpp"
#include "Shader/VertexShader.hpp"

#include <string>
#include <stdlib.h>

namespace gl
{
	unsigned int Program::currentSerial = 1;

	std::string str(int i)
	{
		char buffer[20];
		sprintf(buffer, "%d", i);
		return buffer;
	}

	Uniform::Uniform(GLenum type, GLenum precision, const std::string &name, unsigned int arraySize) : type(type), precision(precision), name(name), arraySize(arraySize)
	{
		int bytes = UniformTypeSize(type) * size();
		data = new unsigned char[bytes];
		memset(data, 0, bytes);
		dirty = true;

		psRegisterIndex = -1;
		vsRegisterIndex = -1;
	}

	Uniform::~Uniform()
	{
		delete[] data;
	}

	bool Uniform::isArray() const
	{
		return arraySize >= 1;
	}

	int Uniform::size() const
	{
		return arraySize > 0 ? arraySize : 1;
	}

	int Uniform::registerCount() const
	{
		return size() * VariableRowCount(type);
	}

	UniformLocation::UniformLocation(const std::string &name, unsigned int element, unsigned int index) : name(name), element(element), index(index)
	{
	}

	Program::Program(ResourceManager *manager, GLuint handle) : serial(issueSerial()), resourceManager(manager), handle(handle)
	{
		device = getDevice();

		fragmentShader = 0;
		vertexShader = 0;
		pixelBinary = 0;
		vertexBinary = 0;

		infoLog = 0;
		validated = false;

		unlink();

		orphaned = false;
		referenceCount = 0;
	}

	Program::~Program()
	{
		unlink();

		if(vertexShader)
		{
			vertexShader->release();
		}

		if(fragmentShader)
		{
			fragmentShader->release();
		}
	}

	bool Program::attachShader(Shader *shader)
	{
		if(shader->getType() == GL_VERTEX_SHADER)
		{
			if(vertexShader)
			{
				return false;
			}

			vertexShader = (VertexShader*)shader;
			vertexShader->addRef();
		}
		else if(shader->getType() == GL_FRAGMENT_SHADER)
		{
			if(fragmentShader)
			{
				return false;
			}

			fragmentShader = (FragmentShader*)shader;
			fragmentShader->addRef();
		}
		else UNREACHABLE(shader->getType());

		return true;
	}

	bool Program::detachShader(Shader *shader)
	{
		if(shader->getType() == GL_VERTEX_SHADER)
		{
			if(vertexShader != shader)
			{
				return false;
			}

			vertexShader->release();
			vertexShader = 0;
		}
		else if(shader->getType() == GL_FRAGMENT_SHADER)
		{
			if(fragmentShader != shader)
			{
				return false;
			}

			fragmentShader->release();
			fragmentShader = 0;
		}
		else UNREACHABLE(shader->getType());

		return true;
	}

	int Program::getAttachedShadersCount() const
	{
		return (vertexShader ? 1 : 0) + (fragmentShader ? 1 : 0);
	}

	sw::PixelShader *Program::getPixelShader()
	{
		return pixelBinary;
	}

	sw::VertexShader *Program::getVertexShader()
	{
		return vertexBinary;
	}

	void Program::bindAttributeLocation(GLuint index, const char *name)
	{
		if(index < MAX_VERTEX_ATTRIBS)
		{
			for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
			{
				attributeBinding[i].erase(name);
			}

			attributeBinding[index].insert(name);
		}
	}

	GLuint Program::getAttributeLocation(const char *name)
	{
		if(name)
		{
			for(int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
			{
				if(linkedAttribute[index].name == std::string(name))
				{
					return index;
				}
			}
		}

		return -1;
	}

	int Program::getAttributeStream(int attributeIndex)
	{
		ASSERT(attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS);

		return attributeStream[attributeIndex];
	}

	// Returns the index of the texture image unit (0-19) corresponding to a sampler index (0-15 for the pixel shader and 0-3 for the vertex shader)
	GLint Program::getSamplerMapping(sw::SamplerType type, unsigned int samplerIndex)
	{
		GLuint logicalTextureUnit = -1;

		switch(type)
		{
		case sw::SAMPLER_PIXEL:
			ASSERT(samplerIndex < sizeof(samplersPS) / sizeof(samplersPS[0]));

			if(samplersPS[samplerIndex].active)
			{
				logicalTextureUnit = samplersPS[samplerIndex].logicalTextureUnit;
			}
			break;
		case sw::SAMPLER_VERTEX:
			ASSERT(samplerIndex < sizeof(samplersVS) / sizeof(samplersVS[0]));

			if(samplersVS[samplerIndex].active)
			{
				logicalTextureUnit = samplersVS[samplerIndex].logicalTextureUnit;
			}
			break;
		default: UNREACHABLE(type);
		}

		if(logicalTextureUnit < MAX_COMBINED_TEXTURE_IMAGE_UNITS)
		{
			return logicalTextureUnit;
		}

		return -1;
	}

	// Returns the texture type for a given sampler type and index (0-15 for the pixel shader and 0-3 for the vertex shader)
	TextureType Program::getSamplerTextureType(sw::SamplerType type, unsigned int samplerIndex)
	{
		switch(type)
		{
		case sw::SAMPLER_PIXEL:
			ASSERT(samplerIndex < sizeof(samplersPS)/sizeof(samplersPS[0]));
			ASSERT(samplersPS[samplerIndex].active);
			return samplersPS[samplerIndex].textureType;
		case sw::SAMPLER_VERTEX:
			ASSERT(samplerIndex < sizeof(samplersVS)/sizeof(samplersVS[0]));
			ASSERT(samplersVS[samplerIndex].active);
			return samplersVS[samplerIndex].textureType;
		default: UNREACHABLE(type);
		}

		return TEXTURE_2D;
	}

	GLint Program::getUniformLocation(std::string name)
	{
		int subscript = 0;

		// Strip any trailing array operator and retrieve the subscript
		size_t open = name.find_last_of('[');
		size_t close = name.find_last_of(']');
		if(open != std::string::npos && close == name.length() - 1)
		{
			subscript = atoi(name.substr(open + 1).c_str());
			name.erase(open);
		}

		unsigned int numUniforms = uniformIndex.size();
		for(unsigned int location = 0; location < numUniforms; location++)
		{
			if(uniformIndex[location].name == name &&
			   uniformIndex[location].element == subscript)
			{
				return location;
			}
		}

		return -1;
	}

	bool Program::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_FLOAT)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat),
				   v, sizeof(GLfloat) * count);
		}
		else if(targetUniform->type == GL_BOOL)
		{
			GLboolean *boolParams = (GLboolean*)targetUniform->data + uniformIndex[location].element;

			for(int i = 0; i < count; i++)
			{
				if(v[i] == 0.0f)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_FLOAT_VEC2)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * 2,
				   v, 2 * sizeof(GLfloat) * count);
		}
		else if(targetUniform->type == GL_BOOL_VEC2)
		{
			GLboolean *boolParams = (GLboolean*)targetUniform->data + uniformIndex[location].element * 2;

			for(int i = 0; i < count * 2; i++)
			{
				if(v[i] == 0.0f)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_FLOAT_VEC3)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * 3,
				   v, 3 * sizeof(GLfloat) * count);
		}
		else if(targetUniform->type == GL_BOOL_VEC3)
		{
			GLboolean *boolParams = (GLboolean*)targetUniform->data + uniformIndex[location].element * 3;

			for(int i = 0; i < count * 3; i++)
			{
				if(v[i] == 0.0f)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_FLOAT_VEC4)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * 4,
				   v, 4 * sizeof(GLfloat) * count);
		}
		else if(targetUniform->type == GL_BOOL_VEC4)
		{
			GLboolean *boolParams = (GLboolean*)targetUniform->data + uniformIndex[location].element * 4;

			for(int i = 0; i < count * 4; i++)
			{
				if(v[i] == 0.0f)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		if(targetUniform->type != GL_FLOAT_MAT2)
		{
			return false;
		}

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * 4,
			   value, 4 * sizeof(GLfloat) * count);

		return true;
	}

	bool Program::setUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		if(targetUniform->type != GL_FLOAT_MAT3)
		{
			return false;
		}

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * 9,
			   value, 9 * sizeof(GLfloat) * count);

		return true;
	}

	bool Program::setUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		if(targetUniform->type != GL_FLOAT_MAT4)
		{
			return false;
		}

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * 16,
			   value, 16 * sizeof(GLfloat) * count);

		return true;
	}

	bool Program::setUniform1iv(GLint location, GLsizei count, const GLint *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_INT ||
		   targetUniform->type == GL_SAMPLER_2D ||
		   targetUniform->type == GL_SAMPLER_CUBE)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLint),
				   v, sizeof(GLint) * count);
		}
		else if(targetUniform->type == GL_BOOL)
		{
			GLboolean *boolParams = new GLboolean[count];

			for(int i = 0; i < count; i++)
			{
				if(v[i] == 0)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}

			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLboolean),
				   boolParams, sizeof(GLboolean) * count);

			delete[] boolParams;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform2iv(GLint location, GLsizei count, const GLint *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_INT_VEC2)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLint) * 2,
				   v, 2 * sizeof(GLint) * count);
		}
		else if(targetUniform->type == GL_BOOL_VEC2)
		{
			GLboolean *boolParams = new GLboolean[count * 2];

			for(int i = 0; i < count * 2; i++)
			{
				if(v[i] == 0)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}

			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLboolean) * 2,
				   boolParams, 2 * sizeof(GLboolean) * count);

			delete[] boolParams;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform3iv(GLint location, GLsizei count, const GLint *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_INT_VEC3)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLint) * 3,
				   v, 3 * sizeof(GLint) * count);
		}
		else if(targetUniform->type == GL_BOOL_VEC3)
		{
			GLboolean *boolParams = new GLboolean[count * 3];

			for(int i = 0; i < count * 3; i++)
			{
				if(v[i] == 0)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}

			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLboolean) * 3,
				   boolParams, 3 * sizeof(GLboolean) * count);

			delete[] boolParams;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform4iv(GLint location, GLsizei count, const GLint *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		if(targetUniform->type == GL_INT_VEC4)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLint) * 4,
				   v, 4 * sizeof(GLint) * count);
		}
		else if(targetUniform->type == GL_BOOL_VEC4)
		{
			GLboolean *boolParams = new GLboolean[count * 4];

			for(int i = 0; i < count * 4; i++)
			{
				if(v[i] == 0)
				{
					boolParams[i] = GL_FALSE;
				}
				else
				{
					boolParams[i] = GL_TRUE;
				}
			}

			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLboolean) * 4,
				   boolParams, 4 * sizeof(GLboolean) * count);

			delete[] boolParams;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		unsigned int count = UniformComponentCount(targetUniform->type);

		// Sized query - ensure the provided buffer is large enough
		if(bufSize && static_cast<unsigned int>(*bufSize) < count * sizeof(GLfloat))
		{
			return false;
		}

		switch(UniformComponentType(targetUniform->type))
		{
		case GL_BOOL:
			{
				GLboolean *boolParams = (GLboolean*)targetUniform->data + uniformIndex[location].element * count;

				for(unsigned int i = 0; i < count; i++)
				{
					params[i] = (boolParams[i] == GL_FALSE) ? 0.0f : 1.0f;
				}
			}
			break;
		case GL_FLOAT:
			memcpy(params, targetUniform->data + uniformIndex[location].element * count * sizeof(GLfloat),
				   count * sizeof(GLfloat));
			break;
		case GL_INT:
			{
				GLint *intParams = (GLint*)targetUniform->data + uniformIndex[location].element * count;

				for(unsigned int i = 0; i < count; i++)
				{
					params[i] = (float)intParams[i];
				}
			}
			break;
		default: UNREACHABLE(targetUniform->type);
		}

		return true;
	}

	bool Program::getUniformiv(GLint location, GLsizei *bufSize, GLint *params)
	{
		if(location < 0 || location >= (int)uniformIndex.size())
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		unsigned int count = UniformComponentCount(targetUniform->type);

		// Sized query - ensure the provided buffer is large enough
		if(bufSize && static_cast<unsigned int>(*bufSize) < count *sizeof(GLint))
		{
			return false;
		}

		switch(UniformComponentType(targetUniform->type))
		{
		case GL_BOOL:
			{
				GLboolean *boolParams = targetUniform->data + uniformIndex[location].element * count;

				for(unsigned int i = 0; i < count; i++)
				{
					params[i] = (GLint)boolParams[i];
				}
			}
			break;
		case GL_FLOAT:
			{
				GLfloat *floatParams = (GLfloat*)targetUniform->data + uniformIndex[location].element * count;

				for(unsigned int i = 0; i < count; i++)
				{
					params[i] = (GLint)floatParams[i];
				}
			}
			break;
		case GL_INT:
			memcpy(params, targetUniform->data + uniformIndex[location].element * count * sizeof(GLint),
				   count * sizeof(GLint));
			break;
		default: UNREACHABLE(targetUniform->type);
		}

		return true;
	}

	void Program::dirtyAllUniforms()
	{
		unsigned int numUniforms = uniforms.size();
		for(unsigned int index = 0; index < numUniforms; index++)
		{
			uniforms[index]->dirty = true;
		}
	}

	// Applies all the uniforms set for this program object to the device
	void Program::applyUniforms()
	{
		unsigned int numUniforms = uniformIndex.size();
		for(unsigned int location = 0; location < numUniforms; location++)
		{
			if(uniformIndex[location].element != 0)
			{
				continue;
			}

			Uniform *targetUniform = uniforms[uniformIndex[location].index];

			if(targetUniform->dirty)
			{
				int size = targetUniform->size();
				GLfloat *f = (GLfloat*)targetUniform->data;
				GLint *i = (GLint*)targetUniform->data;
				GLboolean *b = (GLboolean*)targetUniform->data;

				switch(targetUniform->type)
				{
				case GL_BOOL:       applyUniform1bv(location, size, b);       break;
				case GL_BOOL_VEC2:  applyUniform2bv(location, size, b);       break;
				case GL_BOOL_VEC3:  applyUniform3bv(location, size, b);       break;
				case GL_BOOL_VEC4:  applyUniform4bv(location, size, b);       break;
				case GL_FLOAT:      applyUniform1fv(location, size, f);       break;
				case GL_FLOAT_VEC2: applyUniform2fv(location, size, f);       break;
				case GL_FLOAT_VEC3: applyUniform3fv(location, size, f);       break;
				case GL_FLOAT_VEC4: applyUniform4fv(location, size, f);       break;
				case GL_FLOAT_MAT2: applyUniformMatrix2fv(location, size, f); break;
				case GL_FLOAT_MAT3: applyUniformMatrix3fv(location, size, f); break;
				case GL_FLOAT_MAT4: applyUniformMatrix4fv(location, size, f); break;
				case GL_SAMPLER_2D:
				case GL_SAMPLER_CUBE:
				case GL_INT:        applyUniform1iv(location, size, i);       break;
				case GL_INT_VEC2:   applyUniform2iv(location, size, i);       break;
				case GL_INT_VEC3:   applyUniform3iv(location, size, i);       break;
				case GL_INT_VEC4:   applyUniform4iv(location, size, i);       break;
				default:
					UNREACHABLE(targetUniform->type);
				}

				targetUniform->dirty = false;
			}
		}
	}

	// Packs varyings into generic varying registers.
	// Returns the number of used varying registers, or -1 if unsuccesful
	int Program::packVaryings(const glsl::Varying *packing[][4])
	{
		for(glsl::VaryingList::iterator varying = fragmentShader->varyings.begin(); varying != fragmentShader->varyings.end(); varying++)
		{
			int n = VariableRowCount(varying->type) * varying->size();
			int m = VariableColumnCount(varying->type);
			bool success = false;

			if(m == 2 || m == 3 || m == 4)
			{
				for(int r = 0; r <= MAX_VARYING_VECTORS - n && !success; r++)
				{
					bool available = true;

					for(int y = 0; y < n && available; y++)
					{
						for(int x = 0; x < m && available; x++)
						{
							if(packing[r + y][x])
							{
								available = false;
							}
						}
					}

					if(available)
					{
						varying->registerIndex = r;
						varying->column = 0;

						for(int y = 0; y < n; y++)
						{
							for(int x = 0; x < m; x++)
							{
								packing[r + y][x] = &*varying;
							}
						}

						success = true;
					}
				}

				if(!success && m == 2)
				{
					for(int r = MAX_VARYING_VECTORS - n; r >= 0 && !success; r--)
					{
						bool available = true;

						for(int y = 0; y < n && available; y++)
						{
							for(int x = 2; x < 4 && available; x++)
							{
								if(packing[r + y][x])
								{
									available = false;
								}
							}
						}

						if(available)
						{
							varying->registerIndex = r;
							varying->column = 2;

							for(int y = 0; y < n; y++)
							{
								for(int x = 2; x < 4; x++)
								{
									packing[r + y][x] = &*varying;
								}
							}

							success = true;
						}
					}
				}
			}
			else if(m == 1)
			{
				int space[4] = {0};

				for(int y = 0; y < MAX_VARYING_VECTORS; y++)
				{
					for(int x = 0; x < 4; x++)
					{
						space[x] += packing[y][x] ? 0 : 1;
					}
				}

				int column = 0;

				for(int x = 0; x < 4; x++)
				{
					if(space[x] >= n && space[x] < space[column])
					{
						column = x;
					}
				}

				if(space[column] >= n)
				{
					for(int r = 0; r < MAX_VARYING_VECTORS; r++)
					{
						if(!packing[r][column])
						{
							varying->registerIndex = r;

							for(int y = r; y < r + n; y++)
							{
								packing[y][column] = &*varying;
							}

							break;
						}
					}

					varying->column = column;

					success = true;
				}
			}
			else UNREACHABLE(m);

			if(!success)
			{
				appendToInfoLog("Could not pack varying %s", varying->name.c_str());

				return -1;
			}
		}

		// Return the number of used registers
		int registers = 0;

		for(int r = 0; r < MAX_VARYING_VECTORS; r++)
		{
			if(packing[r][0] || packing[r][1] || packing[r][2] || packing[r][3])
			{
				registers++;
			}
		}

		return registers;
	}

	bool Program::linkVaryings()
	{
		for(glsl::VaryingList::iterator input = fragmentShader->varyings.begin(); input != fragmentShader->varyings.end(); input++)
		{
			bool matched = false;

			for(glsl::VaryingList::iterator output = vertexShader->varyings.begin(); output != vertexShader->varyings.end(); output++)
			{
				if(output->name == input->name)
				{
					if(output->type != input->type || output->size() != input->size())
					{
						appendToInfoLog("Type of vertex varying %s does not match that of the fragment varying", output->name.c_str());

						return false;
					}

					matched = true;
					break;
				}
			}

			if(!matched)
			{
				appendToInfoLog("Fragment varying %s does not match any vertex varying", input->name.c_str());

				return false;
			}
		}

		glsl::VaryingList &psVaryings = fragmentShader->varyings;
		glsl::VaryingList &vsVaryings = vertexShader->varyings;

		for(glsl::VaryingList::iterator output = vsVaryings.begin(); output != vsVaryings.end(); output++)
		{
			for(glsl::VaryingList::iterator input = psVaryings.begin(); input != psVaryings.end(); input++)
			{
				if(output->name == input->name)
				{
					int in = input->registerIndex;
					int out = output->registerIndex;
					int components = VariableColumnCount(output->type);
					int registers = VariableRowCount(output->type) * output->size();

					ASSERT(in >= 0);

					if(in + registers > MAX_VARYING_VECTORS)
					{
						appendToInfoLog("Too many varyings");
						return false;
					}

					if(out >= 0)
					{
						if(out + registers > MAX_VARYING_VECTORS)
						{
							appendToInfoLog("Too many varyings");
							return false;
						}

						for(int i = 0; i < registers; i++)
						{
							vertexBinary->setOutput(out + i, components, sw::Shader::Semantic(sw::Shader::USAGE_COLOR, in + i));
						}
					}
					else   // Vertex varying is declared but not written to
					{
						for(int i = 0; i < registers; i++)
						{
							pixelBinary->setInput(in + i, components, sw::Shader::Semantic());
						}
					}

					break;
				}
			}
		}

		return true;
	}

	// Links the code of the vertex and pixel shader by matching up their varyings,
	// compiling them into binaries, determining the attribute mappings, and collecting
	// a list of uniforms
	void Program::link()
	{
		unlink();

		if(!fragmentShader || !fragmentShader->isCompiled())
		{
			return;
		}

		if(!vertexShader || !vertexShader->isCompiled())
		{
			return;
		}

		vertexBinary = new sw::VertexShader(vertexShader->getVertexShader());
		pixelBinary = new sw::PixelShader(fragmentShader->getPixelShader());

		if(!linkVaryings())
		{
			return;
		}

		if(!linkAttributes())
		{
			return;
		}

		if(!linkUniforms(fragmentShader))
		{
			return;
		}

		if(!linkUniforms(vertexShader))
		{
			return;
		}

		linked = true;   // Success
	}

	// Determines the mapping between GL attributes and vertex stream usage indices
	bool Program::linkAttributes()
	{
		unsigned int usedLocations = 0;

		// Link attributes that have a binding location
		for(glsl::ActiveAttributes::iterator attribute = vertexShader->activeAttributes.begin(); attribute != vertexShader->activeAttributes.end(); attribute++)
		{
			int location = getAttributeBinding(attribute->name);

			if(location != -1)   // Set by glBindAttribLocation
			{
				if(!linkedAttribute[location].name.empty())
				{
					// Multiple active attributes bound to the same location; not an error
				}

				linkedAttribute[location] = *attribute;

				int rows = VariableRowCount(attribute->type);

				if(rows + location > MAX_VERTEX_ATTRIBS)
				{
					appendToInfoLog("Active attribute (%s) at location %d is too big to fit", attribute->name.c_str(), location);
					return false;
				}

				for(int i = 0; i < rows; i++)
				{
					usedLocations |= 1 << (location + i);
				}
			}
		}

		// Link attributes that don't have a binding location
		for(glsl::ActiveAttributes::iterator attribute = vertexShader->activeAttributes.begin(); attribute != vertexShader->activeAttributes.end(); attribute++)
		{
			int location = getAttributeBinding(attribute->name);

			if(location == -1)   // Not set by glBindAttribLocation
			{
				int rows = VariableRowCount(attribute->type);
				int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, MAX_VERTEX_ATTRIBS);

				if(availableIndex == -1 || availableIndex + rows > MAX_VERTEX_ATTRIBS)
				{
					appendToInfoLog("Too many active attributes (%s)", attribute->name.c_str());
					return false;   // Fail to link
				}

				linkedAttribute[availableIndex] = *attribute;
			}
		}

		for(int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; )
		{
			int index = vertexShader->getSemanticIndex(linkedAttribute[attributeIndex].name);
			int rows = std::max(VariableRowCount(linkedAttribute[attributeIndex].type), 1);

			for(int r = 0; r < rows; r++)
			{
				attributeStream[attributeIndex++] = index++;
			}
		}

		return true;
	}

	int Program::getAttributeBinding(const std::string &name)
	{
		for(int location = 0; location < MAX_VERTEX_ATTRIBS; location++)
		{
			if(attributeBinding[location].find(name) != attributeBinding[location].end())
			{
				return location;
			}
		}

		return -1;
	}

	bool Program::linkUniforms(Shader *shader)
	{
		const glsl::ActiveUniforms &activeUniforms = shader->activeUniforms;

		for(unsigned int uniformIndex = 0; uniformIndex < activeUniforms.size(); uniformIndex++)
		{
			const glsl::Uniform &uniform = activeUniforms[uniformIndex];

			if(!defineUniform(shader->getType(), uniform.type, uniform.precision, uniform.name, uniform.arraySize, uniform.registerIndex))
			{
				return false;
			}
		}

		return true;
	}

	bool Program::defineUniform(GLenum shader, GLenum type, GLenum precision, const std::string &name, unsigned int arraySize, int registerIndex)
	{
		if(type == GL_SAMPLER_2D || type == GL_SAMPLER_CUBE)
		{
			int index = registerIndex;

			do
			{
				if(shader == GL_VERTEX_SHADER)
				{
					if(index < MAX_VERTEX_TEXTURE_IMAGE_UNITS)
					{
						samplersVS[index].active = true;
						samplersVS[index].textureType = (type == GL_SAMPLER_CUBE) ? TEXTURE_CUBE : TEXTURE_2D;
						samplersVS[index].logicalTextureUnit = 0;
					}
					else
					{
					   appendToInfoLog("Vertex shader sampler count exceeds MAX_VERTEX_TEXTURE_IMAGE_UNITS (%d).", MAX_VERTEX_TEXTURE_IMAGE_UNITS);
					   return false;
					}
				}
				else if(shader == GL_FRAGMENT_SHADER)
				{
					if(index < MAX_TEXTURE_IMAGE_UNITS)
					{
						samplersPS[index].active = true;
						samplersPS[index].textureType = (type == GL_SAMPLER_CUBE) ? TEXTURE_CUBE : TEXTURE_2D;
						samplersPS[index].logicalTextureUnit = 0;
					}
					else
					{
						appendToInfoLog("Pixel shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS (%d).", MAX_TEXTURE_IMAGE_UNITS);
						return false;
					}
				}
				else UNREACHABLE(shader);

				index++;
			}
			while(index < registerIndex + static_cast<int>(arraySize));
		}

		Uniform *uniform = 0;
		GLint location = getUniformLocation(name);

		if(location >= 0)   // Previously defined, types must match
		{
			uniform = uniforms[uniformIndex[location].index];

			if(uniform->type != type)
			{
				appendToInfoLog("Types for uniform %s do not match between the vertex and fragment shader", uniform->name.c_str());
				return false;
			}

			if(uniform->precision != precision)
			{
				appendToInfoLog("Precisions for uniform %s do not match between the vertex and fragment shader", uniform->name.c_str());
				return false;
			}
		}
		else
		{
			uniform = new Uniform(type, precision, name, arraySize);
		}

		if(!uniform)
		{
			return false;
		}

		if(shader == GL_VERTEX_SHADER)
		{
			uniform->vsRegisterIndex = registerIndex;
		}
		else if(shader == GL_FRAGMENT_SHADER)
		{
			uniform->psRegisterIndex = registerIndex;
		}
		else UNREACHABLE(shader);

		if(location == -1)   // Not previously defined
		{
			uniforms.push_back(uniform);
			unsigned int index = uniforms.size() - 1;

			for(int i = 0; i < uniform->size(); i++)
			{
				uniformIndex.push_back(UniformLocation(name, i, index));
			}
		}

		if(shader == GL_VERTEX_SHADER)
		{
			if(registerIndex + uniform->registerCount() > MAX_VERTEX_UNIFORM_VECTORS)
			{
				appendToInfoLog("Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS (%d)", MAX_VERTEX_UNIFORM_VECTORS);
				return false;
			}
		}
		else if(shader == GL_FRAGMENT_SHADER)
		{
			if(registerIndex + uniform->registerCount() > MAX_FRAGMENT_UNIFORM_VECTORS)
			{
				appendToInfoLog("Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS (%d)", MAX_FRAGMENT_UNIFORM_VECTORS);
				return false;
			}
		}
		else UNREACHABLE(shader);

		return true;
	}

	bool Program::applyUniform1bv(GLint location, GLsizei count, const GLboolean *v)
	{
		int vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (v[0] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][1] = 0;
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 1;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform2bv(GLint location, GLsizei count, const GLboolean *v)
	{
		int vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (v[0] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][1] = (v[1] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 2;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform3bv(GLint location, GLsizei count, const GLboolean *v)
	{
		int vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (v[0] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][1] = (v[1] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][2] = (v[2] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][3] = 0;

			v += 3;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform4bv(GLint location, GLsizei count, const GLboolean *v)
	{
		int vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (v[0] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][1] = (v[1] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][2] = (v[2] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);
			vector[i][3] = (v[3] == GL_FALSE ? 0x00000000 : 0xFFFFFFFF);

			v += 4;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform1fv(GLint location, GLsizei count, const GLfloat *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = 0;
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 1;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform2fv(GLint location, GLsizei count, const GLfloat *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 2;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform3fv(GLint location, GLsizei count, const GLfloat *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = v[2];
			vector[i][3] = 0;

			v += 3;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform4fv(GLint location, GLsizei count, const GLfloat *v)
	{
		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)v, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)v, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 1) / 2][2][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = 0; matrix[i][0][3] = 0;
			matrix[i][1][0] = value[2];	matrix[i][1][1] = value[3];	matrix[i][1][2] = 0; matrix[i][1][3] = 0;

			value += 4;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)matrix, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)matrix, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 2) / 3][3][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = value[2];	matrix[i][0][3] = 0;
			matrix[i][1][0] = value[3];	matrix[i][1][1] = value[4];	matrix[i][1][2] = value[5];	matrix[i][1][3] = 0;
			matrix[i][2][0] = value[6];	matrix[i][2][1] = value[7];	matrix[i][2][2] = value[8];	matrix[i][2][3] = 0;

			value += 9;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)matrix, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)matrix, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value)
	{
		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)value, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)value, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform1iv(GLint location, GLsizei count, const GLint *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (float)v[i];
			vector[i][1] = 0;
			vector[i][2] = 0;
			vector[i][3] = 0;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			if(targetUniform->type == GL_SAMPLER_2D ||
			   targetUniform->type == GL_SAMPLER_CUBE)
			{
				for(int i = 0; i < count; i++)
				{
					unsigned int samplerIndex = targetUniform->psRegisterIndex + i;

					if(samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
					{
						ASSERT(samplersPS[samplerIndex].active);
						samplersPS[samplerIndex].logicalTextureUnit = v[i];
					}
				}
			}
			else
			{
				device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
			}
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			if(targetUniform->type == GL_SAMPLER_2D ||
			   targetUniform->type == GL_SAMPLER_CUBE)
			{
				for(int i = 0; i < count; i++)
				{
					unsigned int samplerIndex = targetUniform->vsRegisterIndex + i;

					if(samplerIndex < MAX_VERTEX_TEXTURE_IMAGE_UNITS)
					{
						ASSERT(samplersVS[samplerIndex].active);
						samplersVS[samplerIndex].logicalTextureUnit = v[i];
					}
				}
			}
			else
			{
				device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
			}
		}

		return true;
	}

	bool Program::applyUniform2iv(GLint location, GLsizei count, const GLint *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (float)v[0];
			vector[i][1] = (float)v[1];
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 2;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform3iv(GLint location, GLsizei count, const GLint *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (float)v[0];
			vector[i][1] = (float)v[1];
			vector[i][2] = (float)v[2];
			vector[i][3] = 0;

			v += 3;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform4iv(GLint location, GLsizei count, const GLint *v)
	{
		float vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = (float)v[0];
			vector[i][1] = (float)v[1];
			vector[i][2] = (float)v[2];
			vector[i][3] = (float)v[3];

			v += 4;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, (float*)vector, targetUniform->registerCount());
		}

		return true;
	}

	void Program::appendToInfoLog(const char *format, ...)
	{
		if(!format)
		{
			return;
		}

		char info[1024];

		va_list vararg;
		va_start(vararg, format);
		vsnprintf(info, sizeof(info), format, vararg);
		va_end(vararg);

		size_t infoLength = strlen(info);

		if(!infoLog)
		{
			infoLog = new char[infoLength + 2];
			strcpy(infoLog, info);
			strcpy(infoLog + infoLength, "\n");
		}
		else
		{
			size_t logLength = strlen(infoLog);
			char *newLog = new char[logLength + infoLength + 2];
			strcpy(newLog, infoLog);
			strcpy(newLog + logLength, info);
			strcpy(newLog + logLength + infoLength, "\n");

			delete[] infoLog;
			infoLog = newLog;
		}
	}

	void Program::resetInfoLog()
	{
		if(infoLog)
		{
			delete[] infoLog;
			infoLog = 0;
		}
	}

	// Returns the program object to an unlinked state, before re-linking, or at destruction
	void Program::unlink()
	{
		delete vertexBinary;
		vertexBinary = 0;
		delete pixelBinary;
		pixelBinary = 0;

		for(int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
		{
			linkedAttribute[index].name.clear();
			attributeStream[index] = -1;
		}

		for(int index = 0; index < MAX_TEXTURE_IMAGE_UNITS; index++)
		{
			samplersPS[index].active = false;
		}

		for(int index = 0; index < MAX_VERTEX_TEXTURE_IMAGE_UNITS; index++)
		{
			samplersVS[index].active = false;
		}

		while(!uniforms.empty())
		{
			delete uniforms.back();
			uniforms.pop_back();
		}

		uniformIndex.clear();

		delete[] infoLog;
		infoLog = 0;

		linked = false;
	}

	bool Program::isLinked()
	{
		return linked;
	}

	bool Program::isValidated() const
	{
		return validated;
	}

	void Program::release()
	{
		referenceCount--;

		if(referenceCount == 0 && orphaned)
		{
			resourceManager->deleteProgram(handle);
		}
	}

	void Program::addRef()
	{
		referenceCount++;
	}

	unsigned int Program::getRefCount() const
	{
		return referenceCount;
	}

	unsigned int Program::getSerial() const
	{
		return serial;
	}

	unsigned int Program::issueSerial()
	{
		return currentSerial++;
	}

	int Program::getInfoLogLength() const
	{
		if(!infoLog)
		{
			return 0;
		}
		else
		{
		   return strlen(infoLog) + 1;
		}
	}

	void Program::getInfoLog(GLsizei bufSize, GLsizei *length, char *buffer)
	{
		int index = 0;

		if(bufSize > 0)
		{
			if(infoLog)
			{
				index = std::min(bufSize - 1, (int)strlen(infoLog));
				memcpy(buffer, infoLog, index);
			}

			buffer[index] = '\0';
		}

		if(length)
		{
			*length = index;
		}
	}

	void Program::getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders)
	{
		int total = 0;

		if(vertexShader)
		{
			if(total < maxCount)
			{
				shaders[total] = vertexShader->getName();
			}

			total++;
		}

		if(fragmentShader)
		{
			if(total < maxCount)
			{
				shaders[total] = fragmentShader->getName();
			}

			total++;
		}

		if(count)
		{
			*count = total;
		}
	}

	void Program::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
	{
		// Skip over inactive attributes
		unsigned int activeAttribute = 0;
		unsigned int attribute;
		for(attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
		{
			if(linkedAttribute[attribute].name.empty())
			{
				continue;
			}

			if(activeAttribute == index)
			{
				break;
			}

			activeAttribute++;
		}

		if(bufsize > 0)
		{
			const char *string = linkedAttribute[attribute].name.c_str();

			strncpy(name, string, bufsize);
			name[bufsize - 1] = '\0';

			if(length)
			{
				*length = strlen(name);
			}
		}

		*size = 1;   // Always a single 'type' instance

		*type = linkedAttribute[attribute].type;
	}

	size_t Program::getActiveAttributeCount() const
	{
		size_t count = 0;

		for(size_t attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; ++attributeIndex)
		{
			if(!linkedAttribute[attributeIndex].name.empty())
			{
				count++;
			}
		}

		return count;
	}

	GLint Program::getActiveAttributeMaxLength() const
	{
		int maxLength = 0;

		for(int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
		{
			if(!linkedAttribute[attributeIndex].name.empty())
			{
				maxLength = std::max((int)(linkedAttribute[attributeIndex].name.length() + 1), maxLength);
			}
		}

		return maxLength;
	}

	void Program::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
	{
		if(bufsize > 0)
		{
			std::string string = uniforms[index]->name;

			if(uniforms[index]->isArray())
			{
				string += "[0]";
			}

			strncpy(name, string.c_str(), bufsize);
			name[bufsize - 1] = '\0';

			if(length)
			{
				*length = strlen(name);
			}
		}

		*size = uniforms[index]->size();

		*type = uniforms[index]->type;
	}

	size_t Program::getActiveUniformCount() const
	{
		return uniforms.size();
	}

	GLint Program::getActiveUniformMaxLength() const
	{
		int maxLength = 0;

		unsigned int numUniforms = uniforms.size();
		for(unsigned int uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
		{
			if(!uniforms[uniformIndex]->name.empty())
			{
				int length = (int)(uniforms[uniformIndex]->name.length() + 1);
				if(uniforms[uniformIndex]->isArray())
				{
					length += 3;  // Counting in "[0]".
				}
				maxLength = std::max(length, maxLength);
			}
		}

		return maxLength;
	}

	void Program::flagForDeletion()
	{
		orphaned = true;
	}

	bool Program::isFlaggedForDeletion() const
	{
		return orphaned;
	}

	void Program::validate()
	{
		resetInfoLog();

		if(!isLinked())
		{
			appendToInfoLog("Program has not been successfully linked.");
			validated = false;
		}
		else
		{
			applyUniforms();
			if(!validateSamplers(true))
			{
				validated = false;
			}
			else
			{
				validated = true;
			}
		}
	}

	bool Program::validateSamplers(bool logErrors)
	{
		// if any two active samplers in a program are of different types, but refer to the same
		// texture image unit, and this is the current program, then ValidateProgram will fail, and
		// DrawArrays and DrawElements will issue the INVALID_OPERATION error.

		TextureType textureUnitType[MAX_COMBINED_TEXTURE_IMAGE_UNITS];

		for(unsigned int i = 0; i < MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++)
		{
			textureUnitType[i] = TEXTURE_UNKNOWN;
		}

		for(unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
		{
			if(samplersPS[i].active)
			{
				unsigned int unit = samplersPS[i].logicalTextureUnit;

				if(unit >= MAX_COMBINED_TEXTURE_IMAGE_UNITS)
				{
					if(logErrors)
					{
						appendToInfoLog("Sampler uniform (%d) exceeds MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, MAX_COMBINED_TEXTURE_IMAGE_UNITS);
					}

					return false;
				}

				if(textureUnitType[unit] != TEXTURE_UNKNOWN)
				{
					if(samplersPS[i].textureType != textureUnitType[unit])
					{
						if(logErrors)
						{
							appendToInfoLog("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
						}

						return false;
					}
				}
				else
				{
					textureUnitType[unit] = samplersPS[i].textureType;
				}
			}
		}

		for(unsigned int i = 0; i < MAX_VERTEX_TEXTURE_IMAGE_UNITS; i++)
		{
			if(samplersVS[i].active)
			{
				unsigned int unit = samplersVS[i].logicalTextureUnit;

				if(unit >= MAX_COMBINED_TEXTURE_IMAGE_UNITS)
				{
					if(logErrors)
					{
						appendToInfoLog("Sampler uniform (%d) exceeds MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, MAX_COMBINED_TEXTURE_IMAGE_UNITS);
					}

					return false;
				}

				if(textureUnitType[unit] != TEXTURE_UNKNOWN)
				{
					if(samplersVS[i].textureType != textureUnitType[unit])
					{
						if(logErrors)
						{
							appendToInfoLog("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
						}

						return false;
					}
				}
				else
				{
					textureUnitType[unit] = samplersVS[i].textureType;
				}
			}
		}

		return true;
	}
}
