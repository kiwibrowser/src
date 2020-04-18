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

// Program.h: Defines the Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#ifndef LIBGLESV2_PROGRAM_H_
#define LIBGLESV2_PROGRAM_H_

#include "Shader.h"
#include "Context.h"
#include "Shader/PixelShader.hpp"
#include "Shader/VertexShader.hpp"

#include <string>
#include <vector>
#include <set>
#include <map>

namespace es2
{
	class Device;
	class ResourceManager;
	class FragmentShader;
	class VertexShader;

	// Helper struct representing a single shader uniform
	struct Uniform
	{
		struct BlockInfo
		{
			BlockInfo(const glsl::Uniform& uniform, int blockIndex);

			int index;
			int offset;
			int arrayStride;
			int matrixStride;
			bool isRowMajorMatrix;
		};

		Uniform(const glsl::Uniform &uniform, const BlockInfo &blockInfo);

		~Uniform();

		bool isArray() const;
		int size() const;
		int registerCount() const;

		const GLenum type;
		const GLenum precision;
		const std::string name;
		const unsigned int arraySize;
		const BlockInfo blockInfo;
		std::vector<glsl::ShaderVariable> fields;

		unsigned char *data;
		bool dirty;

		short psRegisterIndex;
		short vsRegisterIndex;
	};

	// Helper struct representing a single shader uniform block
	struct UniformBlock
	{
		// use GL_INVALID_INDEX for non-array elements
		UniformBlock(const std::string &name, unsigned int elementIndex, unsigned int dataSize, std::vector<unsigned int> memberUniformIndexes);

		void setRegisterIndex(GLenum shader, unsigned int registerIndex);

		bool isArrayElement() const;
		bool isReferencedByVertexShader() const;
		bool isReferencedByFragmentShader() const;

		const std::string name;
		const unsigned int elementIndex;
		const unsigned int dataSize;

		std::vector<unsigned int> memberUniformIndexes;

		unsigned int psRegisterIndex;
		unsigned int vsRegisterIndex;
	};

	// Struct used for correlating uniforms/elements of uniform arrays to handles
	struct UniformLocation
	{
		UniformLocation(const std::string &name, unsigned int element, unsigned int index);

		std::string name;
		unsigned int element;
		unsigned int index;
	};

	struct LinkedVarying
	{
		LinkedVarying();
		LinkedVarying(const std::string &name, GLenum type, GLsizei size, int reg, int col);

		// Original GL name
		std::string name;

		GLenum type;
		GLsizei size;

		int reg;    // First varying register, assigned during link
		int col;    // First register element, assigned during link
	};

	class Program
	{
	public:
		Program(ResourceManager *manager, GLuint handle);

		~Program();

		bool attachShader(Shader *shader);
		bool detachShader(Shader *shader);
		int getAttachedShadersCount() const;

		sw::PixelShader *getPixelShader();
		sw::VertexShader *getVertexShader();

		void bindAttributeLocation(GLuint index, const char *name);
		GLint getAttributeLocation(const char *name);
		int getAttributeStream(int attributeIndex);

		GLint getSamplerMapping(sw::SamplerType type, unsigned int samplerIndex);
		TextureType getSamplerTextureType(sw::SamplerType type, unsigned int samplerIndex);

		GLuint getUniformIndex(const std::string &name) const;
		GLuint getUniformBlockIndex(const std::string &name) const;
		void bindUniformBlock(GLuint uniformBlockIndex, GLuint uniformBlockBinding);
		GLuint getUniformBlockBinding(GLuint uniformBlockIndex) const;
		void getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const;

		bool isUniformDefined(const std::string &name) const;
		GLint getUniformLocation(const std::string &name) const;
		bool setUniform1fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniform2fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniform3fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniform4fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		bool setUniform1iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform2iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform3iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform4iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform1uiv(GLint location, GLsizei count, const GLuint *v);
		bool setUniform2uiv(GLint location, GLsizei count, const GLuint *v);
		bool setUniform3uiv(GLint location, GLsizei count, const GLuint *v);
		bool setUniform4uiv(GLint location, GLsizei count, const GLuint *v);

		bool getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params);
		bool getUniformiv(GLint location, GLsizei *bufSize, GLint *params);
		bool getUniformuiv(GLint location, GLsizei *bufSize, GLuint *params);

		void dirtyAllUniforms();
		void applyUniforms(Device *device);
		void applyUniformBuffers(Device *device, BufferBinding* uniformBuffers);
		void applyTransformFeedback(Device *device, TransformFeedback* transformFeedback);

		void link();
		bool isLinked() const;
		size_t getInfoLogLength() const;
		void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog);
		void getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders);

		GLint getFragDataLocation(const GLchar *name);

		void getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
		size_t getActiveAttributeCount() const;
		GLint getActiveAttributeMaxLength() const;

		void getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
		size_t getActiveUniformCount() const;
		GLint getActiveUniformMaxLength() const;
		GLint getActiveUniformi(GLuint index, GLenum pname) const;

		void getActiveUniformBlockName(GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) const;
		size_t getActiveUniformBlockCount() const;
		GLint getActiveUniformBlockMaxLength() const;

		void setTransformFeedbackVaryings(GLsizei count, const GLchar *const *varyings, GLenum bufferMode);
		void getTransformFeedbackVarying(GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) const;
		GLsizei getTransformFeedbackVaryingCount() const;
		GLsizei getTransformFeedbackVaryingMaxLength() const;
		GLenum getTransformFeedbackBufferMode() const;

		void addRef();
		void release();
		unsigned int getRefCount() const;
		void flagForDeletion();
		bool isFlaggedForDeletion() const;

		void validate(Device* device);
		bool validateSamplers(bool logErrors);
		bool isValidated() const;

		unsigned int getSerial() const;

		bool getBinaryRetrievableHint() const { return retrievableBinary; }
		void setBinaryRetrievable(bool retrievable) { retrievableBinary = retrievable; }
		GLint getBinaryLength() const;

	private:
		void unlink();
		void resetUniformBlockBindings();

		bool linkVaryings();
		bool linkTransformFeedback();

		bool linkAttributes();
		int getAttributeBinding(const glsl::Attribute &attribute);

		bool linkUniforms(const Shader *shader);
		bool linkUniformBlocks(const Shader *vertexShader, const Shader *fragmentShader);
		bool areMatchingUniformBlocks(const glsl::UniformBlock &block1, const glsl::UniformBlock &block2, const Shader *shader1, const Shader *shader2);
		bool areMatchingFields(const std::vector<glsl::ShaderVariable>& fields1, const std::vector<glsl::ShaderVariable>& fields2, const std::string& name);
		bool validateUniformStruct(GLenum shader, const glsl::Uniform &newUniformStruct);
		bool defineUniform(GLenum shader, const glsl::Uniform &uniform, const Uniform::BlockInfo& blockInfo);
		bool defineUniformBlock(const Shader *shader, const glsl::UniformBlock &block);
		bool applyUniform(Device *device, GLint location, float* data);
		bool applyUniform1bv(Device *device, GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform2bv(Device *device, GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform3bv(Device *device, GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform4bv(Device *device, GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform1fv(Device *device, GLint location, GLsizei count, const GLfloat *v);
		bool applyUniform2fv(Device *device, GLint location, GLsizei count, const GLfloat *v);
		bool applyUniform3fv(Device *device, GLint location, GLsizei count, const GLfloat *v);
		bool applyUniform4fv(Device *device, GLint location, GLsizei count, const GLfloat *v);
		bool applyUniformMatrix2fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix2x3fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix2x4fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix3fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix3x2fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix3x4fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix4fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix4x2fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix4x3fv(Device *device, GLint location, GLsizei count, const GLfloat *value);
		bool applyUniform1iv(Device *device, GLint location, GLsizei count, const GLint *v);
		bool applyUniform2iv(Device *device, GLint location, GLsizei count, const GLint *v);
		bool applyUniform3iv(Device *device, GLint location, GLsizei count, const GLint *v);
		bool applyUniform4iv(Device *device, GLint location, GLsizei count, const GLint *v);
		bool applyUniform1uiv(Device *device, GLint location, GLsizei count, const GLuint *v);
		bool applyUniform2uiv(Device *device, GLint location, GLsizei count, const GLuint *v);
		bool applyUniform3uiv(Device *device, GLint location, GLsizei count, const GLuint *v);
		bool applyUniform4uiv(Device *device, GLint location, GLsizei count, const GLuint *v);

		bool setUniformfv(GLint location, GLsizei count, const GLfloat *v, int numElements);
		bool setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum type);
		bool setUniformiv(GLint location, GLsizei count, const GLint *v, int numElements);
		bool setUniformuiv(GLint location, GLsizei count, const GLuint *v, int numElements);

		void appendToInfoLog(const char *info, ...);
		void resetInfoLog();

		static unsigned int issueSerial();

	private:
		FragmentShader *fragmentShader;
		VertexShader *vertexShader;

		sw::PixelShader *pixelBinary;
		sw::VertexShader *vertexBinary;

		std::map<std::string, GLuint> attributeBinding;
		std::map<std::string, GLuint> linkedAttributeLocation;
		std::vector<glsl::Attribute> linkedAttribute;
		int attributeStream[MAX_VERTEX_ATTRIBS];

		GLuint uniformBlockBindings[MAX_UNIFORM_BUFFER_BINDINGS];

		std::vector<std::string> transformFeedbackVaryings;
		GLenum transformFeedbackBufferMode;
		size_t totalLinkedVaryingsComponents;

		struct Sampler
		{
			bool active;
			GLint logicalTextureUnit;
			TextureType textureType;
		};

		Sampler samplersPS[MAX_TEXTURE_IMAGE_UNITS];
		Sampler samplersVS[MAX_VERTEX_TEXTURE_IMAGE_UNITS];

		typedef std::vector<Uniform*> UniformArray;
		UniformArray uniforms;
		typedef std::vector<Uniform> UniformStructArray;
		UniformStructArray uniformStructs;
		typedef std::vector<UniformLocation> UniformIndex;
		UniformIndex uniformIndex;
		typedef std::vector<UniformBlock*> UniformBlockArray;
		UniformBlockArray uniformBlocks;
		typedef std::vector<LinkedVarying> LinkedVaryingArray;
		LinkedVaryingArray transformFeedbackLinkedVaryings;

		bool linked;
		bool orphaned;   // Flag to indicate that the program can be deleted when no longer in use
		char *infoLog;
		bool validated;
		bool retrievableBinary;

		unsigned int referenceCount;
		const unsigned int serial;

		static unsigned int currentSerial;

		ResourceManager *resourceManager;
		const GLuint handle;
	};
}

#endif   // LIBGLESV2_PROGRAM_H_
