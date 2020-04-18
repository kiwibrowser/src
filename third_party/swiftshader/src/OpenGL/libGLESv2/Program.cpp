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
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "Program.h"

#include "main.h"
#include "Buffer.h"
#include "Shader.h"
#include "TransformFeedback.h"
#include "utilities.h"
#include "common/debug.h"
#include "Shader/PixelShader.hpp"
#include "Shader/VertexShader.hpp"

#include <algorithm>
#include <string>
#include <stdlib.h>

namespace es2
{
	unsigned int Program::currentSerial = 1;

	std::string str(int i)
	{
		char buffer[20];
		sprintf(buffer, "%d", i);
		return buffer;
	}

	Uniform::BlockInfo::BlockInfo(const glsl::Uniform& uniform, int blockIndex)
	{
		if(blockIndex >= 0)
		{
			index = blockIndex;
			offset = uniform.blockInfo.offset;
			arrayStride = uniform.blockInfo.arrayStride;
			matrixStride = uniform.blockInfo.matrixStride;
			isRowMajorMatrix = uniform.blockInfo.isRowMajorMatrix;
		}
		else
		{
			index = -1;
			offset = -1;
			arrayStride = -1;
			matrixStride = -1;
			isRowMajorMatrix = false;
		}
	}

	Uniform::Uniform(const glsl::Uniform &uniform, const BlockInfo &blockInfo)
	 : type(uniform.type), precision(uniform.precision), name(uniform.name),
	   arraySize(uniform.arraySize), blockInfo(blockInfo), fields(uniform.fields)
	{
		if((blockInfo.index == -1) && uniform.fields.empty())
		{
			size_t bytes = UniformTypeSize(type) * size();
			data = new unsigned char[bytes];
			memset(data, 0, bytes);
		}
		else
		{
			data = nullptr;
		}
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
		return size() * VariableRegisterCount(type);
	}

	UniformBlock::UniformBlock(const std::string &name, unsigned int elementIndex, unsigned int dataSize, std::vector<unsigned int> memberUniformIndexes) :
		name(name), elementIndex(elementIndex), dataSize(dataSize), memberUniformIndexes(memberUniformIndexes), psRegisterIndex(GL_INVALID_INDEX), vsRegisterIndex(GL_INVALID_INDEX)
	{
	}

	void UniformBlock::setRegisterIndex(GLenum shader, unsigned int registerIndex)
	{
		switch(shader)
		{
		case GL_VERTEX_SHADER:
			vsRegisterIndex = registerIndex;
			break;
		case GL_FRAGMENT_SHADER:
			psRegisterIndex = registerIndex;
			break;
		default:
			UNREACHABLE(shader);
		}
	}

	bool UniformBlock::isArrayElement() const
	{
		return elementIndex != GL_INVALID_INDEX;
	}

	bool UniformBlock::isReferencedByVertexShader() const
	{
		return vsRegisterIndex != GL_INVALID_INDEX;
	}

	bool UniformBlock::isReferencedByFragmentShader() const
	{
		return psRegisterIndex != GL_INVALID_INDEX;
	}

	UniformLocation::UniformLocation(const std::string &name, unsigned int element, unsigned int index) : name(name), element(element), index(index)
	{
	}

	LinkedVarying::LinkedVarying()
	{
	}

	LinkedVarying::LinkedVarying(const std::string &name, GLenum type, GLsizei size, int reg, int col)
	 : name(name), type(type), size(size), reg(reg), col(col)
	{
	}

	Program::Program(ResourceManager *manager, GLuint handle) : serial(issueSerial()), resourceManager(manager), handle(handle)
	{
		fragmentShader = 0;
		vertexShader = 0;
		pixelBinary = 0;
		vertexBinary = 0;

		transformFeedbackBufferMode = GL_INTERLEAVED_ATTRIBS;
		totalLinkedVaryingsComponents = 0;

		infoLog = 0;
		validated = false;

		resetUniformBlockBindings();
		unlink();

		orphaned = false;
		retrievableBinary = false;
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

	GLint Program::getFragDataLocation(const GLchar *name)
	{
		if(name && linked)
		{
			std::string baseName(name);
			unsigned int subscript = GL_INVALID_INDEX;
			baseName = ParseUniformName(baseName, &subscript);
			for(auto const &varying : fragmentShader->varyings)
			{
				if(varying.qualifier == EvqFragmentOut)
				{
					if(varying.name == baseName)
					{
						ASSERT(varying.registerIndex >= 0);

						if(subscript == GL_INVALID_INDEX)   // No subscript
						{
							return varying.registerIndex;
						}

						int rowCount = VariableRowCount(varying.type);
						int colCount = VariableColumnCount(varying.type);

						return varying.registerIndex + (rowCount > 1 ? colCount * subscript : subscript);
					}
				}
			}
		}

		return -1;
	}

	void Program::bindAttributeLocation(GLuint index, const char *name)
	{
		attributeBinding[name] = index;
	}

	GLint Program::getAttributeLocation(const char *name)
	{
		if(name)
		{
			std::string strName(name);
			for(auto const &it : linkedAttribute)
			{
				if(it.name == strName)
				{
					return getAttributeBinding(it);
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
		GLint logicalTextureUnit = -1;

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

	bool Program::isUniformDefined(const std::string &name) const
	{
		unsigned int subscript = GL_INVALID_INDEX;
		std::string baseName = es2::ParseUniformName(name, &subscript);

		size_t numUniforms = uniformIndex.size();
		for(size_t location = 0; location < numUniforms; location++)
		{
			const unsigned int index = uniformIndex[location].index;
			if((uniformIndex[location].name == baseName) && ((index == GL_INVALID_INDEX) ||
			   ((uniforms[index]->isArray() && uniformIndex[location].element == subscript) ||
			    (subscript == GL_INVALID_INDEX))))
			{
				return true;
			}
		}

		return false;
	}

	GLint Program::getUniformLocation(const std::string &name) const
	{
		unsigned int subscript = GL_INVALID_INDEX;
		std::string baseName = es2::ParseUniformName(name, &subscript);

		size_t numUniforms = uniformIndex.size();
		for(size_t location = 0; location < numUniforms; location++)
		{
			const unsigned int index = uniformIndex[location].index;
			if((index != GL_INVALID_INDEX) && (uniformIndex[location].name == baseName) &&
			   ((uniforms[index]->isArray() && uniformIndex[location].element == subscript) ||
			    (subscript == GL_INVALID_INDEX)))
			{
				return (GLint)location;
			}
		}

		return -1;
	}

	GLuint Program::getUniformIndex(const std::string &name) const
	{
		unsigned int subscript = GL_INVALID_INDEX;
		std::string baseName = es2::ParseUniformName(name, &subscript);

		// The app is not allowed to specify array indices other than 0 for arrays of basic types
		if(subscript != 0 && subscript != GL_INVALID_INDEX)
		{
			return GL_INVALID_INDEX;
		}

		size_t numUniforms = uniforms.size();
		for(GLuint index = 0; index < numUniforms; index++)
		{
			if(uniforms[index]->name == baseName)
			{
				if(uniforms[index]->isArray() || subscript == GL_INVALID_INDEX)
				{
					return index;
				}
			}
		}

		return GL_INVALID_INDEX;
	}

	void Program::getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const
	{
		ASSERT(uniformBlockIndex < getActiveUniformBlockCount());

		const UniformBlock &uniformBlock = *uniformBlocks[uniformBlockIndex];

		switch(pname)
		{
		case GL_UNIFORM_BLOCK_DATA_SIZE:
			*params = static_cast<GLint>(uniformBlock.dataSize);
			break;
		case GL_UNIFORM_BLOCK_NAME_LENGTH:
			*params = static_cast<GLint>(uniformBlock.name.size() + 1 + (uniformBlock.isArrayElement() ? 3 : 0));
			break;
		case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
			*params = static_cast<GLint>(uniformBlock.memberUniformIndexes.size());
			break;
		case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
			{
				for(unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
				{
					params[blockMemberIndex] = static_cast<GLint>(uniformBlock.memberUniformIndexes[blockMemberIndex]);
				}
			}
			break;
		case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
			*params = static_cast<GLint>(uniformBlock.isReferencedByVertexShader());
			break;
		case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
			*params = static_cast<GLint>(uniformBlock.isReferencedByFragmentShader());
			break;
		default: UNREACHABLE(pname);
		}
	}

	GLuint Program::getUniformBlockIndex(const std::string &name) const
	{
		unsigned int subscript = GL_INVALID_INDEX;
		std::string baseName = es2::ParseUniformName(name, &subscript);

		size_t numUniformBlocks = getActiveUniformBlockCount();
		for(GLuint blockIndex = 0; blockIndex < numUniformBlocks; blockIndex++)
		{
			const UniformBlock &uniformBlock = *uniformBlocks[blockIndex];
			if(uniformBlock.name == baseName)
			{
				const bool arrayElementZero = (subscript == GL_INVALID_INDEX && uniformBlock.elementIndex == 0);
				if(subscript == uniformBlock.elementIndex || arrayElementZero)
				{
					return blockIndex;
				}
			}
		}

		return GL_INVALID_INDEX;
	}

	void Program::bindUniformBlock(GLuint uniformBlockIndex, GLuint uniformBlockBinding)
	{
		if(uniformBlockIndex >= getActiveUniformBlockCount())
		{
			return error(GL_INVALID_VALUE);
		}

		uniformBlockBindings[uniformBlockIndex] = uniformBlockBinding;
	}

	GLuint Program::getUniformBlockBinding(GLuint uniformBlockIndex) const
	{
		if(uniformBlockIndex >= getActiveUniformBlockCount())
		{
			return error(GL_INVALID_VALUE, GL_INVALID_INDEX);
		}
		return uniformBlockBindings[uniformBlockIndex];
	}

	void Program::resetUniformBlockBindings()
	{
		for(unsigned int blockId = 0; blockId < MAX_UNIFORM_BUFFER_BINDINGS; blockId++)
		{
			uniformBlockBindings[blockId] = 0;
		}
	}

	bool Program::setUniformfv(GLint location, GLsizei count, const GLfloat *v, int numElements)
	{
		ASSERT(numElements >= 1 && numElements <= 4);

		static GLenum floatType[] = { GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4 };
		static GLenum boolType[] = { GL_BOOL, GL_BOOL_VEC2, GL_BOOL_VEC3, GL_BOOL_VEC4 };

		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
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

		int index = numElements - 1;
		if(targetUniform->type == floatType[index])
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat)* numElements,
				   v, numElements * sizeof(GLfloat) * count);
		}
		else if(targetUniform->type == boolType[index])
		{
			GLboolean *boolParams = (GLboolean*)targetUniform->data + uniformIndex[location].element * numElements;

			for(int i = 0; i < count * numElements; i++)
			{
				boolParams[i] = (v[i] == 0.0f) ? GL_FALSE : GL_TRUE;
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
	{
		return setUniformfv(location, count, v, 1);
	}

	bool Program::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
	{
		return setUniformfv(location, count, v, 2);
	}

	bool Program::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
	{
		return setUniformfv(location, count, v, 3);
	}

	bool Program::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
	{
		return setUniformfv(location, count, v, 4);
	}

	bool Program::setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum type)
	{
		int numElements;
		switch(type)
		{
		case GL_FLOAT_MAT2:
			numElements = 4;
			break;
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT3x2:
			numElements = 6;
			break;
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT4x2:
			numElements = 8;
			break;
		case GL_FLOAT_MAT3:
			numElements = 9;
			break;
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4x3:
			numElements = 12;
			break;
		case GL_FLOAT_MAT4:
			numElements = 16;
			break;
		default:
			return false;
		}

		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		targetUniform->dirty = true;

		if(targetUniform->type != type)
		{
			return false;
		}

		int size = targetUniform->size();

		if(size == 1 && count > 1)
		{
			return false;   // Attempting to write an array to a non-array uniform is an INVALID_OPERATION
		}

		count = std::min(size - (int)uniformIndex[location].element, count);

		GLfloat* dst = reinterpret_cast<GLfloat*>(targetUniform->data + uniformIndex[location].element * sizeof(GLfloat) * numElements);

		if(transpose == GL_FALSE)
		{
			memcpy(dst, value, numElements * sizeof(GLfloat) * count);
		}
		else
		{
			const int rowSize = VariableRowCount(type);
			const int colSize = VariableColumnCount(type);
			for(int n = 0; n < count; ++n)
			{
				for(int i = 0; i < colSize; ++i)
				{
					for(int j = 0; j < rowSize; ++j)
					{
						dst[i * rowSize + j] = value[j * colSize + i];
					}
				}
				dst += numElements;
				value += numElements;
			}
		}


		return true;
	}

	bool Program::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT2);
	}

	bool Program::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT2x3);
	}

	bool Program::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT2x4);
	}

	bool Program::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT3);
	}

	bool Program::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT3x2);
	}

	bool Program::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT3x4);
	}

	bool Program::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT4);
	}

	bool Program::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT4x2);
	}

	bool Program::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
	{
		return setUniformMatrixfv(location, count, transpose, value, GL_FLOAT_MAT4x3);
	}

	bool Program::setUniform1iv(GLint location, GLsizei count, const GLint *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
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

		if(targetUniform->type == GL_INT || IsSamplerUniform(targetUniform->type))
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

	bool Program::setUniformiv(GLint location, GLsizei count, const GLint *v, int numElements)
	{
		static GLenum intType[] = { GL_INT, GL_INT_VEC2, GL_INT_VEC3, GL_INT_VEC4 };
		static GLenum boolType[] = { GL_BOOL, GL_BOOL_VEC2, GL_BOOL_VEC3, GL_BOOL_VEC4 };

		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
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

		int index = numElements - 1;
		if(targetUniform->type == intType[index])
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLint)* numElements,
				   v, numElements * sizeof(GLint)* count);
		}
		else if(targetUniform->type == boolType[index])
		{
			GLboolean *boolParams = new GLboolean[count * numElements];

			for(int i = 0; i < count * numElements; i++)
			{
				boolParams[i] = (v[i] == 0) ? GL_FALSE : GL_TRUE;
			}

			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLboolean)* numElements,
				   boolParams, numElements * sizeof(GLboolean)* count);

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
		return setUniformiv(location, count, v, 2);
	}

	bool Program::setUniform3iv(GLint location, GLsizei count, const GLint *v)
	{
		return setUniformiv(location, count, v, 3);
	}

	bool Program::setUniform4iv(GLint location, GLsizei count, const GLint *v)
	{
		return setUniformiv(location, count, v, 4);
	}

	bool Program::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
	{
		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
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

		if(targetUniform->type == GL_UNSIGNED_INT)
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLuint),
				   v, sizeof(GLuint)* count);
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
				   boolParams, sizeof(GLboolean)* count);

			delete[] boolParams;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniformuiv(GLint location, GLsizei count, const GLuint *v, int numElements)
	{
		static GLenum uintType[] = { GL_UNSIGNED_INT, GL_UNSIGNED_INT_VEC2, GL_UNSIGNED_INT_VEC3, GL_UNSIGNED_INT_VEC4 };
		static GLenum boolType[] = { GL_BOOL, GL_BOOL_VEC2, GL_BOOL_VEC3, GL_BOOL_VEC4 };

		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
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

		int index = numElements - 1;
		if(targetUniform->type == uintType[index])
		{
			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLuint)* numElements,
				   v, numElements * sizeof(GLuint)* count);
		}
		else if(targetUniform->type == boolType[index])
		{
			GLboolean *boolParams = new GLboolean[count * numElements];

			for(int i = 0; i < count * numElements; i++)
			{
				boolParams[i] = (v[i] == 0) ? GL_FALSE : GL_TRUE;
			}

			memcpy(targetUniform->data + uniformIndex[location].element * sizeof(GLboolean)* numElements,
				   boolParams, numElements * sizeof(GLboolean)* count);

			delete[] boolParams;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Program::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
	{
		return setUniformuiv(location, count, v, 2);
	}

	bool Program::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
	{
		return setUniformuiv(location, count, v, 3);
	}

	bool Program::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
	{
		return setUniformuiv(location, count, v, 4);
	}

	bool Program::getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params)
	{
		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
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
		case GL_UNSIGNED_INT:
			{
				GLuint *uintParams = (GLuint*)targetUniform->data + uniformIndex[location].element * count;

				for(unsigned int i = 0; i < count; i++)
				{
					params[i] = (float)uintParams[i];
				}
			}
			break;
		default: UNREACHABLE(targetUniform->type);
		}

		return true;
	}

	bool Program::getUniformiv(GLint location, GLsizei *bufSize, GLint *params)
	{
		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		unsigned int count = UniformComponentCount(targetUniform->type);

		// Sized query - ensure the provided buffer is large enough
		if(bufSize && static_cast<unsigned int>(*bufSize) < count * sizeof(GLint))
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
		case GL_UNSIGNED_INT:
			memcpy(params, targetUniform->data + uniformIndex[location].element * count * sizeof(GLint),
				   count * sizeof(GLint));
			break;
		default: UNREACHABLE(targetUniform->type);
		}

		return true;
	}

	bool Program::getUniformuiv(GLint location, GLsizei *bufSize, GLuint *params)
	{
		if(location < 0 || location >= (int)uniformIndex.size() || (uniformIndex[location].index == GL_INVALID_INDEX))
		{
			return false;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		unsigned int count = UniformComponentCount(targetUniform->type);

		// Sized query - ensure the provided buffer is large enough
		if(bufSize && static_cast<unsigned int>(*bufSize) < count * sizeof(GLuint))
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
				params[i] = (GLuint)boolParams[i];
			}
		}
			break;
		case GL_FLOAT:
		{
			GLfloat *floatParams = (GLfloat*)targetUniform->data + uniformIndex[location].element * count;

			for(unsigned int i = 0; i < count; i++)
			{
				params[i] = (GLuint)floatParams[i];
			}
		}
			break;
		case GL_INT:
		case GL_UNSIGNED_INT:
			memcpy(params, targetUniform->data + uniformIndex[location].element * count * sizeof(GLuint),
				   count * sizeof(GLuint));
			break;
		default: UNREACHABLE(targetUniform->type);
		}

		return true;
	}

	void Program::dirtyAllUniforms()
	{
		size_t numUniforms = uniforms.size();
		for(size_t index = 0; index < numUniforms; index++)
		{
			uniforms[index]->dirty = true;
		}
	}

	// Applies all the uniforms set for this program object to the device
	void Program::applyUniforms(Device *device)
	{
		GLint numUniforms = static_cast<GLint>(uniformIndex.size());
		for(GLint location = 0; location < numUniforms; location++)
		{
			if((uniformIndex[location].element != 0) || (uniformIndex[location].index == GL_INVALID_INDEX))
			{
				continue;
			}

			Uniform *targetUniform = uniforms[uniformIndex[location].index];

			if(targetUniform->dirty && (targetUniform->blockInfo.index == -1))
			{
				GLsizei size = targetUniform->size();
				GLfloat *f = (GLfloat*)targetUniform->data;
				GLint *i = (GLint*)targetUniform->data;
				GLuint *ui = (GLuint*)targetUniform->data;
				GLboolean *b = (GLboolean*)targetUniform->data;

				switch(targetUniform->type)
				{
				case GL_BOOL:       applyUniform1bv(device, location, size, b);       break;
				case GL_BOOL_VEC2:  applyUniform2bv(device, location, size, b);       break;
				case GL_BOOL_VEC3:  applyUniform3bv(device, location, size, b);       break;
				case GL_BOOL_VEC4:  applyUniform4bv(device, location, size, b);       break;
				case GL_FLOAT:      applyUniform1fv(device, location, size, f);       break;
				case GL_FLOAT_VEC2: applyUniform2fv(device, location, size, f);       break;
				case GL_FLOAT_VEC3: applyUniform3fv(device, location, size, f);       break;
				case GL_FLOAT_VEC4: applyUniform4fv(device, location, size, f);       break;
				case GL_FLOAT_MAT2:   applyUniformMatrix2fv(device, location, size, f);   break;
				case GL_FLOAT_MAT2x3: applyUniformMatrix2x3fv(device, location, size, f); break;
				case GL_FLOAT_MAT2x4: applyUniformMatrix2x4fv(device, location, size, f); break;
				case GL_FLOAT_MAT3x2: applyUniformMatrix3x2fv(device, location, size, f); break;
				case GL_FLOAT_MAT3:   applyUniformMatrix3fv(device, location, size, f);   break;
				case GL_FLOAT_MAT3x4: applyUniformMatrix3x4fv(device, location, size, f); break;
				case GL_FLOAT_MAT4x2: applyUniformMatrix4x2fv(device, location, size, f); break;
				case GL_FLOAT_MAT4x3: applyUniformMatrix4x3fv(device, location, size, f); break;
				case GL_FLOAT_MAT4:   applyUniformMatrix4fv(device, location, size, f);   break;
				case GL_SAMPLER_2D:
				case GL_SAMPLER_CUBE:
				case GL_SAMPLER_2D_RECT_ARB:
				case GL_SAMPLER_EXTERNAL_OES:
				case GL_SAMPLER_3D_OES:
				case GL_SAMPLER_2D_ARRAY:
				case GL_SAMPLER_2D_SHADOW:
				case GL_SAMPLER_CUBE_SHADOW:
				case GL_SAMPLER_2D_ARRAY_SHADOW:
				case GL_INT_SAMPLER_2D:
				case GL_UNSIGNED_INT_SAMPLER_2D:
				case GL_INT_SAMPLER_CUBE:
				case GL_UNSIGNED_INT_SAMPLER_CUBE:
				case GL_INT_SAMPLER_3D:
				case GL_UNSIGNED_INT_SAMPLER_3D:
				case GL_INT_SAMPLER_2D_ARRAY:
				case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
				case GL_INT:        applyUniform1iv(device, location, size, i);       break;
				case GL_INT_VEC2:   applyUniform2iv(device, location, size, i);       break;
				case GL_INT_VEC3:   applyUniform3iv(device, location, size, i);       break;
				case GL_INT_VEC4:   applyUniform4iv(device, location, size, i);       break;
				case GL_UNSIGNED_INT:      applyUniform1uiv(device, location, size, ui); break;
				case GL_UNSIGNED_INT_VEC2: applyUniform2uiv(device, location, size, ui); break;
				case GL_UNSIGNED_INT_VEC3: applyUniform3uiv(device, location, size, ui); break;
				case GL_UNSIGNED_INT_VEC4: applyUniform4uiv(device, location, size, ui); break;
				default:
					UNREACHABLE(targetUniform->type);
				}

				targetUniform->dirty = false;
			}
		}
	}

	void Program::applyUniformBuffers(Device *device, BufferBinding* uniformBuffers)
	{
		GLint vertexUniformBuffers[MAX_UNIFORM_BUFFER_BINDINGS];
		GLint fragmentUniformBuffers[MAX_UNIFORM_BUFFER_BINDINGS];

		for(unsigned int bufferBindingIndex = 0; bufferBindingIndex < MAX_UNIFORM_BUFFER_BINDINGS; bufferBindingIndex++)
		{
			vertexUniformBuffers[bufferBindingIndex] = -1;
		}

		for(unsigned int bufferBindingIndex = 0; bufferBindingIndex < MAX_UNIFORM_BUFFER_BINDINGS; bufferBindingIndex++)
		{
			fragmentUniformBuffers[bufferBindingIndex] = -1;
		}

		int vertexUniformBufferIndex = 0;
		int fragmentUniformBufferIndex = 0;
		for(unsigned int uniformBlockIndex = 0; uniformBlockIndex < uniformBlocks.size(); uniformBlockIndex++)
		{
			UniformBlock &uniformBlock = *uniformBlocks[uniformBlockIndex];

			// Unnecessary to apply an unreferenced standard or shared UBO
			if(!uniformBlock.isReferencedByVertexShader() && !uniformBlock.isReferencedByFragmentShader())
			{
				continue;
			}

			GLuint blockBinding = uniformBlockBindings[uniformBlockIndex];

			if(uniformBlock.isReferencedByVertexShader())
			{
				vertexUniformBuffers[vertexUniformBufferIndex++] = blockBinding;
			}

			if(uniformBlock.isReferencedByFragmentShader())
			{
				fragmentUniformBuffers[fragmentUniformBufferIndex++] = blockBinding;
			}
		}

		for(unsigned int bufferBindingIndex = 0; bufferBindingIndex < MAX_UNIFORM_BUFFER_BINDINGS; bufferBindingIndex++)
		{
			int index = vertexUniformBuffers[bufferBindingIndex];
			Buffer* vsBuffer = (index != -1) ? (Buffer*)uniformBuffers[index].get() : nullptr;
			device->VertexProcessor::setUniformBuffer(bufferBindingIndex,
				vsBuffer ? vsBuffer->getResource() : nullptr, (index != -1) ? uniformBuffers[index].getOffset() : 0);
			index = fragmentUniformBuffers[bufferBindingIndex];
			Buffer* psBuffer = (index != -1) ? (Buffer*)uniformBuffers[index].get() : nullptr;
			device->PixelProcessor::setUniformBuffer(bufferBindingIndex,
				psBuffer ? psBuffer->getResource() : nullptr, (index != -1) ? uniformBuffers[index].getOffset() : 0);
		}
	}

	void Program::applyTransformFeedback(Device *device, TransformFeedback* transformFeedback)
	{
		// Make sure the flags will fit in a 64 bit unsigned int variable
		ASSERT(sw::max<int>(MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS) <= 64);

		BufferBinding* transformFeedbackBuffers = (transformFeedback && transformFeedback->isActive() && !transformFeedback->isPaused()) ? transformFeedback->getBuffers() : nullptr;

		uint64_t enableTransformFeedback = 0;
		if(!transformFeedbackBuffers)
		{
			for(unsigned int index = 0; index < sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS; ++index)
			{
				device->VertexProcessor::setTransformFeedbackBuffer(index, nullptr, 0, 0, 0, 0, 0);
			}
			device->VertexProcessor::enableTransformFeedback(enableTransformFeedback);
			return;
		}

		unsigned int maxVaryings = static_cast<unsigned int>(transformFeedbackLinkedVaryings.size());
		switch(transformFeedbackBufferMode)
		{
		case GL_SEPARATE_ATTRIBS:
		{
			maxVaryings = sw::min(maxVaryings, (unsigned int)MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS);
			// Attribs go to separate buffers
			for(unsigned int index = 0; index < maxVaryings; ++index)
			{
				int size = transformFeedbackLinkedVaryings[index].size;
				int rowCount = VariableRowCount(transformFeedbackLinkedVaryings[index].type);
				int colCount = VariableColumnCount(transformFeedbackLinkedVaryings[index].type);
				int nbRegs = rowCount > 1 ? colCount * size : size;
				int nbComponentsPerReg = rowCount > 1 ? rowCount : colCount;
				int componentStride = rowCount * colCount * size;
				int baseOffset = transformFeedback->vertexOffset() * componentStride * sizeof(float);
				device->VertexProcessor::setTransformFeedbackBuffer(index,
					transformFeedbackBuffers[index].get()->getResource(),
					transformFeedbackBuffers[index].getOffset() + baseOffset,
					transformFeedbackLinkedVaryings[index].reg * 4 + transformFeedbackLinkedVaryings[index].col,
					nbRegs, nbComponentsPerReg, componentStride);
				enableTransformFeedback |= 1ULL << index;
			}
		}
			break;
		case GL_INTERLEAVED_ATTRIBS:
		{
			// OpenGL ES 3.0.4 spec, section 2.15.2:
			// In INTERLEAVED_ATTRIBS mode, the values of one or more output variables
			// written by a vertex shader are written, interleaved, into the buffer object
			// bound to the first transform feedback binding point (index = 0).
			sw::Resource* resource = transformFeedbackBuffers[0].get()->getResource();
			int componentStride = static_cast<int>(totalLinkedVaryingsComponents);
			int baseOffset = transformFeedbackBuffers[0].getOffset() + (transformFeedback->vertexOffset() * componentStride * sizeof(float));
			maxVaryings = sw::min(maxVaryings, (unsigned int)sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS);
			int totalComponents = 0;
			for(unsigned int index = 0; index < maxVaryings; ++index)
			{
				int size = transformFeedbackLinkedVaryings[index].size;
				int rowCount = VariableRowCount(transformFeedbackLinkedVaryings[index].type);
				int colCount = VariableColumnCount(transformFeedbackLinkedVaryings[index].type);
				int nbRegs = rowCount > 1 ? colCount * size : size;
				int nbComponentsPerReg = rowCount > 1 ? rowCount : colCount;
				device->VertexProcessor::setTransformFeedbackBuffer(index, resource,
					baseOffset + (totalComponents * sizeof(float)),
					transformFeedbackLinkedVaryings[index].reg * 4 + transformFeedbackLinkedVaryings[index].col,
					nbRegs, nbComponentsPerReg, componentStride);
				totalComponents += rowCount * colCount * size;
				enableTransformFeedback |= 1ULL << index;
			}
		}
			break;
		default:
			UNREACHABLE(transformFeedbackBufferMode);
			break;
		}

		// Unset all other transform feedback buffers
		for(unsigned int index = maxVaryings; index < sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS; ++index)
		{
			device->VertexProcessor::setTransformFeedbackBuffer(index, nullptr, 0, 0, 0, 0, 0);
		}

		device->VertexProcessor::enableTransformFeedback(enableTransformFeedback);
	}

	bool Program::linkVaryings()
	{
		glsl::VaryingList &psVaryings = fragmentShader->varyings;
		glsl::VaryingList &vsVaryings = vertexShader->varyings;

		for(auto const &input : psVaryings)
		{
			bool matched = false;

			for(auto const &output : vsVaryings)
			{
				if(output.name == input.name)
				{
					if(output.type != input.type || output.size() != input.size())
					{
						appendToInfoLog("Type of vertex varying %s does not match that of the fragment varying", output.name.c_str());

						return false;
					}

					if((output.qualifier == EvqFlatOut) ^ (input.qualifier == EvqFlatIn))
					{
						appendToInfoLog("Interpolation qualifiers for %s differ between vertex and fragment shaders", output.name.c_str());

						return false;
					}

					if(!areMatchingFields(input.fields, output.fields, input.name))
					{
						return false;
					}

					matched = true;
					break;
				}
			}

			if(!matched)
			{
				// If a fragment varying is declared but not statically used, it's not an error to not have a matching vertex varying.
				if(input.registerIndex >= 0)
				{
					appendToInfoLog("Fragment varying %s does not match any vertex varying", input.name.c_str());

					return false;
				}
			}
		}

		for(auto const &output : vsVaryings)
		{
			bool matched = false;

			for(auto const &input : psVaryings)
			{
				if(output.name == input.name)
				{
					int in = input.registerIndex;
					int out = output.registerIndex;
					int components = VariableRegisterSize(output.type);
					int registers = VariableRegisterCount(output.type) * output.size();

					if(in < 0)  // Fragment varying declared but not used
					{
						continue;
					}

					if(in + registers >= MAX_VARYING_VECTORS)
					{
						appendToInfoLog("Too many varyings");
						return false;
					}

					if(out >= 0)
					{
						if(out + registers >= MAX_VARYING_VECTORS)
						{
							appendToInfoLog("Too many varyings");
							return false;
						}

						for(int i = 0; i < registers; i++)
						{
							vertexBinary->setOutput(out + i, components, sw::Shader::Semantic(sw::Shader::USAGE_COLOR, in + i, pixelBinary->getInput(in + i, 0).flat));
						}
					}
					else   // Vertex varying is declared but not written to
					{
						for(int i = 0; i < registers; i++)
						{
							pixelBinary->setInput(in + i, components, sw::Shader::Semantic());
						}
					}

					matched = true;
					break;
				}
			}

			if(!matched)
			{
				// For openGL ES 3.0, we need to still add the vertex shader outputs for unmatched varyings, for transform feedback.
				for(const std::string &indexedTfVaryingName : transformFeedbackVaryings)
				{
					std::string tfVaryingName = es2::ParseUniformName(indexedTfVaryingName, nullptr);

					if(tfVaryingName == output.name)
					{
						int out = output.registerIndex;
						int components = VariableRegisterSize(output.type);
						int registers = VariableRegisterCount(output.type) * output.size();

						if(out >= 0)
						{
							if(out + registers >= MAX_VARYING_VECTORS)
							{
								appendToInfoLog("Too many varyings");
								return false;
							}

							for(int i = 0; i < registers; i++)
							{
								vertexBinary->setOutput(out + i, components, sw::Shader::Semantic(sw::Shader::USAGE_COLOR));
							}
						}
						break;
					}
				}
			}
		}

		return true;
	}

	bool Program::linkTransformFeedback()
	{
		size_t totalComponents = 0;
		totalLinkedVaryingsComponents = 0;

		std::set<std::string> uniqueNames;

		for(const std::string &indexedTfVaryingName : transformFeedbackVaryings)
		{
			unsigned int subscript = GL_INVALID_INDEX;
			std::string tfVaryingName = es2::ParseUniformName(indexedTfVaryingName, &subscript);
			bool hasSubscript = (subscript != GL_INVALID_INDEX);

			if(tfVaryingName.find('[') != std::string::npos)
			{
				appendToInfoLog("Capture of array sub-elements is undefined and not supported.");
				return false;
			}

			bool found = false;
			for(const glsl::Varying varying : vertexShader->varyings)
			{
				if(tfVaryingName == varying.name)
				{
					if(uniqueNames.count(indexedTfVaryingName) > 0)
					{
						appendToInfoLog("Two transform feedback varyings specify the same output variable (%s)", indexedTfVaryingName.c_str());
						return false;
					}
					uniqueNames.insert(indexedTfVaryingName);

					if(hasSubscript && ((static_cast<int>(subscript)) >= varying.size()))
					{
						appendToInfoLog("Specified transform feedback varying index out of bounds (%s)", indexedTfVaryingName.c_str());
						return false;
					}

					int size = hasSubscript ? 1 : varying.size();

					int rowCount = VariableRowCount(varying.type);
					int colCount = VariableColumnCount(varying.type);
					int componentCount = rowCount * colCount * size;
					if(transformFeedbackBufferMode == GL_SEPARATE_ATTRIBS &&
					   componentCount > sw::MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS)
					{
						appendToInfoLog("Transform feedback varying's %s components (%d) exceed the maximum separate components (%d).",
						                varying.name.c_str(), componentCount, sw::MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS);
						return false;
					}

					totalComponents += componentCount;

					int reg = varying.registerIndex;
					if(hasSubscript)
					{
						reg += rowCount > 1 ? colCount * subscript : subscript;
					}
					int col = varying.column;
					if(tfVaryingName == "gl_PointSize")
					{
						// Point size is stored in the y element of the vector, not the x element
						col = 1; // FIXME: varying.col could already contain this information
					}
					transformFeedbackLinkedVaryings.push_back(LinkedVarying(varying.name, varying.type, size, reg, col));

					found = true;
					break;
				}
			}

			if(!found)
			{
				appendToInfoLog("Transform feedback varying %s does not exist in the vertex shader.", tfVaryingName.c_str());
				return false;
			}
		}

		if(transformFeedbackBufferMode == GL_INTERLEAVED_ATTRIBS &&
		   totalComponents > sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS)
		{
			appendToInfoLog("Transform feedback varying total components (%d) exceed the maximum separate components (%d).",
			                totalComponents, sw::MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS);
			return false;
		}

		totalLinkedVaryingsComponents = totalComponents;

		return true;
	}

	// Links the code of the vertex and pixel shader by matching up their varyings,
	// compiling them into binaries, determining the attribute mappings, and collecting
	// a list of uniforms
	void Program::link()
	{
		unlink();

		resetUniformBlockBindings();

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

		// Link uniform blocks before uniforms to make it easy to assign block indices to fields
		if(!linkUniformBlocks(vertexShader, fragmentShader))
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

		if(!linkTransformFeedback())
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
		for(auto const &attribute : vertexShader->activeAttributes)
		{
			int location = (attributeBinding.find(attribute.name) != attributeBinding.end()) ? attributeBinding[attribute.name] : -1;

			if(location != -1)   // Set by glBindAttribLocation
			{
				int rows = VariableRegisterCount(attribute.type);

				if(rows + location > MAX_VERTEX_ATTRIBS)
				{
					appendToInfoLog("Active attribute (%s) at location %d is too big to fit", attribute.name.c_str(), location);
					return false;
				}

				// In GLSL 3.00, attribute aliasing produces a link error
				// In GLSL 1.00, attribute aliasing is allowed
				if(vertexShader->getShaderVersion() >= 300)
				{
					for(auto const &it : linkedAttribute)
					{
						int itLocStart = getAttributeBinding(it);
						ASSERT(itLocStart >= 0);
						int itLocEnd = itLocStart + VariableRegisterCount(it.type);
						for(int i = 0; i < rows; i++)
						{
							int loc = location + i;
							if((loc >= itLocStart) && (loc < itLocEnd))
							{
								appendToInfoLog("Attribute '%s' aliases attribute '%s' at location %d", attribute.name.c_str(), it.name.c_str(), location);
								return false;
							}
						}
					}
				}

				linkedAttributeLocation[attribute.name] = location;
				linkedAttribute.push_back(attribute);
				for(int i = 0; i < rows; i++)
				{
					usedLocations |= 1 << (location + i);
				}
			}
		}

		// Link attributes that don't have a binding location
		for(auto const &attribute : vertexShader->activeAttributes)
		{
			int location = (attributeBinding.find(attribute.name) != attributeBinding.end()) ? attributeBinding[attribute.name] : -1;

			if(location == -1)   // Not set by glBindAttribLocation
			{
				int rows = VariableRegisterCount(attribute.type);
				int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, MAX_VERTEX_ATTRIBS);

				if(availableIndex == -1 || availableIndex + rows > MAX_VERTEX_ATTRIBS)
				{
					appendToInfoLog("Too many active attributes (%s)", attribute.name.c_str());
					return false;   // Fail to link
				}

				linkedAttributeLocation[attribute.name] = availableIndex;
				linkedAttribute.push_back(attribute);
			}
		}

		for(auto const &it : linkedAttribute)
		{
			int location = getAttributeBinding(it);
			ASSERT(location >= 0);
			int index = vertexShader->getSemanticIndex(it.name);
			int rows = std::max(VariableRegisterCount(it.type), 1);

			for(int r = 0; r < rows; r++)
			{
				attributeStream[r + location] = index++;
			}
		}

		return true;
	}

	int Program::getAttributeBinding(const glsl::Attribute &attribute)
	{
		if(attribute.location != -1)
		{
			return attribute.location;
		}

		std::map<std::string, GLuint>::const_iterator it = linkedAttributeLocation.find(attribute.name);
		if(it != linkedAttributeLocation.end())
		{
			return it->second;
		}

		return -1;
	}

	bool Program::linkUniforms(const Shader *shader)
	{
		for(const auto &uniform : shader->activeUniforms)
		{
			unsigned int blockIndex = GL_INVALID_INDEX;
			if(uniform.blockId >= 0)
			{
				const glsl::ActiveUniformBlocks &activeUniformBlocks = shader->activeUniformBlocks;
				ASSERT(static_cast<size_t>(uniform.blockId) < activeUniformBlocks.size());
				blockIndex = getUniformBlockIndex(activeUniformBlocks[uniform.blockId].name);
				ASSERT(blockIndex != GL_INVALID_INDEX);
			}
			if(!defineUniform(shader->getType(), uniform, Uniform::BlockInfo(uniform, blockIndex)))
			{
				return false;
			}
		}

		for(const auto &uniformStruct : shader->activeUniformStructs)
		{
			if(!validateUniformStruct(shader->getType(), uniformStruct))
			{
				return false;
			}
		}

		return true;
	}

	bool Program::defineUniform(GLenum shader, const glsl::Uniform &glslUniform, const Uniform::BlockInfo& blockInfo)
	{
		if(IsSamplerUniform(glslUniform.type))
	    {
			int index = glslUniform.registerIndex;

			do
			{
				if(shader == GL_VERTEX_SHADER)
				{
					if(index < MAX_VERTEX_TEXTURE_IMAGE_UNITS)
					{
						samplersVS[index].active = true;

						switch(glslUniform.type)
						{
						default:                      UNREACHABLE(glslUniform.type);
						case GL_INT_SAMPLER_2D:
						case GL_UNSIGNED_INT_SAMPLER_2D:
						case GL_SAMPLER_2D_SHADOW:
						case GL_SAMPLER_2D:           samplersVS[index].textureType = TEXTURE_2D;       break;
						case GL_INT_SAMPLER_CUBE:
						case GL_UNSIGNED_INT_SAMPLER_CUBE:
						case GL_SAMPLER_CUBE_SHADOW:
						case GL_SAMPLER_CUBE:         samplersVS[index].textureType = TEXTURE_CUBE;     break;
						case GL_INT_SAMPLER_3D:
						case GL_UNSIGNED_INT_SAMPLER_3D:
						case GL_SAMPLER_3D_OES:       samplersVS[index].textureType = TEXTURE_3D;       break;
						case GL_SAMPLER_2D_RECT_ARB:  samplersVS[index].textureType = TEXTURE_2D_RECT;  break;
						case GL_SAMPLER_EXTERNAL_OES: samplersVS[index].textureType = TEXTURE_EXTERNAL; break;
						case GL_INT_SAMPLER_2D_ARRAY:
						case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
						case GL_SAMPLER_2D_ARRAY_SHADOW:
						case GL_SAMPLER_2D_ARRAY:     samplersVS[index].textureType = TEXTURE_2D_ARRAY; break;
						}

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

						switch(glslUniform.type)
						{
						default:                      UNREACHABLE(glslUniform.type);
						case GL_INT_SAMPLER_2D:
						case GL_UNSIGNED_INT_SAMPLER_2D:
						case GL_SAMPLER_2D_SHADOW:
						case GL_SAMPLER_2D:           samplersPS[index].textureType = TEXTURE_2D;       break;
						case GL_INT_SAMPLER_CUBE:
						case GL_UNSIGNED_INT_SAMPLER_CUBE:
						case GL_SAMPLER_CUBE_SHADOW:
						case GL_SAMPLER_CUBE:         samplersPS[index].textureType = TEXTURE_CUBE;     break;
						case GL_INT_SAMPLER_3D:
						case GL_UNSIGNED_INT_SAMPLER_3D:
						case GL_SAMPLER_3D_OES:       samplersPS[index].textureType = TEXTURE_3D;       break;
						case GL_SAMPLER_2D_RECT_ARB:  samplersPS[index].textureType = TEXTURE_2D_RECT;  break;
						case GL_SAMPLER_EXTERNAL_OES: samplersPS[index].textureType = TEXTURE_EXTERNAL; break;
						case GL_INT_SAMPLER_2D_ARRAY:
						case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
						case GL_SAMPLER_2D_ARRAY_SHADOW:
						case GL_SAMPLER_2D_ARRAY:     samplersPS[index].textureType = TEXTURE_2D_ARRAY; break;
						}

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
			while(index < glslUniform.registerIndex + static_cast<int>(glslUniform.arraySize));
	    }

		Uniform *uniform = 0;
		GLint location = getUniformLocation(glslUniform.name);

		if(location >= 0)   // Previously defined, types must match
		{
			uniform = uniforms[uniformIndex[location].index];

			if(uniform->type != glslUniform.type)
			{
				appendToInfoLog("Types for uniform %s do not match between the vertex and fragment shader", uniform->name.c_str());
				return false;
			}

			if(uniform->precision != glslUniform.precision)
			{
				appendToInfoLog("Precisions for uniform %s do not match between the vertex and fragment shader", uniform->name.c_str());
				return false;
			}

			if(!areMatchingFields(uniform->fields, glslUniform.fields, uniform->name))
			{
				return false;
			}
		}
		else
		{
			uniform = new Uniform(glslUniform, blockInfo);
		}

		if(!uniform)
		{
			return false;
		}

		if(shader == GL_VERTEX_SHADER)
		{
			uniform->vsRegisterIndex = glslUniform.registerIndex;
		}
		else if(shader == GL_FRAGMENT_SHADER)
		{
			uniform->psRegisterIndex = glslUniform.registerIndex;
		}
		else UNREACHABLE(shader);

		if(!isUniformDefined(glslUniform.name))
		{
			uniforms.push_back(uniform);
			unsigned int index = (blockInfo.index == -1) ? static_cast<unsigned int>(uniforms.size() - 1) : GL_INVALID_INDEX;

			for(int i = 0; i < uniform->size(); i++)
			{
				uniformIndex.push_back(UniformLocation(glslUniform.name, i, index));
			}
		}

		if(shader == GL_VERTEX_SHADER)
		{
			if(glslUniform.registerIndex + uniform->registerCount() > MAX_VERTEX_UNIFORM_VECTORS)
			{
				appendToInfoLog("Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS (%d)", MAX_VERTEX_UNIFORM_VECTORS);
				return false;
			}
		}
		else if(shader == GL_FRAGMENT_SHADER)
		{
			if(glslUniform.registerIndex + uniform->registerCount() > MAX_FRAGMENT_UNIFORM_VECTORS)
			{
				appendToInfoLog("Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS (%d)", MAX_FRAGMENT_UNIFORM_VECTORS);
				return false;
			}
		}
		else UNREACHABLE(shader);

		return true;
	}

	bool Program::validateUniformStruct(GLenum shader, const glsl::Uniform &newUniformStruct)
	{
		for(const auto &uniformStruct : uniformStructs)
		{
			if(uniformStruct.name == newUniformStruct.name)
			{
				return areMatchingFields(uniformStruct.fields, newUniformStruct.fields, newUniformStruct.name);
			}
		}

		uniformStructs.push_back(Uniform(newUniformStruct, Uniform::BlockInfo(newUniformStruct, -1)));

		return true;
	}

	bool Program::areMatchingUniformBlocks(const glsl::UniformBlock &block1, const glsl::UniformBlock &block2, const Shader *shader1, const Shader *shader2)
	{
		// validate blocks for the same member types
		if(block1.fields.size() != block2.fields.size())
		{
			appendToInfoLog("Types for interface block '%s' differ between vertex and fragment shaders", block1.name.c_str());
			return false;
		}
		if(block1.arraySize != block2.arraySize)
		{
			appendToInfoLog("Array sizes differ for interface block '%s' between vertex and fragment shaders", block1.name.c_str());
			return false;
		}
		if(block1.layout != block2.layout || block1.isRowMajorLayout != block2.isRowMajorLayout)
		{
			appendToInfoLog("Layout qualifiers differ for interface block '%s' between vertex and fragment shaders", block1.name.c_str());
			return false;
		}
		const size_t numBlockMembers = block1.fields.size();
		for(size_t blockMemberIndex = 0; blockMemberIndex < numBlockMembers; blockMemberIndex++)
		{
			const glsl::Uniform& member1 = shader1->activeUniforms[block1.fields[blockMemberIndex]];
			const glsl::Uniform& member2 = shader2->activeUniforms[block2.fields[blockMemberIndex]];
			if(member1.name != member2.name)
			{
				appendToInfoLog("Name mismatch for field %d of interface block '%s': (in vertex: '%s', in fragment: '%s')",
				                blockMemberIndex, block1.name.c_str(), member1.name.c_str(), member2.name.c_str());
				return false;
			}
			if(member1.arraySize != member2.arraySize)
			{
				appendToInfoLog("Array sizes for %s differ between vertex and fragment shaders", member1.name.c_str());
				return false;
			}
			if(member1.precision != member2.precision)
			{
				appendToInfoLog("Precisions for %s differ between vertex and fragment shaders", member1.name.c_str());
				return false;
			}
			if(member1.type != member2.type)
			{
				appendToInfoLog("Types for %s differ between vertex and fragment shaders", member1.name.c_str());
				return false;
			}
			if(member1.blockInfo.isRowMajorMatrix != member2.blockInfo.isRowMajorMatrix)
			{
				appendToInfoLog("Matrix packings for %s differ between vertex and fragment shaders", member1.name.c_str());
				return false;
			}
		}
		return true;
	}

	bool Program::areMatchingFields(const std::vector<glsl::ShaderVariable>& fields1, const std::vector<glsl::ShaderVariable>& fields2, const std::string& name)
	{
		if(fields1.size() != fields2.size())
		{
			appendToInfoLog("Structure lengths for %s differ between vertex and fragment shaders", name.c_str());
			return false;
		}

		for(size_t i = 0; i < fields1.size(); ++i)
		{
			if(fields1[i].name != fields2[i].name)
			{
				appendToInfoLog("Name mismatch for field '%d' of %s: ('%s', '%s')",
				                i, name.c_str(), fields1[i].name.c_str(), fields2[i].name.c_str());
				return false;
			}
			if(fields1[i].type != fields2[i].type)
			{
				appendToInfoLog("Type for %s.%s differ between vertex and fragment shaders", name.c_str(), fields1[i].name.c_str());
				return false;
			}
			if(fields1[i].arraySize != fields2[i].arraySize)
			{
				appendToInfoLog("Array size for %s.%s differ between vertex and fragment shaders", name.c_str(), fields1[i].name.c_str());
				return false;
			}
			if(!areMatchingFields(fields1[i].fields, fields2[i].fields, fields1[i].name))
			{
				return false;
			}
		}

		return true;
	}

	bool Program::linkUniformBlocks(const Shader *vertexShader, const Shader *fragmentShader)
	{
		const glsl::ActiveUniformBlocks &vertexUniformBlocks = vertexShader->activeUniformBlocks;
		const glsl::ActiveUniformBlocks &fragmentUniformBlocks = fragmentShader->activeUniformBlocks;
		// Check that interface blocks defined in the vertex and fragment shaders are identical
		typedef std::map<std::string, const glsl::UniformBlock*> UniformBlockMap;
		UniformBlockMap linkedUniformBlocks;
		for(unsigned int blockIndex = 0; blockIndex < vertexUniformBlocks.size(); blockIndex++)
		{
			const glsl::UniformBlock &vertexUniformBlock = vertexUniformBlocks[blockIndex];
			linkedUniformBlocks[vertexUniformBlock.name] = &vertexUniformBlock;
		}
		for(unsigned int blockIndex = 0; blockIndex < fragmentUniformBlocks.size(); blockIndex++)
		{
			const glsl::UniformBlock &fragmentUniformBlock = fragmentUniformBlocks[blockIndex];
			UniformBlockMap::const_iterator entry = linkedUniformBlocks.find(fragmentUniformBlock.name);
			if(entry != linkedUniformBlocks.end())
			{
				const glsl::UniformBlock &vertexUniformBlock = *entry->second;
				if(!areMatchingUniformBlocks(vertexUniformBlock, fragmentUniformBlock, vertexShader, fragmentShader))
				{
					return false;
				}
			}
		}
		for(unsigned int blockIndex = 0; blockIndex < vertexUniformBlocks.size(); blockIndex++)
		{
			const glsl::UniformBlock &uniformBlock = vertexUniformBlocks[blockIndex];
			if(!defineUniformBlock(vertexShader, uniformBlock))
			{
				return false;
			}
		}
		for(unsigned int blockIndex = 0; blockIndex < fragmentUniformBlocks.size(); blockIndex++)
		{
			const glsl::UniformBlock &uniformBlock = fragmentUniformBlocks[blockIndex];
			if(!defineUniformBlock(fragmentShader, uniformBlock))
			{
				return false;
			}
		}
		return true;
	}

	bool Program::defineUniformBlock(const Shader *shader, const glsl::UniformBlock &block)
	{
		GLuint blockIndex = getUniformBlockIndex(block.name);

		if(blockIndex == GL_INVALID_INDEX)
		{
			const std::vector<int>& fields = block.fields;
			std::vector<unsigned int> memberUniformIndexes;
			for(size_t i = 0; i < fields.size(); ++i)
			{
				memberUniformIndexes.push_back(fields[i]);
			}

			if(block.arraySize > 0)
			{
				int regIndex = block.registerIndex;
				int regInc = block.dataSize / (glsl::BlockLayoutEncoder::BytesPerComponent * glsl::BlockLayoutEncoder::ComponentsPerRegister);
				for(unsigned int i = 0; i < block.arraySize; ++i, regIndex += regInc)
				{
					uniformBlocks.push_back(new UniformBlock(block.name, i, block.dataSize, memberUniformIndexes));
					uniformBlocks[uniformBlocks.size() - 1]->setRegisterIndex(shader->getType(), regIndex);
				}
			}
			else
			{
				uniformBlocks.push_back(new UniformBlock(block.name, GL_INVALID_INDEX, block.dataSize, memberUniformIndexes));
				uniformBlocks[uniformBlocks.size() - 1]->setRegisterIndex(shader->getType(), block.registerIndex);
			}
		}
		else
		{
			int regIndex = block.registerIndex;
			int regInc = block.dataSize / (glsl::BlockLayoutEncoder::BytesPerComponent * glsl::BlockLayoutEncoder::ComponentsPerRegister);
			int nbBlocks = (block.arraySize > 0) ? block.arraySize : 1;
			for(int i = 0; i < nbBlocks; ++i, regIndex += regInc)
			{
				uniformBlocks[blockIndex + i]->setRegisterIndex(shader->getType(), regIndex);
			}
		}

		return true;
	}

	bool Program::applyUniform(Device *device, GLint location, float* data)
	{
		Uniform *targetUniform = uniforms[uniformIndex[location].index];

		if(targetUniform->psRegisterIndex != -1)
		{
			device->setPixelShaderConstantF(targetUniform->psRegisterIndex, data, targetUniform->registerCount());
		}

		if(targetUniform->vsRegisterIndex != -1)
		{
			device->setVertexShaderConstantF(targetUniform->vsRegisterIndex, data, targetUniform->registerCount());
		}

		return true;
	}

	bool Program::applyUniform1bv(Device *device, GLint location, GLsizei count, const GLboolean *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform2bv(Device *device, GLint location, GLsizei count, const GLboolean *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform3bv(Device *device, GLint location, GLsizei count, const GLboolean *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform4bv(Device *device, GLint location, GLsizei count, const GLboolean *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform1fv(Device *device, GLint location, GLsizei count, const GLfloat *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform2fv(Device *device, GLint location, GLsizei count, const GLfloat *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform3fv(Device *device, GLint location, GLsizei count, const GLfloat *v)
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

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform4fv(Device *device, GLint location, GLsizei count, const GLfloat *v)
	{
		return applyUniform(device, location, (float*)v);
	}

	bool Program::applyUniformMatrix2fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 1) / 2][2][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = 0; matrix[i][0][3] = 0;
			matrix[i][1][0] = value[2];	matrix[i][1][1] = value[3];	matrix[i][1][2] = 0; matrix[i][1][3] = 0;

			value += 4;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix2x3fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 1) / 2][2][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = value[2]; matrix[i][0][3] = 0;
			matrix[i][1][0] = value[3];	matrix[i][1][1] = value[4];	matrix[i][1][2] = value[5]; matrix[i][1][3] = 0;

			value += 6;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix2x4fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 1) / 2][2][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = value[2]; matrix[i][0][3] = value[3];
			matrix[i][1][0] = value[4];	matrix[i][1][1] = value[5];	matrix[i][1][2] = value[6]; matrix[i][1][3] = value[7];

			value += 8;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix3fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 2) / 3][3][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = value[2];	matrix[i][0][3] = 0;
			matrix[i][1][0] = value[3];	matrix[i][1][1] = value[4];	matrix[i][1][2] = value[5];	matrix[i][1][3] = 0;
			matrix[i][2][0] = value[6];	matrix[i][2][1] = value[7];	matrix[i][2][2] = value[8];	matrix[i][2][3] = 0;

			value += 9;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix3x2fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 2) / 3][3][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = 0; matrix[i][0][3] = 0;
			matrix[i][1][0] = value[2];	matrix[i][1][1] = value[3];	matrix[i][1][2] = 0; matrix[i][1][3] = 0;
			matrix[i][2][0] = value[4];	matrix[i][2][1] = value[5];	matrix[i][2][2] = 0; matrix[i][2][3] = 0;

			value += 6;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix3x4fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 2) / 3][3][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = value[2]; 	matrix[i][0][3] = value[3];
			matrix[i][1][0] = value[4];	matrix[i][1][1] = value[5];	matrix[i][1][2] = value[6]; 	matrix[i][1][3] = value[7];
			matrix[i][2][0] = value[8];	matrix[i][2][1] = value[9];	matrix[i][2][2] = value[10];	matrix[i][2][3] = value[11];

			value += 12;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix4fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		return applyUniform(device, location, (float*)value);
	}

	bool Program::applyUniformMatrix4x2fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 3) / 4][4][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];	matrix[i][0][2] = 0; matrix[i][0][3] = 0;
			matrix[i][1][0] = value[2];	matrix[i][1][1] = value[3];	matrix[i][1][2] = 0; matrix[i][1][3] = 0;
			matrix[i][2][0] = value[4];	matrix[i][2][1] = value[5];	matrix[i][2][2] = 0; matrix[i][2][3] = 0;
			matrix[i][3][0] = value[6];	matrix[i][3][1] = value[7];	matrix[i][3][2] = 0; matrix[i][3][3] = 0;

			value += 8;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniformMatrix4x3fv(Device *device, GLint location, GLsizei count, const GLfloat *value)
	{
		float matrix[(MAX_UNIFORM_VECTORS + 3) / 4][4][4];

		for(int i = 0; i < count; i++)
		{
			matrix[i][0][0] = value[0];	matrix[i][0][1] = value[1];  matrix[i][0][2] = value[2];  matrix[i][0][3] = 0;
			matrix[i][1][0] = value[3];	matrix[i][1][1] = value[4];  matrix[i][1][2] = value[5];  matrix[i][1][3] = 0;
			matrix[i][2][0] = value[6];	matrix[i][2][1] = value[7];  matrix[i][2][2] = value[8];  matrix[i][2][3] = 0;
			matrix[i][3][0] = value[9];	matrix[i][3][1] = value[10]; matrix[i][3][2] = value[11]; matrix[i][3][3] = 0;

			value += 12;
		}

		return applyUniform(device, location, (float*)matrix);
	}

	bool Program::applyUniform1iv(Device *device, GLint location, GLsizei count, const GLint *v)
	{
		GLint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[i];
			vector[i][1] = 0;
			vector[i][2] = 0;
			vector[i][3] = 0;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		if(IsSamplerUniform(targetUniform->type))
		{
			if(targetUniform->psRegisterIndex != -1)
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

			if(targetUniform->vsRegisterIndex != -1)
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
		}
		else
		{
			return applyUniform(device, location, (float*)vector);
		}

		return true;
	}

	bool Program::applyUniform2iv(Device *device, GLint location, GLsizei count, const GLint *v)
	{
		GLint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 2;
		}

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform3iv(Device *device, GLint location, GLsizei count, const GLint *v)
	{
		GLint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = v[2];
			vector[i][3] = 0;

			v += 3;
		}

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform4iv(Device *device, GLint location, GLsizei count, const GLint *v)
	{
		GLint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = v[2];
			vector[i][3] = v[3];

			v += 4;
		}

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform1uiv(Device *device, GLint location, GLsizei count, const GLuint *v)
	{
		GLuint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[i];
			vector[i][1] = 0;
			vector[i][2] = 0;
			vector[i][3] = 0;
		}

		Uniform *targetUniform = uniforms[uniformIndex[location].index];
		if(IsSamplerUniform(targetUniform->type))
		{
			if(targetUniform->psRegisterIndex != -1)
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

			if(targetUniform->vsRegisterIndex != -1)
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
		}
		else
		{
			return applyUniform(device, location, (float*)vector);
		}

		return true;
	}

	bool Program::applyUniform2uiv(Device *device, GLint location, GLsizei count, const GLuint *v)
	{
		GLuint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = 0;
			vector[i][3] = 0;

			v += 2;
		}

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform3uiv(Device *device, GLint location, GLsizei count, const GLuint *v)
	{
		GLuint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = v[2];
			vector[i][3] = 0;

			v += 3;
		}

		return applyUniform(device, location, (float*)vector);
	}

	bool Program::applyUniform4uiv(Device *device, GLint location, GLsizei count, const GLuint *v)
	{
		GLuint vector[MAX_UNIFORM_VECTORS][4];

		for(int i = 0; i < count; i++)
		{
			vector[i][0] = v[0];
			vector[i][1] = v[1];
			vector[i][2] = v[2];
			vector[i][3] = v[3];

			v += 4;
		}

		return applyUniform(device, location, (float*)vector);
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

		linkedAttribute.clear();
		linkedAttributeLocation.clear();

		for(int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
		{
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

		while(!uniformBlocks.empty())
		{
			delete uniformBlocks.back();
			uniformBlocks.pop_back();
		}

		uniformIndex.clear();
		transformFeedbackLinkedVaryings.clear();

		delete[] infoLog;
		infoLog = 0;

		linked = false;
	}

	bool Program::isLinked() const
	{
		return linked;
	}

	bool Program::isValidated() const
	{
		return validated;
	}

	GLint Program::getBinaryLength() const
	{
		UNIMPLEMENTED();
		return 0;
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

	size_t Program::getInfoLogLength() const
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

		if(vertexShader && (total < maxCount))
		{
			shaders[total++] = vertexShader->getName();
		}

		if(fragmentShader && (total < maxCount))
		{
			shaders[total++] = fragmentShader->getName();
		}

		if(count)
		{
			*count = total;
		}
	}

	void Program::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
	{
		ASSERT(index < linkedAttribute.size());

		std::vector<glsl::Attribute>::const_iterator it = linkedAttribute.begin() + index;

		if(bufsize > 0)
		{
			const char *string = it->name.c_str();

			strncpy(name, string, bufsize);
			name[bufsize - 1] = '\0';

			if(length)
			{
				*length = static_cast<GLsizei>(strlen(name));
			}
		}

		*size = 1;   // Always a single 'type' instance

		*type = it->type;
	}

	size_t Program::getActiveAttributeCount() const
	{
		return linkedAttribute.size();
	}

	GLint Program::getActiveAttributeMaxLength() const
	{
		int maxLength = 0;

		std::vector<glsl::Attribute>::const_iterator it = linkedAttribute.begin();
		std::vector<glsl::Attribute>::const_iterator itEnd = linkedAttribute.end();
		for(; it != itEnd; ++it)
		{
			maxLength = std::max((int)(it->name.length() + 1), maxLength);
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
				*length = static_cast<GLsizei>(strlen(name));
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

		size_t numUniforms = uniforms.size();
		for(size_t uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
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

	GLint Program::getActiveUniformi(GLuint index, GLenum pname) const
	{
		const Uniform& uniform = *uniforms[index];
		switch(pname)
		{
		case GL_UNIFORM_TYPE:         return static_cast<GLint>(uniform.type);
		case GL_UNIFORM_SIZE:         return static_cast<GLint>(uniform.size());
		case GL_UNIFORM_NAME_LENGTH:  return static_cast<GLint>(uniform.name.size() + 1 + (uniform.isArray() ? 3 : 0));
		case GL_UNIFORM_BLOCK_INDEX:  return uniform.blockInfo.index;
		case GL_UNIFORM_OFFSET:       return uniform.blockInfo.offset;
		case GL_UNIFORM_ARRAY_STRIDE: return uniform.blockInfo.arrayStride;
		case GL_UNIFORM_MATRIX_STRIDE: return uniform.blockInfo.matrixStride;
		case GL_UNIFORM_IS_ROW_MAJOR: return static_cast<GLint>(uniform.blockInfo.isRowMajorMatrix);
		default:
			UNREACHABLE(pname);
			break;
		}
		return 0;
	}

	void Program::getActiveUniformBlockName(GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) const
	{
		if(index >= getActiveUniformBlockCount())
		{
			return error(GL_INVALID_VALUE);
		}

		const UniformBlock &uniformBlock = *uniformBlocks[index];

		if(bufSize > 0)
		{
			std::string string = uniformBlock.name;

			if(uniformBlock.isArrayElement())
			{
				std::ostringstream elementIndex;
				elementIndex << uniformBlock.elementIndex;
				string += "[" + elementIndex.str()  + "]";
			}

			strncpy(name, string.c_str(), bufSize);
			name[bufSize - 1] = '\0';

			if(length)
			{
				*length = static_cast<GLsizei>(strlen(name));
			}
		}
	}

	size_t Program::getActiveUniformBlockCount() const
	{
		return uniformBlocks.size();
	}

	GLint Program::getActiveUniformBlockMaxLength() const
	{
		GLint maxLength = 0;

		if(isLinked())
		{
			size_t numUniformBlocks = getActiveUniformBlockCount();
			for(size_t uniformBlockIndex = 0; uniformBlockIndex < numUniformBlocks; uniformBlockIndex++)
			{
				const UniformBlock &uniformBlock = *uniformBlocks[uniformBlockIndex];
				if(!uniformBlock.name.empty())
				{
					GLint length = static_cast<GLint>(uniformBlock.name.length() + 1);

					// Counting in "[0]".
					const GLint arrayLength = (uniformBlock.isArrayElement() ? 3 : 0);

					maxLength = std::max(length + arrayLength, maxLength);
				}
			}
		}

		return maxLength;
	}

	void Program::setTransformFeedbackVaryings(GLsizei count, const GLchar *const *varyings, GLenum bufferMode)
	{
		transformFeedbackVaryings.resize(count);
		for(GLsizei i = 0; i < count; i++)
		{
			transformFeedbackVaryings[i] = varyings[i];
		}

		transformFeedbackBufferMode = bufferMode;
	}

	void Program::getTransformFeedbackVarying(GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) const
	{
		if(linked)
		{
			ASSERT(index < transformFeedbackLinkedVaryings.size());
			const LinkedVarying &varying = transformFeedbackLinkedVaryings[index];
			GLsizei lastNameIdx = std::min(bufSize - 1, static_cast<GLsizei>(varying.name.length()));
			if(length)
			{
				*length = lastNameIdx;
			}
			if(size)
			{
				*size = varying.size;
			}
			if(type)
			{
				*type = varying.type;
			}
			if(name)
			{
				memcpy(name, varying.name.c_str(), lastNameIdx);
				name[lastNameIdx] = '\0';
			}
		}
	}

	GLsizei Program::getTransformFeedbackVaryingCount() const
	{
		if(linked)
		{
			return static_cast<GLsizei>(transformFeedbackLinkedVaryings.size());
		}
		else
		{
			return 0;
		}
	}

	GLsizei Program::getTransformFeedbackVaryingMaxLength() const
	{
		if(linked)
		{
			GLsizei maxSize = 0;
			for(size_t i = 0; i < transformFeedbackLinkedVaryings.size(); i++)
			{
				const LinkedVarying &varying = transformFeedbackLinkedVaryings[i];
				maxSize = std::max(maxSize, static_cast<GLsizei>(varying.name.length() + 1));
			}

			return maxSize;
		}
		else
		{
			return 0;
		}
	}

	GLenum Program::getTransformFeedbackBufferMode() const
	{
		return transformFeedbackBufferMode;
	}

	void Program::flagForDeletion()
	{
		orphaned = true;
	}

	bool Program::isFlaggedForDeletion() const
	{
		return orphaned;
	}

	void Program::validate(Device* device)
	{
		resetInfoLog();

		if(!isLinked())
		{
			appendToInfoLog("Program has not been successfully linked.");
			validated = false;
		}
		else
		{
			applyUniforms(device);
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
