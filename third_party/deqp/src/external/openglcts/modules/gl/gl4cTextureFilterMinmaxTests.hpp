#ifndef _GL4CTEXTUREFILTERMINMAXTESTS_HPP
#define _GL4CTEXTUREFILTERMINMAXTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cTextureFilterMinmaxTests.hpp
 * \brief Conformance tests for the ARB_texture_filter_minmax functionality.
 */ /*-------------------------------------------------------------------*/

#include "deUniquePtr.hpp"
#include "glcTestCase.hpp"
#include "gluTexture.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"

#include <string>
#include <vector>

namespace gl4cts
{
class TextureFilterMinmaxUtils
{
public:
	enum TextureTestFlag
	{
		MINMAX		  = (1u << 0),
		EXCLUDE_3D	= (1u << 2),
		EXCLUDE_CUBE  = (1u << 3)
	};

	struct SupportedTextureDataType
	{

		glw::GLenum m_format;
		glw::GLenum m_type;
		int			m_testFlags;

		SupportedTextureDataType(glw::GLenum format, glw::GLenum type, int testFlags = MINMAX)
			: m_format(format), m_type(type), m_testFlags(testFlags)
		{
		}

		inline bool hasFlag(TextureTestFlag flag) const
		{
			return (m_testFlags & flag) != 0;
		}
	};

	struct ReductionModeParam
	{
		glw::GLint m_reductionMode;
		bool	   m_queryTestOnly;

		ReductionModeParam(glw::GLint reductionMode, bool queryTestOnly)
			: m_reductionMode(reductionMode), m_queryTestOnly(queryTestOnly)
		{
		}
	};

	class SupportedTextureType
	{
	protected:
		glw::GLenum m_type;
		tcu::IVec3  m_size;
		std::string m_vertexShader;
		std::string m_fragmentShader;

		void replaceAll(std::string& str, const std::string& from, const std::string& to);

		void renderQuad(const glu::RenderContext& context, glw::GLint reductionMode);

		virtual glw::GLuint getTextureGL() = 0;

		virtual std::vector<float> getTexCoords() = 0;

	public:
		SupportedTextureType(glw::GLenum type, const std::string& shaderTexcoordType,
							 const std::string& shaderSamplerType);
		virtual ~SupportedTextureType(){};

		inline glw::GLenum getType() const
		{
			return m_type;
		}

		virtual void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
							  bool generateMipmaps = false) = 0;

		void renderToFBO(const glu::RenderContext& context, glw::GLuint resultTextureId, tcu::IVec2 size,
						 glw::GLint reductionMode);
	};

	class Texture1D : public SupportedTextureType
	{
	protected:
		de::MovePtr<glu::Texture1D> m_texture;

		glw::GLuint		   getTextureGL();
		std::vector<float> getTexCoords();

	public:
		Texture1D();

		void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
					  bool generateMipmaps = false);
	};

	class Texture1DArray : public SupportedTextureType
	{
	protected:
		de::MovePtr<glu::Texture1DArray> m_texture;

		glw::GLuint		   getTextureGL();
		std::vector<float> getTexCoords();

	public:
		Texture1DArray();

		void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
					  bool generateMipmaps = false);
	};

	class Texture2D : public SupportedTextureType
	{
	protected:
		de::MovePtr<glu::Texture2D> m_texture;

		glw::GLuint		   getTextureGL();
		std::vector<float> getTexCoords();

	public:
		Texture2D();

		void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
					  bool generateMipmaps = false);
	};

	class Texture2DArray : public SupportedTextureType
	{
	protected:
		de::MovePtr<glu::Texture2DArray> m_texture;

		glw::GLuint		   getTextureGL();
		std::vector<float> getTexCoords();

	public:
		Texture2DArray();

		void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
					  bool generateMipmaps = false);
	};

	class Texture3D : public SupportedTextureType
	{
	protected:
		de::MovePtr<glu::Texture3D> m_texture;

		glw::GLuint		   getTextureGL();
		std::vector<float> getTexCoords();

	public:
		Texture3D();

		void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
					  bool generateMipmaps = false);
	};

	class TextureCube : public SupportedTextureType
	{
	protected:
		de::MovePtr<glu::TextureCube> m_texture;

		glw::GLuint		   getTextureGL();
		std::vector<float> getTexCoords();

	public:
		TextureCube();

		void generate(const glu::RenderContext& context, tcu::IVec3 size, glw::GLenum format, glw::GLenum type,
					  bool generateMipmaps = false);
	};

	/* Public methods */
	TextureFilterMinmaxUtils();
	~TextureFilterMinmaxUtils();

	std::vector<glw::GLuint> getDataFromTexture(const glu::RenderContext& context, glw::GLuint textureId,
												tcu::IVec2 textureSize, glw::GLenum format, glw::GLenum type);

	glw::GLuint calcPixelSumValue(const glu::RenderContext& context, glw::GLuint textureId, tcu::IVec2 textureSize,
								  glw::GLenum format, glw::GLenum type);

	inline const std::vector<ReductionModeParam>& getReductionModeParams()
	{
		return m_reductionModeParams;
	}

	inline const std::vector<SupportedTextureDataType>& getSupportedTextureDataTypes()
	{
		return m_supportedTextureDataTypes;
	}

	inline const std::vector<SupportedTextureType*>& getSupportedTextureTypes()
	{
		return m_supportedTextureTypes;
	}

	typedef std::vector<ReductionModeParam>::const_iterator		  ReductionModeParamIter;
	typedef std::vector<SupportedTextureDataType>::const_iterator SupportedTextureDataTypeIter;
	typedef std::vector<SupportedTextureType*>::const_iterator	SupportedTextureTypeIter;

private:
	/* Private members */
	std::vector<ReductionModeParam>		  m_reductionModeParams;
	std::vector<SupportedTextureDataType> m_supportedTextureDataTypes;
	std::vector<SupportedTextureType*>	m_supportedTextureTypes;
};

/** Test verifies default values and manual setting of reduction mode queries
 **/
class TextureFilterMinmaxParameterQueriesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	TextureFilterMinmaxParameterQueriesTestCase(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void testReductionModeQueriesDefaultValues(const glw::Functions& gl);
	void testReductionModeQueries(const glw::Functions& gl, glw::GLint pname);

	/* Private members */
	TextureFilterMinmaxUtils m_utils;
};

/** Base class for rendering based filtering tests
**/
class TextureFilterMinmaxFilteringTestCaseBase : public deqp::TestCase
{
public:
	/* Public methods */
	TextureFilterMinmaxFilteringTestCaseBase(deqp::Context& context, const char* name, const char* description,
											 float renderScale, bool mipmapping);

	tcu::TestNode::IterateResult iterate();

protected:
	/* Protected members */
	TextureFilterMinmaxUtils m_utils;
	float					 m_renderScale;
	bool					 m_mipmapping;
};

/** Test verifies results of minification filtering for different reduction modes, targets, texture formats
**/
class TextureFilterMinmaxMinificationFilteringTestCase : public TextureFilterMinmaxFilteringTestCaseBase
{
public:
	/* Public methods */
	TextureFilterMinmaxMinificationFilteringTestCase(deqp::Context& context);
};

/** Test verifies results of magnification filtering for different reduction modes, targets, texture formats
**/
class TextureFilterMinmaxMagnificationFilteringTestCase : public TextureFilterMinmaxFilteringTestCaseBase
{
public:
	/* Public methods */
	TextureFilterMinmaxMagnificationFilteringTestCase(deqp::Context& context);
};

/** Test verifies results of minification filtering for different reduction modes, targets, texture formats
	with mipmapping enabled
**/
class TextureFilterMinmaxMipmapMinificationFilteringTestCase : public TextureFilterMinmaxFilteringTestCaseBase
{
public:
	/* Public methods */
	TextureFilterMinmaxMipmapMinificationFilteringTestCase(deqp::Context& context);
};

/** Test verifies results of calling GetInternalFormat to verify support for specific texture types
**/
class TextureFilterMinmaxSupportTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	TextureFilterMinmaxSupportTestCase(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
	TextureFilterMinmaxUtils m_utils;
};

/** Test group which encapsulates all texture filter minmax conformance tests */
class TextureFilterMinmax : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	TextureFilterMinmax(deqp::Context& context);

	void init();

private:
	TextureFilterMinmax(const TextureFilterMinmax& other);
	TextureFilterMinmax& operator=(const TextureFilterMinmax& other);
};

} /* glcts namespace */

#endif // _GL4CTEXTUREFILTERMINMAXTESTS_HPP
