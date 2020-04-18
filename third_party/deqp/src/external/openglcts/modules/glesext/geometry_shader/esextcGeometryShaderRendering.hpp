#ifndef _ESEXTCGEOMETRYSHADERRENDERING_HPP
#define _ESEXTCGEOMETRYSHADERRENDERING_HPP
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

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** Supported geometry shader output layout qualifiers */
typedef enum {
	/* points */
	SHADER_OUTPUT_TYPE_POINTS,
	/* lines */
	SHADER_OUTPUT_TYPE_LINE_STRIP,
	/* triangles */
	SHADER_OUTPUT_TYPE_TRIANGLE_STRIP,

	/* Always last */
	SHADER_OUTPUT_TYPE_COUNT
} _shader_output_type;

/** Implements Geometry Shader conformance test group 1.
 *
 *  Note that actual testing is handled by classes implementing GeometryShaderRenderingCase
 *  interface. This class implements DEQP CTS test case interface, meaning it is only
 *  responsible for executing the test and reporting the results back to CTS.
 *
 **/
class GeometryShaderRendering : public TestCaseGroupBase
{
public:
	/* Public methods */
	GeometryShaderRendering(Context& context, const ExtParameters& extParams, const char* name,
							const char* description);

	virtual ~GeometryShaderRendering()
	{
	}

	virtual void init(void);

private:
	/* Private type definitions */
	typedef enum {
		/* points */
		SHADER_INPUT_POINTS,

		/* lines */
		SHADER_INPUT_LINES,

		/* lines_with_adjacency */
		SHADER_INPUT_LINES_WITH_ADJACENCY,

		/* triangles */
		SHADER_INPUT_TRIANGLES,

		/* triangles_with_adjacency */
		SHADER_INPUT_TRIANGLES_WITH_ADJACENCY,

		/* Always last */
		SHADER_INPUT_UNKNOWN
	} _shader_input;

	/* Private methods */
	const char* getTestName(_shader_input input, _shader_output_type output_type, glw::GLenum drawcall_mode);
};

/* Defines an interface that all test case classes must implement.
 *
 * Base implementation initializes GLES objects later used in the specific test
 * and fills them with content, as reported by actual test case implementations.
 *
 * Instances matching this interface are used by GeometryShaderRendering class
 * to execute a set of all the tests as defined in the test specification.
 */
class GeometryShaderRenderingCase : public TestCaseBase
{
public:
	/* Public type definitions */
	/** Supported draw call types. */
	typedef enum {
		/* glDrawArrays() */
		DRAW_CALL_TYPE_GL_DRAW_ARRAYS,

		/* glDrawArraysInstanced() */
		DRAW_CALL_TYPE_GL_DRAW_ARRAYS_INSTANCED,

		/* glDrawElements() */
		DRAW_CALL_TYPE_GL_DRAW_ELEMENTS,

		/* glDrawElementsInstanced() */
		DRAW_CALL_TYPE_GL_DRAW_ELEMENTS_INSTANCED,

		/* glDrawRangeElements() */
		DRAW_CALL_TYPE_GL_DRAW_RANGE_ELEMENTS

	} _draw_call_type;

	/* Public methods */
	GeometryShaderRenderingCase(Context& Context, const ExtParameters& extParams, const char* name,
								const char* description);

	virtual ~GeometryShaderRenderingCase()
	{
	}

	virtual void deinit();
	void executeTest(_draw_call_type type);
	virtual IterateResult iterate();

protected:
	/* Protected methods */
	void				 initTest();
	virtual unsigned int getAmountOfDrawInstances()				   = 0;
	virtual unsigned int getAmountOfElementsPerInstance()		   = 0;
	virtual unsigned int getAmountOfVerticesPerInstance()		   = 0;
	virtual glw::GLenum  getDrawCallMode()						   = 0;
	virtual std::string  getFragmentShaderCode()				   = 0;
	virtual std::string  getGeometryShaderCode()				   = 0;
	virtual glw::GLuint getRawArraysDataBufferSize(bool instanced) = 0;
	virtual const void* getRawArraysDataBuffer(bool instanced)	 = 0;
	virtual void getRenderTargetSize(unsigned int n_instances, unsigned int* out_width, unsigned int* out_height) = 0;
	virtual glw::GLuint getUnorderedArraysDataBufferSize(bool instanced)   = 0;
	virtual const void* getUnorderedArraysDataBuffer(bool instanced)	   = 0;
	virtual glw::GLuint getUnorderedElementsDataBufferSize(bool instanced) = 0;
	virtual const void* getUnorderedElementsDataBuffer(bool instanced)	 = 0;
	virtual glw::GLenum  getUnorderedElementsDataType()					   = 0;
	virtual glw::GLubyte getUnorderedElementsMaxIndex()					   = 0;
	virtual glw::GLubyte getUnorderedElementsMinIndex()					   = 0;
	virtual std::string  getVertexShaderCode()							   = 0;
	virtual void verify(_draw_call_type drawcall_type, unsigned int instance_id, const unsigned char* data) = 0;

	virtual void setUniformsBeforeDrawCall(_draw_call_type /*drawcall_type*/)
	{
	}

	/* Protected variables */
	deqp::Context& m_context;
	glw::GLuint	m_instanced_raw_arrays_bo_id;
	glw::GLuint	m_instanced_unordered_arrays_bo_id;
	glw::GLuint	m_instanced_unordered_elements_bo_id;
	glw::GLuint	m_noninstanced_raw_arrays_bo_id;
	glw::GLuint	m_noninstanced_unordered_arrays_bo_id;
	glw::GLuint	m_noninstanced_unordered_elements_bo_id;
	glw::GLuint	m_fs_id;
	glw::GLuint	m_gs_id;
	glw::GLuint	m_po_id;
	glw::GLuint	m_renderingTargetSize_uniform_location;
	glw::GLuint	m_singleRenderingTargetSize_uniform_location;
	glw::GLuint	m_vao_id;
	glw::GLuint	m_vs_id;

	glw::GLuint m_fbo_id;
	glw::GLuint m_read_fbo_id;
	glw::GLuint m_to_id;

	glw::GLuint m_instanced_fbo_id;
	glw::GLuint m_instanced_read_fbo_id;
	glw::GLuint m_instanced_to_id;
};

/** Implements Geometry Shader conformance test group 1, 'points' input primitive type case.
 *  Test specification for this case follows:
 *
 *  All sub-tests assume point size & line width of 1 pixel, which is the
 *  minimum maximum value for both properties in GLES 3.0.
 *
 *  Let (R, G, B, A) be the color of the  input point that is to be amplified.
 *  Color buffer should be cleared with (0, 0, 0, 0) prior to executing each
 *  of the scenarios.
 *
 *  1.1. If "points" output primitive type is used:
 *
 *  The geometry shader should emit 9 points in total for a single input.
 *  Each point should be assigned  a size of 1. The points should be
 *  positioned, so that they tightly surround the "input" point from left /
 *  right / top / left sides, as well as from top-left / top-right /
 *  bottom-left / bottom-right corners but do *not* overlap in screen-space.
 *
 *  Additional points should also use (R, G, B, A) color.
 *
 *  The test should draw 8 points (one after another, assume a horizontal
 *  delta of 2 pixels) of varying colors to a 2D texture of resolution 38x3.
 *  Test succeeds if centers of all emitted points have colors different than
 *  the background color.
 *
 *  1.2. If "lines" output primitive type is used:
 *
 *  The geometry shader should draw outlines (built of line segments) of
 *  three quads nested within each other, as depicted below:
 *
 *                           1 1 1 1 1 1 1
 *                           1 2 2 2 2 2 1
 *                           1 2 3 3 3 2 1
 *                           1 2 3 * 3 2 1
 *                           1 2 3 3 3 2 1
 *                           1 2 2 2 2 2 1
 *                           1 1 1 1 1 1 1
 *
 *  where each number corresponds to index of the quad and * indicates
 *  position of the input point which is not drawn.
 *
 *  Each quad should be drawn with (R, G, B, A) color.
 *  The test should draw 8 points (one after another) of varying colors to
 *  a 2D texture of resolution 54x7. Test succeeds if all pixels making up
 *  the central quad 2 have valid colors.
 *
 *  1.3. If "triangles" output primitive type is used:
 *
 *  The geometry shader should generate 2 triangle primitives for a single
 *  input point:
 *
 *  * A) (Bottom-left corner, top-left corner, bottom-right corner), use
 *       (R, 0, 0, 0) color;
 *  * B) (Bottom-right corner, top-left corner, top-right corner), use
 *       (0, G, 0, 0) color;
 *
 *  The test should draw 8 points (one after another) of varying colors to
 *  a 2D texture of resolution of resolution 48x6. Test succeeds if centers
 *  of the rendered triangles have valid colors.
 *
 **/
class GeometryShaderRenderingPointsCase : public GeometryShaderRenderingCase
{
public:
	/* Public methods */
	GeometryShaderRenderingPointsCase(Context& context, const ExtParameters& extParams, const char* name,
									  glw::GLenum drawcall_mode, _shader_output_type output_type);

	virtual ~GeometryShaderRenderingPointsCase();

protected:
	/* GeometryShaderRenderingCase interface implementation */
	unsigned int getAmountOfDrawInstances();
	unsigned int getAmountOfElementsPerInstance();
	unsigned int getAmountOfVerticesPerInstance();
	glw::GLenum  getDrawCallMode();
	std::string  getFragmentShaderCode();
	std::string  getGeometryShaderCode();
	glw::GLuint getRawArraysDataBufferSize(bool instanced);
	const void* getRawArraysDataBuffer(bool instanced);
	void getRenderTargetSize(unsigned int n_instances, unsigned int* out_width, unsigned int* out_height);
	glw::GLuint getUnorderedArraysDataBufferSize(bool instanced);
	const void* getUnorderedArraysDataBuffer(bool instanced);
	glw::GLuint getUnorderedElementsDataBufferSize(bool instanced);
	const void* getUnorderedElementsDataBuffer(bool instanced);
	glw::GLenum  getUnorderedElementsDataType();
	glw::GLubyte getUnorderedElementsMaxIndex();
	glw::GLubyte getUnorderedElementsMinIndex();
	std::string  getVertexShaderCode();
	void verify(_draw_call_type drawcall_type, unsigned int instance_id, const unsigned char* data);

private:
	/* Private variables */
	_shader_output_type m_output_type;

	float*		   m_raw_array_data;
	float*		   m_unordered_array_data;
	unsigned char* m_unordered_elements_data;
};

/** Implements Geometry Shader conformance test group 1, 'lines' and
 *  'lines_adjacency' input primitive type cases.
 *
 *  Test specification for this case follows:
 *
 *  Assume a point size of 1 pixel and line width of 1 pixel, where
 *  appropriate.
 *  Let (R, G, B, A) be the color of start point of the input line and
 *  (R', G', B', A') be the color of end point of the input line.
 *
 *  2.1. If "points" output primitive type is used:
 *
 *  The geometry shader should generate 8 points for a single input line segment,
 *  where each point consists of 9 sub-points tightly forming a quad (with
 *  the actual point located in the center) in order to emulate larger point
 *  size. The points should be uniformly distributed over the primitive
 *  (first point positioned at the start point and the last one located at
 *  the end point) and their color should linearly interpolate from the
 *  (R, G, B, A) to (R', G', B', A'). All sub-points should use the same
 *  color as the parent point.
 *
 *  The test should draw the points over a square outline. Each instance should
 *  draw a set of points occupying a separate outline. Each rectangle should
 *  occupy a block of 45x45.
 *
 *  Test succeeds if centers of generated points have valid colors.
 *
 *  2.2. If "lines" output primitive type is used:
 *
 *  Expanding on the idea presenting in 2.1, for each line segment the GS
 *  should generate three line segments, as presented below:
 *
 *                       Upper/left helper line segment
 *                       Line segment
 *                       Bottom/right helper line segment
 *
 *  This is to emulate a larger line width than the minimum maximum line
 *  width all GLES implementations must support.
 *
 *  Upper helper line segment should use start point's color;
 *  Middle line segment should take mix(start_color, end_color, 0.5) color;
 *  Bottom helper line segment should use end point's color;
 *
 *  Test succeeds if all pixels of generated middle line segments have valid
 *  colors. Do not test corners.
 *
 *  2.3. If "triangles" output primitive type is used:
 *
 *  Expanding on the idea presented in 2.1: for each input line segment,
 *  the GS should generate a triangle, using two vertices provided and
 *  (0, 0, 0, 1). By drawing a quad outline, whole screen-space should be
 *  covered with four triangles.
 *  The test passes if centroids of the generated triangles carry valid colors.
 *
 **/
class GeometryShaderRenderingLinesCase : public GeometryShaderRenderingCase
{
public:
	/* Public methods */
	GeometryShaderRenderingLinesCase(Context& context, const ExtParameters& extParams, const char* name,
									 bool use_adjacency_data, glw::GLenum drawcall_mode,
									 _shader_output_type output_type);

	virtual ~GeometryShaderRenderingLinesCase();

protected:
	/* GeometryShaderRenderingCase interface implementation */
	unsigned int getAmountOfDrawInstances();
	unsigned int getAmountOfElementsPerInstance();
	unsigned int getAmountOfVerticesPerInstance();
	glw::GLenum  getDrawCallMode();
	std::string  getFragmentShaderCode();
	std::string  getGeometryShaderCode();
	glw::GLuint getRawArraysDataBufferSize(bool instanced);
	const void* getRawArraysDataBuffer(bool instanced);
	void getRenderTargetSize(unsigned int n_instances, unsigned int* out_width, unsigned int* out_height);
	glw::GLuint getUnorderedArraysDataBufferSize(bool instanced);
	const void* getUnorderedArraysDataBuffer(bool instanced);
	glw::GLuint getUnorderedElementsDataBufferSize(bool instanced);
	const void* getUnorderedElementsDataBuffer(bool instanced);
	glw::GLenum  getUnorderedElementsDataType();
	glw::GLubyte getUnorderedElementsMaxIndex();
	glw::GLubyte getUnorderedElementsMinIndex();
	std::string  getVertexShaderCode();
	void setUniformsBeforeDrawCall(_draw_call_type drawcall_type);
	void verify(_draw_call_type drawcall_type, unsigned int instance_id, const unsigned char* data);

private:
	/* Private variables */
	_shader_output_type m_output_type;

	glw::GLenum m_drawcall_mode;
	bool		m_use_adjacency_data;

	float*		 m_raw_array_instanced_data;
	unsigned int m_raw_array_instanced_data_size;
	float*		 m_raw_array_noninstanced_data;
	unsigned int m_raw_array_noninstanced_data_size;

	float*		 m_unordered_array_instanced_data;
	unsigned int m_unordered_array_instanced_data_size;
	float*		 m_unordered_array_noninstanced_data;
	unsigned int m_unordered_array_noninstanced_data_size;

	unsigned char* m_unordered_elements_instanced_data;
	unsigned int   m_unordered_elements_instanced_data_size;
	unsigned char* m_unordered_elements_noninstanced_data;
	unsigned int   m_unordered_elements_noninstanced_data_size;

	unsigned char m_unordered_elements_max_index;
	unsigned char m_unordered_elements_min_index;
};

/** Implements Geometry Shader conformance test group 1, 'triangles' and
 *  'triangles_adjacency' input primitive type cases. Test specification
 *  for this case follows:
 *
 *  All tests should draw a 45-degree rotated square shape consisting of four
 *  separate triangles, as depicted in the picture below:
 *
 *
 *                                 C
 *                                / \
     *                               /   \
     *                              B--A--D
 *                               \   /
 *                                \_/
 *                                 E
 *
 *  For GL_TRIANGLES data, the rendering order is: ABC, ACD, ADE, AEB;
 *  For GL_TRIANGLE_FAN,   the rendering order is: ABCDEB;
 *  For GL_TRIANGLE_STRIP, the rendering order is: BACDAEB;
 *
 *  Note that for triangle strips, a degenerate triangle will be rendered.
 *  Test implementation should not test the first triangle rendered in the
 *  top-right quarter, as it will be overwritten by the triangle that follows
 *  right after.
 *
 *  Subsequent draw call instances should draw the geometry one after another,
 *  in vertical direction.
 *
 *  Each of the tests should use 29x(29 * number of instances) resolution for
 *  the rendertarget.
 *
 *  3.1. If "points" output primitive type is used:
 *
 *  The geometry shader should generate 3 points for a single input triangle.
 *  These points should be emitted for each of the triangle's vertex
 *  locations and:
 *
 *  * First vertex should be of (R, G, B, A) color;
 *  * Second vertex should be of (R', G', B', A') color;
 *  * Third vertex should be of (R'', G'', B'', A'') color;
 *
 *  Each point should actually consist of 9 emitted points of size 1 (as
 *  described in test scenario 1.1), with the middle point being positioned
 *  at exact vertex location. This is to emulate larger point size than the
 *  minimum maximum allows. All emitted points should use the same color as
 *  parent point's.
 *
 *  Test succeeds if centers of the rendered points have valid colors.
 *
 *  3.2. If "lines" output primitive type is used:
 *
 *  Let:
 *
 *  * TL represent top-left corner of the triangle's bounding box;
 *  * TR represent top-right corner of the triangle's bounding box;
 *  * BL represent bottom-left corner of the triangle's bounding box;
 *  * BR represent bottom-right corner of the triangle's bounding box;
 *
 *  The geometry shader should draw 4 line segments for a single input
 *  triangle:
 *
 *  * First line segment should start at BL and end at TL and use a static
 *    (R, G, B, A) color;
 *  * Second line segment should start at TL and end at TR and use a static
 *    (R', G', B', A') color;
 *  * Third line segment should start at TR and end at BR and use a static
 *    (R'', G'', B'', A'') color;
 *  * Fourth line segment should start at BR and end at BL and use a static
 *    (R, G', B'', A) color;
 *
 *  Each line segment should actually consist of 3 separate line segments
 *  "stacked" on top of each other, with the middle segment being positioned
 *  as described above (as described in test scenario 2.2). This is to
 *  emulate line width that is larger than the minimum maximum allows. All
 *  emitted line segments should use the same color as parent line segment's.
 *
 *  Test succeeds if centers of the rendered line segments have valid colors.
 *
 *  3.3. If "triangles" output primitive type is used:
 *
 *  Test should take incoming triangle vertex locations and order them in the
 *  following order:
 *
 *  a) A - vertex in the origin;
 *  b) B - the other vertex located on the same height as A;
 *  c) C - remaining vertex;
 *
 *  Let D = BC/2.
 *
 *  The test should emit ABD and ACD triangles for input ABC triangle data.
 *  First triangle emitted should take color of the first input vertex
 *  (not necessarily A!).
 *  The second one should use third vertex's color (not necessarily C!),
 *  with the first channel being multiplied by two.
 *
 *  Test succeeds if centers of the rendered triangles have valid colors;
 *
 **/
class GeometryShaderRenderingTrianglesCase : public GeometryShaderRenderingCase
{
public:
	/* Public methods */
	GeometryShaderRenderingTrianglesCase(Context& context, const ExtParameters& extParams, const char* name,
										 bool use_adjacency_data, glw::GLenum drawcall_mode,
										 _shader_output_type output_type);

	virtual ~GeometryShaderRenderingTrianglesCase();

protected:
	/* GeometryShaderRenderingCase interface implementation */
	unsigned int getAmountOfDrawInstances();
	unsigned int getAmountOfElementsPerInstance();
	unsigned int getAmountOfVerticesPerInstance();
	glw::GLenum  getDrawCallMode();
	std::string  getFragmentShaderCode();
	std::string  getGeometryShaderCode();
	glw::GLuint getRawArraysDataBufferSize(bool instanced);
	const void* getRawArraysDataBuffer(bool instanced);
	void getRenderTargetSize(unsigned int n_instances, unsigned int* out_width, unsigned int* out_height);
	glw::GLuint getUnorderedArraysDataBufferSize(bool instanced);
	const void* getUnorderedArraysDataBuffer(bool instanced);
	glw::GLuint getUnorderedElementsDataBufferSize(bool instanced);
	const void* getUnorderedElementsDataBuffer(bool instanced);
	glw::GLenum  getUnorderedElementsDataType();
	glw::GLubyte getUnorderedElementsMaxIndex();
	glw::GLubyte getUnorderedElementsMinIndex();
	std::string  getVertexShaderCode();
	void setUniformsBeforeDrawCall(_draw_call_type drawcall_type);
	void verify(_draw_call_type drawcall_type, unsigned int instance_id, const unsigned char* data);

private:
	/* Private variables */
	_shader_output_type m_output_type;

	glw::GLenum m_drawcall_mode;
	bool		m_use_adjacency_data;

	float*		 m_raw_array_instanced_data;
	unsigned int m_raw_array_instanced_data_size;
	float*		 m_raw_array_noninstanced_data;
	unsigned int m_raw_array_noninstanced_data_size;

	float*		 m_unordered_array_instanced_data;
	unsigned int m_unordered_array_instanced_data_size;
	float*		 m_unordered_array_noninstanced_data;
	unsigned int m_unordered_array_noninstanced_data_size;

	unsigned char* m_unordered_elements_instanced_data;
	unsigned int   m_unordered_elements_instanced_data_size;
	unsigned char* m_unordered_elements_noninstanced_data;
	unsigned int   m_unordered_elements_noninstanced_data_size;

	unsigned char m_unordered_elements_max_index;
	unsigned char m_unordered_elements_min_index;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERRENDERING_HPP
