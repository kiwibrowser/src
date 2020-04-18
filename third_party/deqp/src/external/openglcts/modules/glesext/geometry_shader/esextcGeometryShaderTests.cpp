/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

#include "esextcGeometryShaderTests.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "tcuCommandLine.hpp"

#include "esextcGeometryShaderAPI.hpp"
#include "esextcGeometryShaderAdjacencyTests.hpp"
#include "esextcGeometryShaderBlitting.hpp"
#include "esextcGeometryShaderClipping.hpp"
#include "esextcGeometryShaderConstantVariables.hpp"
#include "esextcGeometryShaderInput.hpp"
#include "esextcGeometryShaderLayeredFBO.hpp"
#include "esextcGeometryShaderLayeredFramebuffer.hpp"
#include "esextcGeometryShaderLayeredRendering.hpp"
#include "esextcGeometryShaderLayeredRenderingBoundaryCondition.hpp"
#include "esextcGeometryShaderLayeredRenderingFBONoAttachment.hpp"
#include "esextcGeometryShaderLimits.hpp"
#include "esextcGeometryShaderLinking.hpp"
#include "esextcGeometryShaderNonarrayInput.hpp"
#include "esextcGeometryShaderOutput.hpp"
#include "esextcGeometryShaderPrimitiveCounter.hpp"
#include "esextcGeometryShaderPrimitiveQueries.hpp"
#include "esextcGeometryShaderProgramResource.hpp"
#include "esextcGeometryShaderQualifiers.hpp"
#include "esextcGeometryShaderRendering.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 * @param extType       Extension type
 **/
GeometryShaderTests::GeometryShaderTests(Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "geometry_shader", "Geometry Shader tests")
{
}

/**
 * Initializes test groups for geometry shader tests
 **/
void GeometryShaderTests::init(void)
{
	/* Creating Test Groups */
	TestCaseGroupBase* renderingTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "rendering", "Rendering (Test Group 1)");
	TestCaseGroupBase* adjacencyTestGroup =
		new GeometryShaderAdjacencyTests(m_context, m_extParams, "adjacency", "Adjacency (Test Group 2)");
	TestCaseGroupBase* programResourceTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "program_resource", "Program Resource (Test Group 3)");
	TestCaseGroupBase* nonarrayInputTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "nonarray_input", "Nonarray input (Test Group 4)");
	TestCaseGroupBase* inputTestGroup = new TestCaseGroupBase(m_context, m_extParams, "input", "Input (Test Group 5)");

	GeometryShaderPrimitiveCounterTestGroup* primitiveCounterTestGroup = new GeometryShaderPrimitiveCounterTestGroup(
		m_context, m_extParams, "primitive_counter", "PrimitiveCounter (Test Group 6)");

	TestCaseGroupBase* layeredRenderingTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "layered_rendering", "Layered rendering (Test Group 7)");
	TestCaseGroupBase* clippingTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "clipping", "Clipping (Test Group 8)");
	TestCaseGroupBase* blittingTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "blitting", "Blitting (Test Group 9)");

	TestCaseGroupBase* layeredRenderingBoudaryCondition =
		new TestCaseGroupBase(m_context, m_extParams, "layered_rendering_boundary_condition",
							  "Layered Rendering Boundary Condition (Test Group 10)");

	TestCaseGroupBase* layeredFramebufferTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "layered_framebuffer", "Layered framebuffer (Test Group 11)");

	TestCaseGroupBase* outputTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "output", "Output (Test Group 12)");
	TestCaseGroupBase* primitiveQueriesTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "primitive_queries", "Primitive Queries (Test Group 13)");

	TestCaseGroupBase* layeredRenderingFBONoAttachmentTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "layered_rendering_fbo_no_attachment",
							  "Layered rendering FBO no attachment (Test Group 14)");

	TestCaseGroupBase* constantVariablesTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "constant_variables", "Constant Variables (Test Group 15)");
	TestCaseGroupBase* limitsTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "limits", "Limits (Test Group 16)");
	TestCaseGroupBase* linkingTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "linking", "Linking (Test Group 17 & Test Group 20)");
	TestCaseGroupBase* apiTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "api", "API interactions (Test Group 18)");
	TestCaseGroupBase* qualifiersTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "qualifiers", "Layout qualifiers (Test Group 22)");
	TestCaseGroupBase* layeredFBOTestGroup =
		new TestCaseGroupBase(m_context, m_extParams, "layered_fbo", "Layered FBOs (Test Group 21) & Test Group 26 ");

	/* Adding tests groups as children of this test suite */
	addChild(adjacencyTestGroup);
	addChild(renderingTestGroup);
	addChild(programResourceTestGroup);
	addChild(nonarrayInputTestGroup);
	addChild(inputTestGroup);
	addChild(primitiveCounterTestGroup);
	addChild(layeredRenderingTestGroup);
	addChild(clippingTestGroup);
	addChild(blittingTestGroup);
	addChild(layeredRenderingBoudaryCondition);
	addChild(layeredFramebufferTestGroup);
	addChild(outputTestGroup);
	addChild(primitiveQueriesTestGroup);
	addChild(layeredRenderingFBONoAttachmentTestGroup);
	addChild(constantVariablesTestGroup);
	addChild(limitsTestGroup);
	addChild(linkingTestGroup);
	addChild(apiTestGroup);
	addChild(qualifiersTestGroup);
	addChild(layeredFBOTestGroup);

	/* Adding tests to test groups */

	/* Rendering test (Test Group 1) */
	renderingTestGroup->addChild(new GeometryShaderRendering(m_context, m_extParams, "rendering", "Test 1"));

	/* Program Resource test (Test Group 3) */
	programResourceTestGroup->addChild(
		new GeometryShaderProgramResourceTest(m_context, m_extParams, "program_resource", "Test 3.1"));

	/* Nonarray input tests (Test Group 4) */
	nonarrayInputTestGroup->addChild(
		new GeometryShaderNonarrayInputCase(m_context, m_extParams, "nonarray_input", "Test 4.1"));

	/* Input tests (Test Group 5) */
	inputTestGroup->addChild(
		new GeometryShader_gl_in_ArrayContentsTest(m_context, m_extParams, "gl_in_array_contents", "Test 5.1"));
	inputTestGroup->addChild(
		new GeometryShader_gl_in_ArrayLengthTest(m_context, m_extParams, "gl_in_array_length", "Test 5.2"));
	inputTestGroup->addChild(
		new GeometryShader_gl_PointSize_ValueTest(m_context, m_extParams, "gl_pointsize_value", "Test 5.3"));
	inputTestGroup->addChild(
		new GeometryShader_gl_Position_ValueTest(m_context, m_extParams, "gl_position_value", "Test 5.4"));

	/* PrimitiveCounter tests (Test Group 6) */
	//inputTestGroup->addChild(new PrimitiveCounter(m_context, m_extParams, "gl_primitive_counter", "Test 6"));
	/* Layered rendering (Test Group 7 & Test Group 21) */
	layeredRenderingTestGroup->addChild(new GeometryShaderLayeredRendering(m_context, m_extParams, "layered_rendering",
																		   "Tests 7.1, 7.2, 7.3, 7.4 & 21.3"));

	/* Clipping (Test group 8)*/
	clippingTestGroup->addChild(new GeometryShaderClipping(m_context, m_extParams, "clipping", "Test 8.1"));

	/* Clipping (Test group 9)*/
	blittingTestGroup->addChild(
		new GeometryShaderBlittingLayeredToNonLayered(m_context, m_extParams, "layered_nonlayered", "Test 9.1"));
	blittingTestGroup->addChild(
		new GeometryShaderBlittingNonLayeredToLayered(m_context, m_extParams, "nonlayered_layered", "Test 9.2"));
	blittingTestGroup->addChild(
		new GeometryShaderBlittingLayeredToLayered(m_context, m_extParams, "layered_layered", "Test 9.3"));

	/* Layered rendering Boundary Condition FBO no attachment (Test Group 10) */
	layeredRenderingBoudaryCondition->addChild(new GeometryShaderLayeredRenderingBoundaryConditionVariousTextures(
		m_context, m_extParams, "layered_rendering_boundary_condition_various_textures", "Test 10.1"));

	layeredRenderingBoudaryCondition->addChild(new GeometryShaderLayeredRenderingBoundaryConditionNoGS(
		m_context, m_extParams, "layered_rendering_boundary_condition_no_gs", "Test 10.2"));

	layeredRenderingBoudaryCondition->addChild(new GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet(
		m_context, m_extParams, "layered_rendering_boundary_condition_no_default_layer", "Test 10.3"));

	layeredRenderingBoudaryCondition->addChild(new GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO(
		m_context, m_extParams, "layered_rendering_boundary_condition_no_layered_fbo", "Test 10.4"));

	/* Layered framebuffer (Test Group 11) */
	layeredFramebufferTestGroup->addChild(
		new GeometryShaderLayeredFramebufferStencil(m_context, m_extParams, "stencil_support", "Test 11.1"));
	layeredFramebufferTestGroup->addChild(
		new GeometryShaderLayeredFramebufferDepth(m_context, m_extParams, "depth_support", "Test 11.2"));
	layeredFramebufferTestGroup->addChild(
		new GeometryShaderLayeredFramebufferBlending(m_context, m_extParams, "blending_support", "Tests 11.3"));
	layeredFramebufferTestGroup->addChild(
		new GeometryShaderLayeredFramebufferClear(m_context, m_extParams, "clear_call_support", "Tests 11.4, 11.5"));

	/* Output (Test Group 12) */
	outputTestGroup->addChild(new GeometryShaderDuplicateOutputLayoutQualifierTest(
		m_context, m_extParams, "conflicted_output_primitive", "Test 12.1"));
	outputTestGroup->addChild(new GeometryShaderDuplicateMaxVerticesLayoutQualifierTest(
		m_context, m_extParams, "conflicted_output_vertices_max", "Test 12.2"));
	outputTestGroup->addChild(
		new GeometryShaderIfVertexEmitIsDoneAtEndTest(m_context, m_extParams, "vertex_emit_at_end", "Test 12.3"));
	outputTestGroup->addChild(new GeometryShaderMissingEndPrimitiveCallTest(m_context, m_extParams,
																			"primitive_end_done_at_end", "Test 12.4"));
	outputTestGroup->addChild(new GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest(
		m_context, m_extParams, "primite_end_done_for_single_primitive", "Test 12.5"));

	/* Layered rendering FBO no attachment (Test Group 13) */
	primitiveQueriesTestGroup->addChild(new GeometryShaderPrimitiveQueriesPoints(
		m_context, m_extParams, "primitive_queries_points", "Test 13.1, 13.2 - Points"));
	primitiveQueriesTestGroup->addChild(new GeometryShaderPrimitiveQueriesLines(
		m_context, m_extParams, "primitive_queries_lines", "Test 13.1, 13.2 - Lines"));
	primitiveQueriesTestGroup->addChild(new GeometryShaderPrimitiveQueriesTriangles(
		m_context, m_extParams, "primitive_queries_triangles", "Test 13.1, 13.2 - Triangles"));

	/* Layered rendering FBO no attachment (Test Group 14 & Test Group 21) */
	layeredRenderingFBONoAttachmentTestGroup->addChild(new GeometryShaderLayeredRenderingFBONoAttachment(
		m_context, m_extParams, "layered_rendering_fbo_no_attachment", "Test 14.1, 14.2, Test 21.4"));

	/* Constants Variables (Test Group 15) */
	constantVariablesTestGroup->addChild(
		new GeometryShaderConstantVariables(m_context, m_extParams, "constant_variables", "Test 15.1, Test 15.2"));

	/* Limits (Test Group 16) */
	limitsTestGroup->addChild(
		new GeometryShaderMaxUniformComponentsTest(m_context, m_extParams, "max_uniform_components", "Test 16.1"));
	limitsTestGroup->addChild(
		new GeometryShaderMaxUniformBlocksTest(m_context, m_extParams, "max_uniform_blocks", "Test 16.2"));
	limitsTestGroup->addChild(
		new GeometryShaderMaxInputComponentsTest(m_context, m_extParams, "max_input_components", "Test 16.3"));
	limitsTestGroup->addChild(
		new GeometryShaderMaxOutputComponentsTest(m_context, m_extParams, "max_output_components", "Test 16.4"));
	limitsTestGroup->addChild(
		new GeometryShaderMaxOutputVerticesTest(m_context, m_extParams, "max_output_vertices", "Test 16.5"));
	limitsTestGroup->addChild(new GeometryShaderMaxOutputComponentsSinglePointTest(
		m_context, m_extParams, "max_output_components_single_point", "Test 16.6"));
	limitsTestGroup->addChild(
		new GeometryShaderMaxTextureUnitsTest(m_context, m_extParams, "max_texture_units", "Test 16.7"));
	limitsTestGroup->addChild(
		new GeometryShaderMaxInvocationsTest(m_context, m_extParams, "max_invocations", "Test 16.8"));
	limitsTestGroup->addChild(new GeometryShaderMaxCombinedTextureUnitsTest(m_context, m_extParams,
																			"max_combined_texture_units", "Test 16.9"));

	/* Linking (Test Group 17 & Test Group 20 & part of Test Group 23 & part of Test Group 24 & part of Test Group 25) */
	linkingTestGroup->addChild(new GeometryShaderIncompleteProgramObjectsTest(
		m_context, m_extParams, "incomplete_program_objects", "Test 17.1, 17.2"));
	linkingTestGroup->addChild(
		new GeometryShaderIncompleteGSTest(m_context, m_extParams, "incomplete_gs", "Test 17.3"));
	linkingTestGroup->addChild(new GeometryShaderInvalidArrayedInputVariablesTest(
		m_context, m_extParams, "invalid_arrayed_input_variables", "Test 17.4"));

	linkingTestGroup->addChild(new GeometryShaderVSGSVariableTypeMismatchTest(
		m_context, m_extParams, "vs_gs_variable_type_mismatch", "Test 20.1"));
	linkingTestGroup->addChild(new GeometryShaderVSGSVariableQualifierMismatchTest(
		m_context, m_extParams, "vs_gs_variable_qualifier_mismatch", "Test 20.2"));
	linkingTestGroup->addChild(new GeometryShaderVSGSArrayedVariableSizeMismatchTest(
		m_context, m_extParams, "vs_gs_arrayed_variable_size_mismatch", "Test 20.3"));
	linkingTestGroup->addChild(
		new GeometryShaderFragCoordRedeclarationTest(m_context, m_extParams, "fragcoord_redeclaration", "Test 20.4"));
	linkingTestGroup->addChild(
		new GeometryShaderLocationAliasingTest(m_context, m_extParams, "location_aliasing", "Test 20.5"));

	linkingTestGroup->addChild(new GeometryShaderMoreACsInGSThanSupportedTest(
		m_context, m_extParams, "more_ACs_in_GS_than_supported", "Test 23.4"));
	linkingTestGroup->addChild(new GeometryShaderMoreACBsInGSThanSupportedTest(
		m_context, m_extParams, "more_ACBs_in_GS_than_supported", "Test 23.6"));

	linkingTestGroup->addChild(
		new GeometryShaderCompilationFailTest(m_context, m_extParams, "geometry_shader_compilation_fail", "Test 24.1"));
	linkingTestGroup->addChild(new GeometryShaderMoreInputVerticesThanAvailableTest(
		m_context, m_extParams, "more_input_vertices_in_GS_than_available", "Test 24.4"));

	linkingTestGroup->addChild(new GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest(
		m_context, m_extParams, "tf_capture_from_gs_and_vs_variables", "Test 25.1"));

	/* API interactions (Test Group 18 & Test Group 19 & part of Test Group 23  & part of Test Group 24 & part of Test Group 25) */
	apiTestGroup->addChild(
		new GeometryShaderCreateShaderProgramvTest(m_context, m_extParams, "createShaderProgramv", "Test 18.1"));
	apiTestGroup->addChild(new GeometryShaderGetShaderivTest(m_context, m_extParams, "shader_type", "Test 18.2"));
	apiTestGroup->addChild(new GeometryShaderGetProgramivTest(m_context, m_extParams, "getProgramiv", "Test 18.3"));
	apiTestGroup->addChild(new GeometryShaderGetProgramiv2Test(m_context, m_extParams, "getProgramiv2", "Test 18.4"));

	apiTestGroup->addChild(new GeometryShaderGetProgramiv3Test(m_context, m_extParams, "getProgramiv3", "Test 19.1"));
	apiTestGroup->addChild(
		new GeometryShaderDrawCallWithFSAndGS(m_context, m_extParams, "fs_gs_draw_call", "Test 19.2"));

	apiTestGroup->addChild(
		new GeometryShaderMaxImageUniformsTest(m_context, m_extParams, "max_image_uniforms", "Test 23.1"));
	apiTestGroup->addChild(
		new GeometryShaderMaxShaderStorageBlocksTest(m_context, m_extParams, "max_shader_storage_blocks", "Test 23.2"));
	apiTestGroup->addChild(
		new GeometryShaderMaxAtomicCountersTest(m_context, m_extParams, "max_atomic_counters", "Test 23.3"));
	apiTestGroup->addChild(new GeometryShaderMaxAtomicCounterBuffersTest(m_context, m_extParams,
																		 "max_atomic_counter_buffers", "Test 23.5"));

	apiTestGroup->addChild(new GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest(
		m_context, m_extParams, "pipeline_program_without_active_vs", "Test 24.2"));
	apiTestGroup->addChild(new GeometryShaderIncompatibleDrawCallModeTest(m_context, m_extParams,
																		  "incompatible_draw_call_mode", "Test 24.3"));
	apiTestGroup->addChild(new GeometryShaderInsufficientEmittedVerticesTest(
		m_context, m_extParams, "insufficient_emitted_vertices", "Test 24.5"));

	apiTestGroup->addChild(new GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest(
		m_context, m_extParams, "program_pipeline_vs_gs_capture", "Test 25.2"));

	apiTestGroup->addChild(new GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives(
		m_context, m_extParams, "draw_primitives_do_not_match_output_primitives", "Test 25.3"));
	apiTestGroup->addChild(
		new GeometryShaderDrawCallsWhileTFPaused(m_context, m_extParams, "draw_calls_while_tf_is_paused", "Test 25.4"));

	/* Layered FBOs (Test Group 21) & Test Group 26 */
	layeredFBOTestGroup->addChild(
		new GeometryShaderIncompleteLayeredFBOTest(m_context, m_extParams, "layered_fbo", "Test 21.1"));
	layeredFBOTestGroup->addChild(new GeometryShaderIncompleteLayeredAttachmentsTest(
		m_context, m_extParams, "layered_fbo_attachments", "Test 21.2"));

	layeredFBOTestGroup->addChild(new GeometryShaderFramebufferTextureInvalidTarget(
		m_context, m_extParams, "fb_texture_invalid_target", "Test 26.1"));
	layeredFBOTestGroup->addChild(new GeometryShaderFramebufferTextureNoFBOBoundToTarget(
		m_context, m_extParams, "fb_texture_no_fbo_bound_to_target", "Test 26.2"));
	layeredFBOTestGroup->addChild(new GeometryShaderFramebufferTextureInvalidAttachment(
		m_context, m_extParams, "fb_texture_invalid_attachment", "Test 26.3"));
	layeredFBOTestGroup->addChild(new GeometryShaderFramebufferTextureInvalidValue(
		m_context, m_extParams, "fb_texture_invalid_value", "Test 26.4"));
	layeredFBOTestGroup->addChild(new GeometryShaderFramebufferTextureInvalidLevelNumber(
		m_context, m_extParams, "fb_texture_invalid_level_number", "Test 26.5"));
	layeredFBOTestGroup->addChild(new GeometryShaderFramebufferTextureArgumentRefersToBufferTexture(
		m_context, m_extParams, "fb_texture_argument_refers_to_buffer_texture", "Test 26.6"));

	/* Layout qualifiers (Test Group 22) */
	qualifiersTestGroup->addChild(
		new GeometryShaderFlatInterpolationTest(m_context, m_extParams, "flat_interpolation", "Test 22.1"));
}

} // glcts
