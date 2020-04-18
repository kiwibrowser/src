#ifndef _GL4CSHADERVIEWPORTLAYERARRAYTESTS_HPP
#define _GL4CSHADERVIEWPORTLAYERARRAYTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  gl4cShaderViewportLayerArrayTests.hpp
 * \brief Conformance tests for the ARB_shader_viewport_layer_array functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluDrawUtil.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
#include "tcuVector.hpp"

#include <string>

namespace gl4cts
{

class ShaderViewportLayerArrayUtils
{
public:
	enum ViewportOffset
	{
		OFFSET_VERTEX		= 1,
		OFFSET_TESSELLATION = 2,
		OFFSET_GEOMETRY		= 3
	};

	class ShaderPipeline
	{
	private:
		glu::ShaderProgram* m_program;

		bool		   m_hasTessellationShader;
		bool		   m_hasGeometryShader;
		ViewportOffset m_viewportLayerOffset;
		std::string	m_varName;

		std::string m_vs;
		std::string m_tcs;
		std::string m_tes;
		std::string m_gs;
		std::string m_fs;

		void adaptShaderToPipeline(std::string& shader, const std::string& varKey, const std::string& vsVersion,
								   const std::string& tesVersion, const std::string& gsVersion);
		void adaptShaderToPipeline(std::string& shader, const std::string& varKey, int value);
		void adaptShaderToPipeline(std::string& shader, const std::string& varKey, const std::string& value);

	public:
		ShaderPipeline(bool tesselletionShader, bool geometryShader, int maxViewports, const std::string& varName);
		~ShaderPipeline();

		void create(const glu::RenderContext& context);
		void use(const glu::RenderContext& context);

		inline glu::ShaderProgram* getShaderProgram() const
		{
			return m_program;
		}
		inline bool hasTessellationShader() const
		{
			return m_hasTessellationShader;
		}
		inline ViewportOffset getViewportLayerOffset() const
		{
			return m_viewportLayerOffset;
		}
		inline std::string getVarName() const
		{
			return m_varName;
		}
	};

	static void renderQuad(const glu::RenderContext& context, ShaderPipeline& shaderPipeline, int viewportLayerIndex,
						   tcu::Vec4 color);

	static bool validateColor(tcu::Vec4 testedColor, tcu::Vec4 desiredColor);
};

/** Test verifies functionality of defining viewport by changing value of gl_ViewportIndex in
	Vertex, Tessellation and Geometry shaders, it also verifies if final viewport is defined
	by last stage in the shader's pipeline
**/
class ShaderViewportIndexTestCase : public deqp::TestCase
{
private:
	/* Private methods */
	glw::GLint createMaxViewports();

	/* Private members*/
	bool													   m_isExtensionSupported;
	std::vector<tcu::Vec4>									   m_viewportData;
	std::vector<ShaderViewportLayerArrayUtils::ShaderPipeline> m_shaderPipelines;
	int														   m_maxViewports;
	int														   m_currentViewport;

	typedef std::vector<ShaderViewportLayerArrayUtils::ShaderPipeline>::iterator ShaderPipelineIter;

public:
	/* Public methods */
	ShaderViewportIndexTestCase(deqp::Context& context);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();
};

class ShaderLayerFramebufferTestCaseBase : public deqp::TestCase
{
protected:
	/* Protected methods */
	virtual void createFBO() = 0;
	virtual void deleteFBO() = 0;

	/* Protected members*/
	const int m_layersNum;
	const int m_fboSize;

	bool													   m_isExtensionSupported;
	std::vector<ShaderViewportLayerArrayUtils::ShaderPipeline> m_shaderPipelines;
	deUint32												   m_texture;
	std::vector<deUint32>									   m_fbos;
	deUint32												   m_mainFbo;
	int														   m_currentLayer;

	typedef std::vector<ShaderViewportLayerArrayUtils::ShaderPipeline>::iterator ShaderPipelineIter;

public:
	/* Public methods */
	ShaderLayerFramebufferTestCaseBase(deqp::Context& context, const char* name, const char* description, bool layered);

	void init();
	void deinit();

	tcu::TestNode::IterateResult iterate();
};

/** Test verifies functionality of defining rendering layer by changing value of gl_Layer in
	Vertex, Tessellation and Geometry shaders, it also verifies if final layer is defined
	by last stage in the shader's pipeline
**/
class ShaderLayerFramebufferLayeredTestCase : public ShaderLayerFramebufferTestCaseBase
{
private:
	/* Private methods */
	void createFBO();
	void deleteFBO();

public:
	/* Public methods */
	ShaderLayerFramebufferLayeredTestCase(deqp::Context& context);
};

/** Test verifies that defining rendering layer by changing value of gl_Layer in
	Vertex, Tessellation and Geometry shaders has no effect if framebuffer is not layered
**/
class ShaderLayerFramebufferNonLayeredTestCase : public ShaderLayerFramebufferTestCaseBase
{
private:
	/* Private methods */
	void createFBO();
	void deleteFBO();

public:
	/* Public methods */
	ShaderLayerFramebufferNonLayeredTestCase(deqp::Context& context);
};

/** Test group which encapsulates all ARB_shader_viewport_layer_array conformance tests */
class ShaderViewportLayerArray : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ShaderViewportLayerArray(deqp::Context& context);

	void init();

private:
	ShaderViewportLayerArray(const ShaderViewportLayerArray& other);
	ShaderViewportLayerArray& operator=(const ShaderViewportLayerArray& other);
};

} /* glcts namespace */

#endif // _GL4CSHADERVIEWPORTLAYERARRAYTESTS_HPP
