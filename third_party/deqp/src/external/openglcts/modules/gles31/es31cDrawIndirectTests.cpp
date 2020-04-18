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

#include "es31cDrawIndirectTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"

#include <map>

namespace glcts
{
using namespace glw;
namespace
{

class DILogger
{
public:
	DILogger() : null_log_(0)
	{
	}

	DILogger(const DILogger& rhs)
	{
		null_log_ = rhs.null_log_;
		if (!null_log_)
		{
			str_ << rhs.str_.str();
		}
	}

	~DILogger()
	{
		s_tcuLog->writeMessage(str_.str().c_str());
		if (!str_.str().empty())
		{
			s_tcuLog->writeMessage(NL);
		}
	}

	template <class T>
	DILogger& operator<<(const T& t)
	{
		if (!null_log_)
		{
			str_ << t;
		}
		return *this;
	}

	DILogger& nullify()
	{
		null_log_ = true;
		return *this;
	}

	static void setOutput(tcu::TestLog& log)
	{
		s_tcuLog = &log;
	}

private:
	void				 operator=(const DILogger&);
	bool				 null_log_;
	std::ostringstream   str_;
	static tcu::TestLog* s_tcuLog;
};
tcu::TestLog* DILogger::s_tcuLog = NULL;

class DIResult
{
public:
	DIResult() : status_(NO_ERROR)
	{
	}

	DILogger error()
	{
		return sub_result(ERROR);
	}
	long code() const
	{
		return status_;
	}
	DILogger sub_result(long _code)
	{
		if (_code == NO_ERROR)
		{
			return sub_result_inner(_code).nullify();
		}
		else
		{
			return sub_result_inner(_code);
		}
	}

private:
	DILogger sub_result_inner(long _code)
	{
		status_ |= _code;
		return DILogger();
	}
	DILogger logger_;
	long	 status_;
};

namespace test_api
{
struct ES3
{
	static bool isES()
	{
		return true;
	}
	static std::string glslVer(bool = false)
	{
		return "#version 310 es";
	}
	static void ES_Only()
	{
	}
};

struct GL
{
	static bool isES()
	{
		return false;
	}
	static std::string glslVer(bool compute = false)
	{
		if (compute)
		{
			return "#version 430";
		}
		else
		{
			return "#version 400";
		}
	}
	static void GL_Only()
	{
	}
};
}

namespace shaders
{

template <typename api>
std::string vshSimple()
{
	return api::glslVer() + NL "in vec4 i_vertex;" NL "void main()" NL "{" NL "    gl_Position = i_vertex;" NL "}";
}
template <typename api>
std::string vshSimple_point()
{
	return api::glslVer() + NL "in vec4 i_vertex;" NL "void main()" NL "{" NL "    gl_Position = i_vertex;" NL
							   "#if defined(GL_ES)" NL "    gl_PointSize = 1.0;" NL "#endif" NL "}";
}

template <typename api>
std::string fshSimple()
{
	return api::glslVer() + NL "precision highp float; " NL "out vec4 outColor;" NL "void main() {" NL
							   "  outColor = vec4(0.1,0.2,0.3,1.0);" NL "}";
}
}

class DrawIndirectBase : public glcts::SubcaseBase
{
protected:
	typedef std::vector<unsigned int> CDataArray;
	typedef std::vector<tcu::Vec3>	CVertexArray;
	typedef std::vector<tcu::Vec4>	CColorArray;
	typedef std::vector<GLuint>		  CElementArray;

	enum TDrawFunction
	{
		DRAW_ARRAYS,
		DRAW_ELEMENTS,
	};

	typedef struct
	{
		GLuint count;
		GLuint primCount;
		GLuint first;
		GLuint reservedMustBeZero;
	} DrawArraysIndirectCommand;

	typedef struct
	{
		GLuint count;
		GLuint primCount;
		GLuint firstIndex;
		GLint  baseVertex;
		GLuint reservedMustBeZero;
	} DrawElementsIndirectCommand;

	int getWindowWidth()
	{
		return m_context.getRenderContext().getRenderTarget().getWidth();
	}

	int getWindowHeight()
	{
		return m_context.getRenderContext().getRenderTarget().getHeight();
	}

	void getDataSize(int& width, int& height)
	{
		width  = std::min(getWindowWidth(), 16384);				 // Cap width to 16384
		height = std::min(getWindowHeight(), 4 * 16384 / width); // Height is 4 if width is capped
	}

	GLuint CreateComputeProgram(const std::string& cs, bool linkAndCheck)
	{
		const GLuint p = glCreateProgram();

		const GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
		glAttachShader(p, sh);
		glDeleteShader(sh);
		const char* const src[1] = { cs.c_str() };
		glShaderSource(sh, 1, src, NULL);
		glCompileShader(sh);

		if (linkAndCheck)
		{
			glLinkProgram(p);
			if (!CheckProgram(p))
			{
				return 0;
			}
		}

		return p;
	}

	GLuint CreateProgram(const std::string& vs, const std::string& gs, const std::string& fs, bool linkAndCheck)
	{
		const GLuint p = glCreateProgram();

		if (!vs.empty())
		{
			const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[1] = { vs.c_str() };
			glShaderSource(sh, 1, src, NULL);
			glCompileShader(sh);
		}
		if (!gs.empty())
		{
			const GLuint sh = glCreateShader(GL_GEOMETRY_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[1] = { gs.c_str() };
			glShaderSource(sh, 1, src, NULL);
			glCompileShader(sh);
		}
		if (!fs.empty())
		{
			const GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[1] = { fs.c_str() };
			glShaderSource(sh, 1, src, NULL);
			glCompileShader(sh);
		}

		if (linkAndCheck)
		{
			glLinkProgram(p);
			if (!CheckProgram(p))
			{
				return 0;
			}
		}

		return p;
	}

	long CheckProgram(GLuint program)
	{
		DIResult status;
		GLint	progStatus;
		glGetProgramiv(program, GL_LINK_STATUS, &progStatus);

		if (progStatus == GL_FALSE)
		{

			status.error() << "GL_LINK_STATUS is false";

			GLint attached_shaders;
			glGetProgramiv(program, GL_ATTACHED_SHADERS, &attached_shaders);

			if (attached_shaders > 0)
			{
				std::vector<GLuint> shaders(attached_shaders);
				glGetAttachedShaders(program, attached_shaders, NULL, &shaders[0]);

				for (GLint i = 0; i < attached_shaders; ++i)
				{
					// shader type
					GLenum type;
					glGetShaderiv(shaders[i], GL_SHADER_TYPE, reinterpret_cast<GLint*>(&type));
					switch (type)
					{
					case GL_VERTEX_SHADER:
						status.error() << "*** Vertex Shader ***\n";
						break;
					case GL_FRAGMENT_SHADER:
						status.error() << "*** Fragment Shader ***\n";
						break;
					case GL_COMPUTE_SHADER:
						status.error() << "*** Compute Shader ***\n";
						break;
					default:
						status.error() << "*** Unknown Shader ***\n";
						break;
					}

					// shader source
					GLint length;
					glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &length);
					if (length > 0)
					{
						std::vector<GLchar> source(length);
						glGetShaderSource(shaders[i], length, NULL, &source[0]);
						status.error() << source[0];
					}

					// shader info log
					glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &length);
					if (length > 0)
					{
						std::vector<GLchar> log(length);
						glGetShaderInfoLog(shaders[i], length, NULL, &log[0]);
						status.error() << &log[0];
					}
				}
			}

			// program info log
			GLint length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			if (length > 0)
			{
				std::vector<GLchar> log(length);
				glGetProgramInfoLog(program, length, NULL, &log[0]);
				status.error() << &log[0];
			}
		}

		return status.code() == NO_ERROR;
	}

	template <typename api>
	void ReadPixelsFloat(int x, int y, int width, int height, void* data);

	template <typename api>
	void GetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data);

	template <typename T>
	void DataGen(std::vector<T>& data, unsigned int sizeX, unsigned int sizeY, T valueMin, T valueMax)
	{
		data.resize(sizeX * sizeY, 0);
		T range = valueMax - valueMin;
		T stepX = range / sizeX;
		T stepY = range / sizeY;

		for (unsigned int i = 0; i < sizeY; ++i)
		{
			T valueY = i * stepY;

			for (unsigned int j = 0; j < sizeX; ++j)
			{
				data[j + i * sizeX] = valueMin + j * stepX + valueY;
			}
		}
	}

	template <typename T>
	long DataCompare(const std::vector<T>& dataRef, unsigned int widthRef, unsigned int heightRef,
					 const std::vector<T>& dataTest, unsigned int widthTest, unsigned int heightTest,
					 unsigned offsetYRef = 0, unsigned offsetYTest = 0)
	{
		if (widthRef * heightRef > dataRef.size())
			throw std::runtime_error("Invalid reference buffer resolution!");

		if (widthTest * heightTest > dataTest.size())
			throw std::runtime_error("Invalid test buffer resolution!");

		unsigned int width  = std::min(widthRef, widthTest);
		unsigned int height = std::min(heightRef, heightTest);

		for (unsigned int i = 0; i < height; ++i)
		{
			unsigned int offsetRef  = (i + offsetYRef) * widthRef;
			unsigned int offsetTest = (i + offsetYTest) * widthTest;

			for (size_t j = 0; j < width; ++j)
			{
				if (dataRef[offsetRef + j] != dataTest[offsetTest + j])
				{
					DIResult status;
					status.error() << "Compare failed: different values [x: " << j << ", y: " << i + offsetYTest
								   << ", reference: " << dataRef[offsetRef + j]
								   << ", test: " << dataTest[offsetTest + j] << "]";
					return status.code();
				}
			}
		}

		return NO_ERROR;
	}

	template <typename api>
	long BindingPointCheck(GLuint expectedValue)
	{
		DIResult status;

		GLint valueInt = -9999;
		glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &valueInt);
		if (valueInt != static_cast<GLint>(expectedValue))
		{
			status.error() << "glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING) returned invalid value: " << valueInt
						   << ", expected: " << expectedValue;
		}

		GLboolean valueBool = expectedValue ? GL_FALSE : GL_TRUE;
		glGetBooleanv(GL_DRAW_INDIRECT_BUFFER_BINDING, &valueBool);
		if (valueBool != (expectedValue ? GL_TRUE : GL_FALSE))
		{
			status.error() << "glGetBooleanv(GL_DRAW_INDIRECT_BUFFER_BINDING) returned invalid value: "
						   << BoolToString(valueBool)
						   << ", expected: " << BoolToString(expectedValue ? GL_TRUE : GL_FALSE);
		}

		GLfloat valueFloat		   = -9999;
		GLfloat expectedFloatValue = static_cast<GLfloat>(expectedValue);
		glGetFloatv(GL_DRAW_INDIRECT_BUFFER_BINDING, &valueFloat);
		if (valueFloat != expectedFloatValue)
		{
			status.error() << "glGetFloatv(GL_DRAW_INDIRECT_BUFFER_BINDING) returned invalid value: " << valueFloat
						   << ", expected: " << expectedValue;
		}

		if (!api::isES())
		{
			GLdouble valueDouble = -9999;
			glGetDoublev(GL_DRAW_INDIRECT_BUFFER_BINDING, &valueDouble);
			if (valueDouble != static_cast<GLdouble>(expectedValue))
			{
				status.error() << "glGetDoublev(GL_DRAW_INDIRECT_BUFFER_BINDING) returned invalid value: "
							   << valueDouble << ", expected: " << expectedValue;
			}
		}

		return status.code();
	}

	template <typename T>
	long BuffersCompare(const std::vector<T>& bufferTest, unsigned int widthTest, unsigned int heightTest,
						const std::vector<T>& bufferRef, unsigned int widthRef, unsigned int heightRef)
	{

		const tcu::PixelFormat& pixelFormat = m_context.getRenderContext().getRenderTarget().getPixelFormat();
		tcu::Vec4				epsilon		= tcu::Vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		double stepX = widthRef / static_cast<double>(widthTest);
		double stepY = heightRef / static_cast<double>(heightTest);
		for (unsigned int i = 0; i < heightTest; ++i)
		{
			unsigned int offsetTest = i * widthTest;
			unsigned int offsetRef  = static_cast<int>(i * stepY + 0.5) * widthRef;
			for (unsigned int j = 0; j < widthTest; ++j)
			{
				unsigned int posXRef = static_cast<int>(j * stepX + 0.5);
				if (!ColorVerify(bufferTest[j + offsetTest], bufferRef[posXRef + offsetRef], epsilon))
				{
					DIResult status;
					status.error() << "(x,y)= (" << j << "," << i << "). Color RGBA(" << bufferTest[j + offsetTest][0]
								   << "," << bufferTest[j + offsetTest][1] << "," << bufferTest[j + offsetTest][2]
								   << "," << bufferTest[j + offsetTest][3] << ") is different than expected RGBA("
								   << bufferRef[posXRef + offsetRef][0] << "," << bufferRef[posXRef + offsetRef][1]
								   << "," << bufferRef[posXRef + offsetRef][2] << ","
								   << bufferRef[posXRef + offsetRef][3] << ")";
					return status.code();
				}
			}
		}
		return NO_ERROR;
	}

	template <typename T>
	bool ColorVerify(T color, T colorExpected, tcu::Vec4 epsilon)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (fabsf(colorExpected[i] - color[i]) > epsilon[i])
				return false;
		}
		return true;
	}

	long BufferCheck(const CDataArray& dataRef, unsigned int widthRef, unsigned int heightRef, const void* bufTest,
					 unsigned int widthTest, unsigned int heightTest, unsigned int offsetYRef = 0,
					 unsigned int offsetYTest = 0)
	{
		if (bufTest == 0)
		{
			throw std::runtime_error("Invalid test buffer!");
		}

		CDataArray dataTest(widthTest * heightTest, 0);
		memcpy(&dataTest[0], bufTest, widthTest * heightTest * sizeof(unsigned int));

		return DataCompare(dataRef, widthRef, heightRef, dataTest, widthTest, heightTest, offsetYRef, offsetYTest);
	}

	template <typename api>
	long StateValidate(GLboolean mapped, GLbitfield access, GLbitfield accessFlag, GLintptr offset, GLsizeiptr length)
	{
		DIResult result;

		if (!api::isES())
		{
			int v;
			glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_ACCESS, &v);
			if (v != static_cast<int>(access))
			{
				result.error() << "glGetBufferParameteriv(GL_BUFFER_ACCESS) returned incorrect state: "
							   << AccessToString(v) << ", expected: " << AccessToString(access);
			}
		}

		int v;
		glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_ACCESS_FLAGS, &v);
		if (v != static_cast<int>(accessFlag))
		{
			result.error() << "glGetBufferParameteriv(GL_BUFFER_ACCESS_FLAGS) returned incorrect state: " << v
						   << ", expected: " << accessFlag;
		}

		glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAPPED, &v);
		if (v != mapped)
		{
			result.error() << "glGetBufferParameteriv(GL_BUFFER_MAPPED) returned incorrect state: "
						   << BoolToString((GLboolean)v) << ", expected: " << BoolToString((GLboolean)access);
		}

		glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_OFFSET, &v);
		if (v != offset)
		{
			result.error() << "glGetBufferParameteriv(GL_BUFFER_MAP_OFFSET) returned incorrect offset: " << v
						   << ", expected: " << offset;
		}

		glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_LENGTH, &v);
		if (v != length)
		{
			result.error() << "glGetBufferParameteriv(GL_BUFFER_MAP_LENGTH) returned incorrect length: " << v
						   << ", expected: " << length;
		}

		return result.code();
	}

	void PointsGen(unsigned int drawSizeX, unsigned int drawSizeY, CColorArray& output)
	{
		output.reserve(drawSizeY * 2);
		float rasterSizeX = 2.0f / static_cast<float>(getWindowWidth());
		float rasterSizeY = 2.0f / static_cast<float>(getWindowHeight());
		for (unsigned int i = 0; i < drawSizeY; ++i)
		{
			float offsetY = -1.0f + rasterSizeY * static_cast<float>(i) + rasterSizeY / 2;
			for (unsigned int j = 0; j < drawSizeX; ++j)
			{
				float offsetX = -1.0f + rasterSizeX * static_cast<float>(j) + rasterSizeX / 2;
				output.push_back(tcu::Vec4(offsetX, offsetY, 0.0f, 1.0f));
			}
		}
	}

	float LinesOffsetY(unsigned int i, float rasterSize)
	{
		// Offset lines slightly from the center of pixels so as not to hit rasterizer
		// tie-break conditions (the right-edge of the screen at half-integer pixel
		// heights is the right corner of a diamond). rasterSize/16 is the smallest
		// offset that the spec guarantees the rasterizer can resolve.
		return -1.0f + rasterSize * static_cast<float>(i) + rasterSize / 2 + rasterSize / 16;
	}

	void LinesGen(unsigned int, unsigned int drawSizeY, CColorArray& output)
	{
		output.reserve(drawSizeY * 2);
		float rasterSize = 2.0f / static_cast<float>(getWindowHeight());
		for (unsigned int i = 0; i < drawSizeY; ++i)
		{
			float offsetY = LinesOffsetY(i, rasterSize);
			output.push_back(tcu::Vec4(-1.0f, offsetY, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(1.0f, offsetY, 0.0f, 1.0f));
		}
	}

	void LinesAdjacencyGen(unsigned int, unsigned int drawSizeY, CColorArray& output)
	{
		float rasterSize = 2.0f / static_cast<float>(getWindowHeight());
		for (unsigned int i = 0; i < drawSizeY; ++i)
		{
			float offsetY = LinesOffsetY(i, rasterSize);
			output.push_back(tcu::Vec4(-1.5f, -1.0f + offsetY, 0.0f, 1.0f)); //adj
			output.push_back(tcu::Vec4(-1.0f, offsetY, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(1.0f, offsetY, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(1.5f, -1.0f + offsetY, 0.0f, 1.0f)); //adj
		}
	}

	void LineStripAdjacencyGen(unsigned int, unsigned int drawSizeY, CColorArray& output)
	{
		float rasterSize = 2.0f / static_cast<float>(getWindowHeight());
		output.push_back(tcu::Vec4(-1.5f, rasterSize / 2, 0.0f, 1.0f));
		for (unsigned int i = 0; i < drawSizeY; ++i)
		{
			float offsetY = LinesOffsetY(i, rasterSize);
			output.push_back(tcu::Vec4(-1.0f, offsetY, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(-1.0f, offsetY, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(1.0f, offsetY, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(1.0f, offsetY, 0.0f, 1.0f));
		}
		output.push_back(tcu::Vec4(1.5f, 1.0f - rasterSize / 2, 0.0f, 1.0f));
	}

	void TrianglesGen(unsigned int drawSizeX, unsigned int drawSizeY, CColorArray& output)
	{
		output.reserve(drawSizeX * 2 * 6);

		switch (drawSizeX)
		{
		case 1:
		{
			output.push_back(tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(4.0f, -1.0f, 0.0f, 1.0f));
			output.push_back(tcu::Vec4(-1.0f, 4.0f, 0.0f, 1.0f));
		}
		break;
		case 0:
		{
			throw std::runtime_error("Invalid drawSizeX!");
		}
		break;
		default:
		{
			float drawStepX = 2.0f / static_cast<float>(drawSizeX);
			float drawStepY = 2.0f / static_cast<float>(drawSizeY);

			for (unsigned int i = 0; i < drawSizeY; ++i)
			{
				float offsetY = -1.0f + drawStepY * static_cast<float>(i);
				for (unsigned int j = 0; j < drawSizeX; ++j)
				{
					float offsetX = -1.0f + drawStepX * static_cast<float>(j);

					output.push_back(tcu::Vec4(offsetX, offsetY, 0.0f, 1.0f));
					output.push_back(tcu::Vec4(offsetX + drawStepX, offsetY, 0.0f, 1.0f));
					output.push_back(tcu::Vec4(offsetX, offsetY + drawStepY, 0.0f, 1.0f));

					output.push_back(tcu::Vec4(offsetX + drawStepX, offsetY, 0.0f, 1.0f));
					output.push_back(tcu::Vec4(offsetX + drawStepX, offsetY + drawStepY, 0.0f, 1.0f));
					output.push_back(tcu::Vec4(offsetX, offsetY + drawStepY, 0.0f, 1.0f));
				}
			}
		}
		break;
		}
	}

	void TrianglesAdjacencyGen(unsigned int drawSizeX, unsigned int drawSizeY, CColorArray& output)
	{
		float sizeX = 1.0f / static_cast<float>(drawSizeX);
		float sizeY = 1.0f / static_cast<float>(drawSizeY);

		for (unsigned int i = 0; i < drawSizeX; ++i)
		{
			float offsetY = -0.5f + sizeY * static_cast<float>(i);
			for (unsigned int j = 0; j < drawSizeY; ++j)
			{
				float offsetX = -0.5f + sizeX * static_cast<float>(j);

				output.push_back(tcu::Vec4(offsetX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX - sizeX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY - sizeY, 0.0f, 1.0f));

				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + 2 * sizeX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX, offsetY + 2 * sizeY, 0.0f, 1.0f));
			}
		}
	}

	void TriangleStripAdjacencyGen(unsigned int drawSizeX, unsigned int drawSizeY, CColorArray& output)
	{
		float sizeX = 1.0f / static_cast<float>(drawSizeX);
		float sizeY = 1.0f / static_cast<float>(drawSizeY);

		for (unsigned int i = 0; i < drawSizeX; ++i)
		{
			float offsetY = -0.5f + sizeY * static_cast<float>(i);
			for (unsigned int j = 0; j < drawSizeY; ++j)
			{
				float offsetX = -0.5f + sizeX * static_cast<float>(j);

				output.push_back(tcu::Vec4(offsetX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX - sizeX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY - sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX, offsetY + 2 * sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + sizeX, offsetY + sizeY, 0.0f, 1.0f));
				output.push_back(tcu::Vec4(offsetX + 2 * sizeX, offsetY - sizeY, 0.0f, 1.0f));
			}
		}
	}

	void PrimitiveGen(GLenum primitiveType, unsigned int drawSizeX, unsigned int drawSizeY, CColorArray& output)
	{
		switch (primitiveType)
		{
		case GL_POINTS:
			PointsGen(drawSizeX, drawSizeY, output);
			break;
		case GL_LINES:
		case GL_LINE_STRIP:
		case GL_LINE_LOOP:
			LinesGen(drawSizeX, drawSizeY, output);
			break;
		case GL_LINES_ADJACENCY:
			LinesAdjacencyGen(drawSizeX, drawSizeY, output);
			break;
		case GL_LINE_STRIP_ADJACENCY:
			LineStripAdjacencyGen(drawSizeX, drawSizeY, output);
			break;
		case GL_TRIANGLES:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLE_FAN:
			TrianglesGen(drawSizeX, drawSizeY, output);
			break;
		case GL_TRIANGLES_ADJACENCY:
			TrianglesAdjacencyGen(drawSizeX, drawSizeY, output);
			break;
		case GL_TRIANGLE_STRIP_ADJACENCY:
			TriangleStripAdjacencyGen(drawSizeX, drawSizeY, output);
			break;
		default:
			throw std::runtime_error("Unknown primitive type!");
			break;
		}
	}

	std::string BoolToString(GLboolean value)
	{
		if (value == GL_TRUE)
			return "GL_TRUE";

		return "GL_FALSE";
	}

	std::string AccessToString(GLbitfield access)
	{
		switch (access)
		{
		case GL_READ_WRITE:
			return "GL_READ_WRITE";
			break;
		case GL_READ_ONLY:
			return "GL_READ_ONLY";
			break;
		case GL_WRITE_ONLY:
			return "GL_WRITE_ONLY";
			break;
		default:
			throw std::runtime_error("Invalid access type!");
			break;
		}
	}
};

template <>
void DrawIndirectBase::ReadPixelsFloat<test_api::GL>(int x, int y, int width, int height, void* data)
{
	glReadPixels(x, y, width, height, GL_RGBA, GL_FLOAT, data);
}

template <>
void DrawIndirectBase::ReadPixelsFloat<test_api::ES3>(int x, int y, int width, int height, void* data)
{
	// Use 1010102 pixel buffer for RGB10_A2 FBO to preserve precision during pixel transfer
	std::vector<GLuint>     uData(width * height);
	const tcu::PixelFormat& pixelFormat = m_context.getRenderContext().getRenderTarget().getPixelFormat();
	GLfloat*                fData       = reinterpret_cast<GLfloat*>(data);
	GLenum                  type        = ((pixelFormat.redBits   == 10) &&
	                                       (pixelFormat.greenBits == 10) &&
	                                       (pixelFormat.blueBits  == 10) &&
	                                       (pixelFormat.alphaBits == 2)) ?
	                                      GL_UNSIGNED_INT_2_10_10_10_REV :
	                                      GL_UNSIGNED_BYTE;

	glReadPixels(x, y, width, height, GL_RGBA, type, &uData[0]);

	if (type == GL_UNSIGNED_BYTE)
	{
		for (size_t i = 0; i < uData.size(); i++)
		{
			GLubyte* uCompData = reinterpret_cast<GLubyte*>(&uData[i]);

			for (size_t c = 0; c < 4; c++)
			{
				fData[i * 4 + c] = float(uCompData[c]) / 255.0f;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < uData.size(); i++)
		{
			fData[i * 4]     = float(uData[i] & 0x3FF) / 1023.0f;
			fData[i * 4 + 1] = float((uData[i] >> 10) & 0x3FF) / 1023.0f;
			fData[i * 4 + 2] = float((uData[i] >> 20) & 0x3FF) / 1023.0f;
			fData[i * 4 + 3] = float((uData[i] >> 30) & 0x3) / 3.0f;
		}
	}
}

template <>
void DrawIndirectBase::GetBufferSubData<test_api::GL>(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data)
{
	glGetBufferSubData(target, offset, size, data);
}

template <>
void DrawIndirectBase::GetBufferSubData<test_api::ES3>(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data)
{
	void* ptr = glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
	memcpy(data, ptr, size);
	glUnmapBuffer(target);
}

template <typename api>
struct CDefaultBindingPoint : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Draw Indirect: Check default binding point";
	}

	virtual std::string Purpose()
	{
		return "Verify that default binding point is set to zero";
	}

	virtual std::string Method()
	{
		return "Use glGetIntegerv, glGetBooleanv, glGetFloatv, glGetDoublev to get default binding point";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if default binding point is zero";
	}

	virtual long Run()
	{
		return BindingPointCheck<api>(0);
	}
};

template <typename api>
struct CZeroBindingPoint : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Draw Indirect: Zero binding point";
	}

	virtual std::string Purpose()
	{
		return "Verify that binding point is set to zero";
	}

	virtual std::string Method()
	{
		return "Bind zero and check that binding point is set to zero";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if binding point is set to zero";
	}

	virtual long Run()
	{
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		return BindingPointCheck<api>(0);
	}
};

template <typename api>
struct CSingleBindingPoint : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Draw Indirect: Single binding point";
	}

	virtual std::string Purpose()
	{
		return "Verify that binding point is set to correct value";
	}

	virtual std::string Method()
	{
		return "Bind non-zero buffer and check that binding point is set to correct value";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if binding point is set to correct value";
	}

	virtual long Run()
	{
		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		long ret = BindingPointCheck<api>(_buffer);

		return ret;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <typename api>
class CMultiBindingPoint : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Draw Indirect: Multi binding point";
	}

	virtual std::string Purpose()
	{
		return "Verify that binding points are set to correct value";
	}

	virtual std::string Method()
	{
		return "Bind in loop non-zero buffers and check that binding points are set to correct value";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if binding points are set to correct value";
	}

	virtual long Run()
	{
		DIResult result;

		const int buffNum = sizeof(_buffers) / sizeof(_buffers[0]);

		glGenBuffers(buffNum, _buffers);

		for (int i = 0; i < buffNum; ++i)
		{
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[i]);
			result.sub_result(BindingPointCheck<api>(_buffers[i]));
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(sizeof(_buffers) / sizeof(_buffers[0]), _buffers);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffers[10];
};

template <typename api>
struct CDeleteBindingPoint : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Draw Indirect: Delete binding point";
	}

	virtual std::string Purpose()
	{
		return "Verify that after deleting buffer, binding point is set to correct value";
	}

	virtual std::string Method()
	{
		return "Bind non-zero buffer, delete buffer, check that binding point is set to 0";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if binding point is set to correct value";
	}

	virtual long Run()
	{
		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <typename api>
struct CBufferData : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glBufferData and GetBufferSubData<api>";
	}

	virtual std::string Purpose()
	{
		return "Verify that glBufferData and GetBufferSubData<api> accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Set data using glBufferData" NL
			   "4. Get data using GetBufferSubData<api>" NL "5. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		int dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);

		CDataArray dataTest(dataWidth * dataHeight, 0);

		glGenBuffers(sizeof(_buffers) / sizeof(_buffers[0]), _buffers);
		CDataArray dataRef1;
		DataGen<unsigned int>(dataRef1, dataWidth, dataHeight, 0, 50);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[1]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef1.size() * sizeof(unsigned int)), &dataRef1[0],
					 GL_DYNAMIC_DRAW);
		result.sub_result(BindingPointCheck<api>(_buffers[1]));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef1, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		CDataArray dataRef2;
		DataGen<unsigned int>(dataRef2, dataWidth, dataHeight, 10, 70);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[2]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef2.size() * sizeof(unsigned int)), &dataRef2[0],
					 GL_STREAM_DRAW);
		result.sub_result(BindingPointCheck<api>(_buffers[2]));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[3]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, 300, NULL, GL_STATIC_DRAW);
		result.sub_result(BindingPointCheck<api>(_buffers[3]));

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[4]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, 400, NULL, GL_DYNAMIC_READ);
		result.sub_result(BindingPointCheck<api>(_buffers[4]));

		CDataArray dataRef5;
		DataGen<unsigned int>(dataRef5, dataWidth, dataHeight, 0, 50);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[5]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef5.size() * sizeof(unsigned int)), &dataRef5[0],
					 GL_STREAM_READ);
		result.sub_result(BindingPointCheck<api>(_buffers[5]));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef5, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		CDataArray dataRef6;
		DataGen<unsigned int>(dataRef6, dataWidth, dataHeight, 10, 40);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[6]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef6.size() * sizeof(unsigned int)), &dataRef6[0],
					 GL_STATIC_READ);
		result.sub_result(BindingPointCheck<api>(_buffers[6]));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef6, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		CDataArray dataRef7;
		DataGen<unsigned int>(dataRef7, dataWidth, dataHeight, 4, 70);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[7]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef7.size() * sizeof(unsigned int)), &dataRef7[0],
					 GL_DYNAMIC_COPY);
		result.sub_result(BindingPointCheck<api>(_buffers[7]));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef7, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[8]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, 800, NULL, GL_STREAM_COPY);
		result.sub_result(BindingPointCheck<api>(_buffers[8]));

		CDataArray dataRef9;
		DataGen<unsigned int>(dataRef9, dataWidth, dataHeight, 18, 35);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[9]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef9.size() * sizeof(unsigned int)), &dataRef9[0],
					 GL_STATIC_COPY);
		result.sub_result(BindingPointCheck<api>(_buffers[9]));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef9, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		//reallocation: same size
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef9.size() * sizeof(unsigned int)), &dataRef9[0],
					 GL_STATIC_COPY);
		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef9, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		//reallocation: larger size
		DataGen<unsigned int>(dataRef9, dataWidth * 2, dataHeight * 2, 18, 35);
		dataTest.resize(dataRef9.size());
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef9.size() * sizeof(unsigned int)), &dataRef9[0],
					 GL_STATIC_COPY);
		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(
			DataCompare(dataRef9, dataWidth * 2, dataHeight * 2, dataTest, dataWidth * 2, dataHeight * 2));

		//reallocation: smaller size
		DataGen<unsigned int>(dataRef9, dataWidth / 2, dataHeight / 2, 18, 35);
		dataTest.resize(dataRef9.size());
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef9.size() * sizeof(unsigned int)), &dataRef9[0],
					 GL_STATIC_COPY);
		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(
			DataCompare(dataRef9, dataWidth / 2, dataHeight / 2, dataTest, dataWidth / 2, dataHeight / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(sizeof(_buffers) / sizeof(_buffers[0]), _buffers);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffers[10];
};

template <typename api>
struct CBufferSubData : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check function: glBufferSubData and GetBufferSubData<api>";
	}

	virtual std::string Purpose()
	{
		return "Verify that glBufferSubData and GetBufferSubData<api> accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Allocate buffer using glBufferData" NL
			   "4. Set data using glBufferSubData" NL "5. Get data using GetBufferSubData<api>" NL "6. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		CDataArray dataRef;
		int		   dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		DataGen<unsigned int>(dataRef, dataWidth, dataHeight, 4, 70);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)), NULL,
					 GL_DYNAMIC_DRAW);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)), &dataRef[0]);

		CDataArray dataTest(dataWidth * dataHeight, 0);
		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);

		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		CDataArray dataSubRef;
		DataGen<unsigned int>(dataSubRef, dataWidth / 2, dataHeight / 2, 80, 90);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 4, (GLsizeiptr)(dataSubRef.size() * sizeof(unsigned int)),
						&dataSubRef[0]);
		std::copy(dataSubRef.begin(), dataSubRef.end(), dataRef.begin() + 1);

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <typename api>
struct CBufferMap : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glMapBuffer, glUnmapBuffer and getParameteriv";
	}

	virtual std::string Purpose()
	{
		return "Verify that glMapBuffer, glUnmapBuffer and getParameteriv accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Set data" NL "4. Map buffer" NL
			   "5. Verify mapped buffer" NL "6. Check state" NL "7. Unmap buffer" NL "8. Check state";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		api::GL_Only();

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		CDataArray dataRef;
		int		   dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		DataGen<unsigned int>(dataRef, dataWidth, dataHeight, 30, 50);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)), &dataRef[0],
					 GL_DYNAMIC_DRAW);

		result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));

		void* buf = glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_READ_ONLY);
		if (buf == 0)
		{
			result.error() << "glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_READ_ONLY) returned NULL";
		}

		if (buf)
		{
			result.sub_result(BufferCheck(dataRef, dataWidth, dataHeight, buf, dataWidth, dataHeight));

			result.sub_result(StateValidate<api>(GL_TRUE, GL_READ_ONLY, GL_MAP_READ_BIT, 0,
												 (GLsizeiptr)(dataRef.size() * sizeof(unsigned int))));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		buf = glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY);
		if (buf == 0)
		{
			result.error() << "glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY) returned NULL";
		}

		if (buf)
		{
			result.sub_result(BufferCheck(dataRef, dataWidth, dataHeight, buf, dataWidth, dataHeight));

			result.sub_result(StateValidate<api>(GL_TRUE, GL_WRITE_ONLY, GL_MAP_WRITE_BIT, 0,
												 (GLsizeiptr)(dataRef.size() * sizeof(unsigned int))));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) != GL_TRUE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		buf = glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_READ_WRITE);
		if (buf == 0)
		{
			result.error() << "glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_READ_WRITE) returned NULL";
		}

		if (buf)
		{
			result.sub_result(BufferCheck(dataRef, dataWidth, dataHeight, buf, dataWidth, dataHeight));

			result.sub_result(StateValidate<api>(GL_TRUE, GL_READ_WRITE, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, 0,
												 (GLsizeiptr)(dataRef.size() * sizeof(unsigned int))));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <typename api>
struct CBufferGetPointerv : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glBuffergetPointerv";
	}

	virtual std::string Purpose()
	{
		return "Verify that glBuffergetPointerv accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Set data" NL "4. Map buffer" NL
			   "5. Get a pointer to buffer" NL "6. Compare pointers from point 4) and 5)" NL
			   "7. Verify mapped buffer" NL "8. Unmap buffer";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		CDataArray dataRef;
		int		   dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		DataGen<unsigned int>(dataRef, dataWidth, dataHeight, 30, 50);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)), &dataRef[0],
					 GL_DYNAMIC_DRAW);

		void* ptr = 0;
		glGetBufferPointerv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_POINTER, &ptr);

		if (ptr != 0)
		{
			result.error() << "glGetBufferPointerv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_POINTER) returned invalid "
							  "pointer, expected: NULL";
		}

		void* buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)),
									 GL_MAP_READ_BIT);
		if (buf == 0)
		{
			result.error() << "glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, GL_MAP_READ_BIT) returned NULL";

			return result.code();
		}

		glGetBufferPointerv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_POINTER, &ptr);

		if (ptr == 0)
		{
			result.error() << "glGetBufferPointerv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_POINTER) returned NULL";
		}

		if (ptr)
		{
			result.sub_result(BufferCheck(dataRef, dataWidth, dataHeight, ptr, dataWidth, dataHeight));
		}

		if (ptr != buf)
		{
			result.error() << "glGetBufferPointerv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_MAP_POINTER) different pointer "
							  "than glMapBuffer(GL_DRAW_INDIRECT_BUFFER)";
		}

		if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
		{
			result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
		}
		buf = 0;
		ptr = 0;

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <class api>
struct CBufferMapRange : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glMapRangeBuffer, glUnmapBuffer and getParameteriv";
	}

	virtual std::string Purpose()
	{
		return "Verify that glMapRangeBuffer, glUnmapBuffer and getParameteriv accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "Bind non-zero buffer and check that binding point is set to correct value";
	}

	virtual std::string PassCriteria()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Set data" NL "4. Map buffer using glMapBufferRange" NL
			   "5. Check state" NL "6. Verify mapped buffer" NL "7. Unmap buffer" NL "8. Check state";
	}

	virtual long Run()
	{
		DIResult result;

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		CDataArray dataRef;
		int		   dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		DataGen<unsigned int>(dataRef, dataWidth, dataHeight, 30, 50);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)), &dataRef[0],
					 GL_DYNAMIC_DRAW);

		result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));

		void* buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)),
									 GL_MAP_READ_BIT);
		if (buf == 0)
		{
			result.error() << "glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_MAP_READ_BIT) returned NULL";
		}

		if (buf)
		{
			result.sub_result(BufferCheck(dataRef, dataWidth, dataHeight, buf, dataWidth, dataHeight));

			result.sub_result(StateValidate<api>(GL_TRUE, GL_READ_ONLY, GL_MAP_READ_BIT, 0,
												 (GLsizeiptr)(dataRef.size() * sizeof(unsigned int))));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataRef.size() / 2 * sizeof(unsigned int)),
							   GL_MAP_WRITE_BIT);
		if (buf == 0)
		{
			result.error() << "glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, GL_MAP_WRITE_BIT) returned NULL";
		}

		if (buf)
		{
			result.sub_result(BufferCheck(dataRef, dataWidth, dataHeight / 2, buf, dataWidth, dataHeight / 2));

			result.sub_result(StateValidate<api>(GL_TRUE, GL_WRITE_ONLY, GL_MAP_WRITE_BIT, 0,
												 (GLsizeiptr)(dataRef.size() / 2 * sizeof(unsigned int))));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, (GLintptr)(dataRef.size() / 4 * sizeof(unsigned int)),
							   (GLsizeiptr)(dataRef.size() / 2 * sizeof(unsigned int)), GL_MAP_WRITE_BIT);
		if (buf == 0)
		{
			result.error() << "glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, GL_MAP_WRITE_BIT) returned NULL";
		}

		if (buf)
		{
			result.sub_result(
				BufferCheck(dataRef, dataWidth, dataHeight / 2, buf, dataWidth, dataHeight / 2, dataHeight / 4));

			result.sub_result(StateValidate<api>(GL_TRUE, GL_WRITE_ONLY, GL_MAP_WRITE_BIT,
												 (GLintptr)(dataRef.size() / 4 * sizeof(unsigned int)),
												 (GLsizeiptr)(dataRef.size() / 2 * sizeof(unsigned int))));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <class api>
struct CBufferFlushMappedRange : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glFlushMappedBufferRange";
	}

	virtual std::string Purpose()
	{
		return "Verify that glFlushMappedBufferRange and getParameteriv accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Set data" NL
			   "4. Map buffer with GL_MAP_FLUSH_EXPLICIT_BIT flag" NL "5. Check state" NL "6. Modify mapped buffer" NL
			   "7. Flush buffer" NL "8. Unmap buffer" NL "9. Check state" NL "10. Verify buffer";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		CDataArray dataRef;
		int		   dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		DataGen<unsigned int>(dataRef, dataWidth, dataHeight, 1, 1000);

		CDataArray dataRef2;
		DataGen<unsigned int>(dataRef2, dataWidth, dataHeight, 1000, 2000);

		const int halfSize	= dataHeight / 2 * dataWidth;
		const int quarterSize = dataHeight / 4 * dataWidth;

		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef.size() * sizeof(unsigned int)), &dataRef[0],
					 GL_DYNAMIC_DRAW);

		result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));

		void* buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, quarterSize * sizeof(unsigned int),
									 halfSize * sizeof(unsigned int), GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

		if (buf == 0)
		{
			result.error() << "glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, GL_MAP_WRITE_BIT) returned NULL";
		}

		if (buf)
		{
			result.sub_result(StateValidate<api>(GL_TRUE, GL_WRITE_ONLY, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,
												 quarterSize * sizeof(unsigned int), halfSize * sizeof(unsigned int)));

			memcpy(buf, &dataRef2[quarterSize], halfSize * sizeof(unsigned int));
			glFlushMappedBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, halfSize * sizeof(unsigned int));

			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;

			result.sub_result(StateValidate<api>(GL_FALSE, GL_READ_WRITE, 0, 0, 0));
		}

		CDataArray dataTest(dataWidth * dataHeight, 0);
		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);

		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight / 4, dataTest, dataWidth, dataHeight / 4));

		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight / 2, dataTest, dataWidth, dataHeight / 2,
									  dataHeight / 4, dataHeight / 4));

		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight / 4, dataTest, dataWidth, dataHeight / 4,
									  dataHeight * 3 / 4, dataHeight * 3 / 4));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <class api>
struct CBufferBindRange : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glBindBufferRange";
	}

	virtual std::string Purpose()
	{
		return "Verify that glBindBufferRange accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer using glBindBufferRange" NL "3. Set data" NL "4. Verify buffer";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffer);

		CDataArray dataRef;
		int		   dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		DataGen<unsigned int>(dataRef, dataWidth, dataHeight, 1, 100);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, dataRef.size() * sizeof(unsigned int), &dataRef[0], GL_DYNAMIC_DRAW);

		CDataArray dataTest(dataWidth * dataHeight, 0);
		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, dataTest.size() * sizeof(unsigned int), &dataTest[0]);
		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		result.sub_result(BindingPointCheck<api>(0));

		glBindBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, _buffer, 0, dataTest.size() * sizeof(unsigned int) / 4);
		result.sub_result(BindingPointCheck<api>(_buffer));

		CDataArray dataRef2;
		DataGen<unsigned int>(dataRef2, dataWidth, dataHeight / 2, 10, 15);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, dataRef2.size() * sizeof(unsigned int) / 4, &dataRef2[0],
					 GL_DYNAMIC_DRAW);

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, dataTest.size() * sizeof(unsigned int), &dataTest[0]);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight / 4, dataTest, dataWidth, dataHeight / 4));
		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight * 3 / 4, dataTest, dataWidth, dataHeight * 3 / 4,
									  dataHeight / 4, dataHeight / 4));

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		result.sub_result(BindingPointCheck<api>(0));

		glBindBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, _buffer, dataTest.size() * sizeof(unsigned int) / 4,
						  dataTest.size() * sizeof(unsigned int) / 4);
		result.sub_result(BindingPointCheck<api>(_buffer));

		glBufferData(GL_DRAW_INDIRECT_BUFFER, dataRef2.size() * sizeof(unsigned int) / 2,
					 &dataRef2[dataRef2.size() / 2], GL_DYNAMIC_DRAW);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight / 2, dataTest, dataWidth, dataHeight / 2));
		result.sub_result(DataCompare(dataRef, dataWidth, dataHeight / 2, dataTest, dataWidth, dataHeight / 2,
									  dataHeight / 2, dataHeight / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &_buffer);

		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffer;
};

template <class api>
struct CBufferBindBase : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glBindBufferBase";
	}

	virtual std::string Purpose()
	{
		return "Verify that glBindBufferBase accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer using glBindBufferBase" NL "3. Set data" NL "4. Verify buffer";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;

		glGenBuffers(2, _buffers);

		int dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		CDataArray dataTest(dataWidth * dataHeight, 0);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[0]);
		CDataArray dataRef1;
		DataGen<unsigned int>(dataRef1, dataWidth, dataHeight, 1, 100);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, dataRef1.size() * sizeof(unsigned int), &dataRef1[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		result.sub_result(BindingPointCheck<api>(0));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, dataTest.size() * sizeof(unsigned int), &dataTest[0]);
		result.sub_result(DataCompare(dataRef1, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));
		result.sub_result(BindingPointCheck<api>(_buffers[0]));

		glBindBufferBase(GL_DRAW_INDIRECT_BUFFER, 0, _buffers[1]);
		result.sub_result(BindingPointCheck<api>(_buffers[1]));

		CDataArray dataRef2;
		DataGen<unsigned int>(dataRef2, dataWidth, dataHeight, 50, 70);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, dataRef2.size() * sizeof(unsigned int), &dataRef2[0], GL_DYNAMIC_DRAW);

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, dataTest.size() * sizeof(unsigned int), &dataTest[0]);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		result.sub_result(BindingPointCheck<api>(_buffers[1]));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(2, _buffers);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffers[2];
};

template <class api>
struct CBufferCopySubData : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Check functions: glCopyBufferSubData";
	}

	virtual std::string Purpose()
	{
		return "Verify that glCopyBufferSubData accepts GL_DRAW_INDIRECT_BUFFER enum";
	}

	virtual std::string Method()
	{
		return "1. Create buffer" NL "2. Bind buffer" NL "3. Set data" NL "4. Verify buffer" NL
			   "5. Modify buffer using glCopyBufferSubData" NL "6. Verify buffer";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		DIResult result;
		int		 dataWidth, dataHeight;
		getDataSize(dataWidth, dataHeight);
		CDataArray dataTest(dataWidth * dataHeight, 0);

		glGenBuffers(2, _buffers);
		glBindBuffer(GL_ARRAY_BUFFER, _buffers[0]);

		CDataArray dataRef1;
		DataGen<unsigned int>(dataRef1, dataWidth, dataHeight, 1, 100);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(dataRef1.size() * sizeof(unsigned int)), &dataRef1[0],
					 GL_DYNAMIC_DRAW);

		GetBufferSubData<api>(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)), &dataTest[0]);
		result.sub_result(DataCompare(dataRef1, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _buffers[1]);

		CDataArray dataRef2;
		DataGen<unsigned int>(dataRef2, dataWidth, dataHeight, 10, 30);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef2.size() * sizeof(unsigned int)), &dataRef2[0],
					 GL_DYNAMIC_DRAW);

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glCopyBufferSubData(GL_ARRAY_BUFFER, GL_DRAW_INDIRECT_BUFFER, 0, 0,
							(GLsizeiptr)(dataTest.size() * sizeof(unsigned int)));

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef1, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glBufferData(GL_DRAW_INDIRECT_BUFFER, (GLsizeiptr)(dataRef2.size() * sizeof(unsigned int)), &dataRef2[0],
					 GL_DYNAMIC_DRAW);

		GetBufferSubData<api>(GL_DRAW_INDIRECT_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)),
							  &dataTest[0]);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		glCopyBufferSubData(GL_DRAW_INDIRECT_BUFFER, GL_ARRAY_BUFFER, 0, 0,
							(GLsizeiptr)(dataTest.size() * sizeof(unsigned int)));

		GetBufferSubData<api>(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(dataTest.size() * sizeof(unsigned int)), &dataTest[0]);
		result.sub_result(DataCompare(dataRef2, dataWidth, dataHeight, dataTest, dataWidth, dataHeight));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(2, _buffers);
		return BindingPointCheck<api>(0);
	}

private:
	GLuint _buffers[2];
};

class CBasicVertexDef : public DrawIndirectBase
{
public:
	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run()
	{
		CColorArray coords;
		PrimitiveGen(_primitiveType, _drawSizeX, _drawSizeY, coords);

		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenBuffers(1, &_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand   indirectArrays   = { 0, 0, 0, 0 };
		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			indirectArrays.count			  = static_cast<GLuint>(coords.size());
			indirectArrays.primCount		  = 1;
			indirectArrays.first			  = 0;
			indirectArrays.reservedMustBeZero = 0;

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectArrays), &indirectArrays, GL_STATIC_DRAW);
				glDrawArraysIndirect(_primitiveType, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		case DRAW_ELEMENTS:
		{
			indirectElements.count				= static_cast<GLuint>(coords.size());
			indirectElements.primCount			= 1;
			indirectElements.firstIndex			= 0;
			indirectElements.baseVertex			= 0;
			indirectElements.reservedMustBeZero = 0;

			glGenBuffers(1, &_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
						 GL_STATIC_DRAW);

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectElements), &indirectElements, GL_STATIC_DRAW);
				glDrawElementsIndirect(_primitiveType, GL_UNSIGNED_INT, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);

		DIResult result;
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		if (_vao)
		{
			glDeleteVertexArrays(1, &_vao);
		}
		if (_vbo)
		{
			glDeleteBuffers(1, &_vbo);
		}
		if (_ebo)
		{
			glDeleteBuffers(1, &_ebo);
		}
		if (_program)
		{
			glDeleteProgram(_program);
		}

		return NO_ERROR;
	}

	CBasicVertexDef(TDrawFunction drawFunc, GLenum primitiveType, unsigned int drawSizeX, unsigned int drawSizeY)
		: _drawFunc(drawFunc)
		, _primitiveType(primitiveType)
		, _drawSizeX(drawSizeX)
		, _drawSizeY(drawSizeY)
		, _vao(0)
		, _vbo(0)
		, _ebo(0)
		, _program(0)
	{
	}

private:
	TDrawFunction _drawFunc;
	GLenum		  _primitiveType;
	unsigned int  _drawSizeX;
	unsigned int  _drawSizeY;

	GLuint _vao;
	GLuint _vbo, _ebo;
	GLuint _program;

	CBasicVertexDef()
	{
	}
};

class CBasicVertexInstancingDef : public DrawIndirectBase
{
public:
	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run()
	{
		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, _drawSizeX, _drawSizeY, coords);

		CColorArray coords_instanced(4);
		coords_instanced[0] = tcu::Vec4(0.5, 0.5, 0.0, 0.0);
		coords_instanced[1] = tcu::Vec4(-0.5, 0.5, 0.0, 0.0);
		coords_instanced[2] = tcu::Vec4(-0.5, -0.5, 0.0, 0.0);
		coords_instanced[3] = tcu::Vec4(0.5, -0.5, 0.0, 0.0);

		CColorArray colors_instanced(2);
		colors_instanced[0] = tcu::Vec4(1.0, 0.0, 0.0, 1.0);
		colors_instanced[1] = tcu::Vec4(0.0, 1.0, 0.0, 1.0);

		_program = CreateProgram(Vsh<api>(), "", Fsh<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenBuffers(1, &_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0]) +
												   coords_instanced.size() * sizeof(coords_instanced[0]) +
												   colors_instanced.size() * sizeof(colors_instanced[0])),
					 NULL, GL_STATIC_DRAW);

		const size_t coords_offset			 = 0;
		const size_t coords_instanced_offset = coords_offset + coords.size() * sizeof(coords[0]);
		const size_t colors_instanced_offset =
			coords_instanced_offset + coords_instanced.size() * sizeof(coords_instanced[0]);

		glBufferSubData(GL_ARRAY_BUFFER, coords_offset, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)coords_instanced_offset,
						(GLsizeiptr)(coords_instanced.size() * sizeof(coords_instanced[0])), &coords_instanced[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)colors_instanced_offset,
						(GLsizeiptr)(colors_instanced.size() * sizeof(colors_instanced[0])), &colors_instanced[0]);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		//i_vertex (coords)
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(coords_offset));
		glEnableVertexAttribArray(0);

		//i_vertex_instanced (coords_instanced)
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(coords_instanced_offset));
		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1);

		//i_vertex_color_instanced (color_instanced)
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(colors_instanced_offset));
		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 3);

		DrawArraysIndirectCommand   indirectArrays   = { 0, 0, 0, 0 };
		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			indirectArrays.count			  = static_cast<GLuint>(coords.size());
			indirectArrays.primCount		  = 4;
			indirectArrays.first			  = 0;
			indirectArrays.reservedMustBeZero = 0;

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectArrays), &indirectArrays, GL_STATIC_DRAW);
				glDrawArraysIndirect(GL_TRIANGLES, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		case DRAW_ELEMENTS:
		{
			indirectElements.count				= static_cast<GLuint>(coords.size());
			indirectElements.primCount			= 4;
			indirectElements.firstIndex			= 0;
			indirectElements.baseVertex			= 0;
			indirectElements.reservedMustBeZero = 0;

			glGenBuffers(1, &_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
						 GL_STATIC_DRAW);

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectElements), &indirectElements, GL_STATIC_DRAW);
				glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		CColorArray bufferRef1(getWindowWidth() / 2 * getWindowHeight() / 2, colors_instanced[0]);
		CColorArray bufferRef2(getWindowWidth() / 2 * getWindowHeight() / 2, colors_instanced[1]);

		CColorArray bufferTest(getWindowWidth() / 2 * getWindowHeight() / 2, tcu::Vec4(0.0f));
		DIResult	result;

		ReadPixelsFloat<api>(0, 0, getWindowWidth() / 2, getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef1,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		ReadPixelsFloat<api>((getWindowWidth() + 1) / 2, 0, getWindowWidth() / 2, getWindowHeight() / 2,
							 &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef2,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth() / 2, getWindowHeight() / 2,
							 &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef1,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		ReadPixelsFloat<api>((getWindowWidth() + 1) / 2, (getWindowHeight() + 1) / 2, getWindowWidth() / 2,
							 getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef1,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		if (_vao)
		{
			glDeleteVertexArrays(1, &_vao);
		}
		if (_vbo)
		{
			glDeleteBuffers(1, &_vbo);
		}
		if (_ebo)
		{
			glDeleteBuffers(1, &_ebo);
		}
		if (_program)
		{
			glDeleteProgram(_program);
		}

		return NO_ERROR;
	}

	template <typename api>
	std::string Vsh()
	{
		return api::glslVer() + NL
			   "layout(location = 0) in vec4 i_vertex;" NL "layout(location = 1) in vec4 i_vertex_instanced;" NL
			   "layout(location = 2) in vec4 i_vertex_color_instanced;" NL "out vec4 vertex_color_instanced;" NL
			   "void main()" NL "{" NL "    gl_Position = vec4(i_vertex.xyz * .5, 1.0) + i_vertex_instanced;" NL
			   "    vertex_color_instanced = i_vertex_color_instanced;" NL "}";
	}

	template <typename api>
	std::string Fsh()
	{
		return api::glslVer() + NL "precision highp float; " NL "in  vec4 vertex_color_instanced;" NL
								   "out vec4 outColor;" NL "void main() {" NL "  outColor = vertex_color_instanced;" NL
								   "}";
	}

	CBasicVertexInstancingDef(TDrawFunction drawFunc)
		: _drawFunc(drawFunc), _drawSizeX(2), _drawSizeY(2), _vao(0), _vbo(0), _ebo(0), _program(0)
	{
	}

private:
	TDrawFunction _drawFunc;
	unsigned int  _drawSizeX;
	unsigned int  _drawSizeY;

	GLuint _vao;
	GLuint _vbo, _ebo;
	GLuint _program;

	CBasicVertexInstancingDef()
	{
	}
};

template <typename api>
class CVBODrawArraysSingle : public CBasicVertexDef
{
public:
	virtual std::string Title()
	{
		return "VBO: Single primitive using glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that the vertex attributes can be sourced from VBO for glDrawArraysIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define a primitive using VBO" NL "2. Draw primitive using glDrawArraysIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawArraysSingle() : CBasicVertexDef(DRAW_ARRAYS, GL_TRIANGLES, 1, 1)
	{
	}
	virtual long Run()
	{
		return CBasicVertexDef::Run<api>();
	}
};

template <typename api>
class CVBODrawArraysMany : public CBasicVertexDef
{
public:
	virtual std::string Title()
	{
		return "VBO: Many primitives using glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that the vertex attributes can be sourced from VBO for glDrawArraysIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define primitives using VBO" NL "2. Draw primitive using glDrawArraysIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawArraysMany() : CBasicVertexDef(DRAW_ARRAYS, GL_TRIANGLES, 8, 8)
	{
	}
	virtual long Run()
	{
		return CBasicVertexDef::Run<api>();
	}
};

template <typename api>
class CVBODrawArraysInstancing : public CBasicVertexInstancingDef
{
public:
	virtual std::string Title()
	{
		return "VBO: Single primitive using glDrawArraysIndirect, multiple instances";
	}

	virtual std::string Purpose()
	{
		return "Verify that the vertex attributes can be sourced from VBO for glDrawArraysIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define a primitive using VBO" NL "2. Draw primitive using glDrawArraysIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawArraysInstancing() : CBasicVertexInstancingDef(DRAW_ARRAYS)
	{
	}
	virtual long Run()
	{
		return CBasicVertexInstancingDef::Run<api>();
	}
};

class CBasicXFBPausedDef : public DrawIndirectBase
{
public:
	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run()
	{
		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, _drawSizeX, _drawSizeY, coords);

		_program = CreateProgram(Vsh<api>(), "", shaders::fshSimple<api>(), false);

		const GLchar* varyings[] = { "dataOut" };
		glTransformFeedbackVaryings(_program, 1, varyings, GL_INTERLEAVED_ATTRIBS);
		glLinkProgram(_program);
		if (!CheckProgram(_program))
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenBuffers(1, &_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand   indirectArrays   = { 0, 0, 0, 0 };
		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenTransformFeedbacks(1, &_xfo);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _xfo);

		glGenBuffers(1, &_xfbo);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, _xfbo);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1024, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _xfbo);

		glBeginTransformFeedback(GL_TRIANGLES);
		glPauseTransformFeedback();

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			indirectArrays.count			  = static_cast<GLuint>(coords.size());
			indirectArrays.primCount		  = 1;
			indirectArrays.first			  = 0;
			indirectArrays.reservedMustBeZero = 0;

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectArrays), &indirectArrays, GL_STATIC_DRAW);
				glDrawArraysIndirect(GL_TRIANGLES, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		case DRAW_ELEMENTS:
		{
			indirectElements.count				= static_cast<GLuint>(coords.size());
			indirectElements.primCount			= 1;
			indirectElements.firstIndex			= 0;
			indirectElements.baseVertex			= 0;
			indirectElements.reservedMustBeZero = 0;

			glGenBuffers(1, &_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
						 GL_STATIC_DRAW);

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectElements), &indirectElements, GL_STATIC_DRAW);
				glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
				glDeleteBuffers(1, &buffer);
			}
		}

		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		glResumeTransformFeedback();
		glEndTransformFeedback();

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);

		DIResult result;
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		if (_vao)
		{
			glDeleteVertexArrays(1, &_vao);
		}
		if (_vbo)
		{
			glDeleteBuffers(1, &_vbo);
		}
		if (_ebo)
		{
			glDeleteBuffers(1, &_ebo);
		}
		if (_xfbo)
		{
			glDeleteBuffers(1, &_xfbo);
		}
		if (_program)
		{
			glDeleteProgram(_program);
		}
		if (_xfo)
		{
			glDeleteTransformFeedbacks(1, &_xfo);
		}

		return NO_ERROR;
	}

	template <typename api>
	std::string Vsh()
	{
		return api::glslVer() + NL "in vec4 i_vertex;" NL "out vec4 dataOut;" NL "void main()" NL "{" NL
								   "    gl_Position = i_vertex;" NL "    dataOut = i_vertex;" NL "}";
	}

	CBasicXFBPausedDef(TDrawFunction drawFunc)
		: _drawFunc(drawFunc), _drawSizeX(2), _drawSizeY(2), _vao(0), _vbo(0), _ebo(0), _xfbo(0), _program(0), _xfo(0)
	{
	}

private:
	TDrawFunction _drawFunc;
	unsigned int  _drawSizeX;
	unsigned int  _drawSizeY;

	GLuint _vao;
	GLuint _vbo, _ebo, _xfbo;
	GLuint _program;
	GLuint _xfo;

	CBasicXFBPausedDef()
	{
	}
};

template <typename api>
class CVBODrawArraysXFBPaused : public CBasicXFBPausedDef
{
public:
	virtual std::string Title()
	{
		return "VBO: glDrawArraysIndirect, in paused transform feedback operation";
	}

	virtual std::string Purpose()
	{
		return "Verify  glDrawArraysIndirect works, if XFB is active and paused";
	}

	virtual std::string Method()
	{
		return "1. Define a primitive using VBO" NL "2. Draw primitive using glDrawArraysIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawArraysXFBPaused() : CBasicXFBPausedDef(DRAW_ARRAYS)
	{
	}
	virtual long Run()
	{
		return CBasicXFBPausedDef::Run<api>();
	}
};

template <typename api>
class CVBODrawElementsSingle : public CBasicVertexDef
{
public:
	virtual std::string Title()
	{
		return "VBO: Single primitive using glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that the vertex attributes can be sourced from VBO for glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define a primitive using VBO" NL "2. Draw primitive using glDrawElementsIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawElementsSingle() : CBasicVertexDef(DRAW_ELEMENTS, GL_TRIANGLES, 1, 1)
	{
	}
	virtual long Run()
	{
		return CBasicVertexDef::Run<api>();
	}
};

template <typename api>
class CVBODrawElementsMany : public CBasicVertexDef
{
public:
	virtual std::string Title()
	{
		return "VBO: Many primitives using glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that the vertex attributes can be sourced from VBO for glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define primitives using VBO" NL "2. Draw primitive using glDrawElementsIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawElementsMany() : CBasicVertexDef(DRAW_ELEMENTS, GL_TRIANGLES, 8, 8)
	{
	}

	virtual long Run()
	{
		return CBasicVertexDef::Run<api>();
	}
};

template <typename api>
class CVBODrawElementsInstancing : public CBasicVertexInstancingDef
{
public:
	virtual std::string Title()
	{
		return "VBO: Single primitive using glDrawElementsIndirect, multiple instances";
	}

	virtual std::string Purpose()
	{
		return "Verify that the vertex attributes can be sourced from VBO for glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define a primitive using VBO" NL "2. Draw primitive using glDrawElementsIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawElementsInstancing() : CBasicVertexInstancingDef(DRAW_ELEMENTS)
	{
	}
	virtual long Run()
	{
		return CBasicVertexInstancingDef::Run<api>();
	}
};

template <typename api>
class CVBODrawElementsXFBPaused : public CBasicXFBPausedDef
{
public:
	virtual std::string Title()
	{
		return "VBO: glDrawElementsIndirect, in paused transform feedback operation";
	}

	virtual std::string Purpose()
	{
		return "Verify  glDrawElementsIndirect works, if XFB is active and paused";
	}

	virtual std::string Method()
	{
		return "1. Define a primitive using VBO" NL "2. Draw primitive using glDrawArraysIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CVBODrawElementsXFBPaused() : CBasicXFBPausedDef(DRAW_ELEMENTS)
	{
	}
	virtual long Run()
	{
		return CBasicXFBPausedDef::Run<api>();
	}
};

template <typename api>
class CBufferIndirectDrawArraysSimple : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawArraysIndirect: many primitives simple";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with specified indirect structure" NL "in a buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;
		indirectArrays.first					 = 0;
		indirectArrays.reservedMustBeZero		 = 0;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		glDrawArraysIndirect(GL_TRIANGLES, 0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
class CBufferIndirectDrawArraysNoFirst : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawArraysIndirect: non-zero 'first' argument";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with specified non-zero 'first' argument" NL
			   "in indirect buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size()) / 2;
		indirectArrays.primCount				 = 1;
		indirectArrays.first					 = static_cast<GLuint>(coords.size()) / 2;
		indirectArrays.reservedMustBeZero		 = 0;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		glDrawArraysIndirect(GL_TRIANGLES, 0);

		CColorArray bufferRef1(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		CColorArray bufferRef2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef2,
										 getWindowWidth(), getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
class CBufferIndirectDrawArraysOffset : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawArraysIndirect: offset as a function parameter";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with offset as a function parameter";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect with offset" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;
		indirectArrays.first					 = 0;
		indirectArrays.reservedMustBeZero		 = 0;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), sizeof(DrawArraysIndirectCommand),
						&indirectArrays);
		glDrawArraysIndirect(GL_TRIANGLES, (void*)sizeof(DrawArraysIndirectCommand));

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
class CBufferIndirectDrawElementsSimple : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawElementsIndirect: many primitives simple";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with specified indirect structure" NL "in a buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = 0;
		indirectElements.firstIndex					 = 0;
		indirectElements.reservedMustBeZero			 = 0;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		glDeleteBuffers(1, &_ebo);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect, _ebo;
};

template <typename api>
class CBufferIndirectDrawElementsNoFirstIndex : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawElementsIndirect: non-zero first index";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with non-zero first index" NL "in indirect buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size()) / 2;
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = 0;
		indirectElements.firstIndex					 = static_cast<GLuint>(coords.size()) / 2;
		indirectElements.reservedMustBeZero			 = 0;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

		CColorArray bufferRef1(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		CColorArray bufferRef2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef2,
										 getWindowWidth(), getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect, _ebo;
};

template <typename api>
class CBufferIndirectDrawElementsNoBasevertex : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawElementsIndirect: non-zero base vertex";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with non-zero base vertex" NL "in indirect buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size()) / 2;
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = static_cast<GLint>(coords.size()) / 2;
		indirectElements.firstIndex					 = 0;
		indirectElements.reservedMustBeZero			 = 0;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

		CColorArray bufferRef1(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		CColorArray bufferRef2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef2,
										 getWindowWidth(), getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
class CBufferIndirectDrawElementsOffset : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawElementsIndirect: offset as a function parameter";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with offset as a function parameter";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect with offset" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = 0;
		indirectElements.firstIndex					 = 0;
		indirectElements.reservedMustBeZero			 = 0;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand),
						sizeof(DrawElementsIndirectCommand), &indirectElements);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);
		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)sizeof(DrawElementsIndirectCommand));

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

class CBasicVertexIDsDef : public DrawIndirectBase
{
public:
	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run()
	{
		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, _drawSizeX, _drawSizeY, coords);

		CColorArray coords_instanced(4);
		coords_instanced[0] = tcu::Vec4(0.5, 0.5, 0.0, 0.0);
		coords_instanced[1] = tcu::Vec4(-0.5, 0.5, 0.0, 0.0);
		coords_instanced[2] = tcu::Vec4(-0.5, -0.5, 0.0, 0.0);
		coords_instanced[3] = tcu::Vec4(0.5, -0.5, 0.0, 0.0);

		std::vector<glw::GLfloat> ref_VertexId(coords.size());
		for (size_t i = 0; i < ref_VertexId.size(); i++)
		{
			ref_VertexId[i] = glw::GLfloat(i);
		}

		std::vector<glw::GLfloat> ref_InstanceId(4);
		for (size_t i = 0; i < ref_InstanceId.size(); i++)
		{
			ref_InstanceId[i] = glw::GLfloat(i);
		}

		_program = CreateProgram(Vsh<api>(), "", Fsh<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenBuffers(1, &_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0]) +
												   coords_instanced.size() * sizeof(coords_instanced[0]) +
												   ref_VertexId.size() * sizeof(ref_VertexId[0]) +
												   ref_InstanceId.size() * sizeof(ref_InstanceId[0])),
					 NULL, GL_STATIC_DRAW);

		const size_t coords_offset			 = 0;
		const size_t coords_instanced_offset = coords_offset + coords.size() * sizeof(coords[0]);
		const size_t ref_VertexId_offset =
			coords_instanced_offset + coords_instanced.size() * sizeof(coords_instanced[0]);
		const size_t ref_InstanceId_offset = ref_VertexId_offset + ref_VertexId.size() * sizeof(ref_VertexId[0]);

		glBufferSubData(GL_ARRAY_BUFFER, coords_offset, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)coords_instanced_offset,
						(GLsizeiptr)(coords_instanced.size() * sizeof(coords_instanced[0])), &coords_instanced[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)ref_VertexId_offset,
						(GLsizeiptr)(ref_VertexId.size() * sizeof(ref_VertexId[0])), &ref_VertexId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)ref_InstanceId_offset,
						(GLsizeiptr)(ref_InstanceId.size() * sizeof(ref_InstanceId[0])), &ref_InstanceId[0]);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		//i_vertex (coords)
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(coords_offset));
		glEnableVertexAttribArray(0);

		//i_vertex_instanced (coords_instanced)
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(coords_instanced_offset));
		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1);

		//i_ref_VertexId
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(ref_VertexId_offset));
		glEnableVertexAttribArray(2);
		//i_ref_InstanceId
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<glw::GLvoid*>(ref_InstanceId_offset));
		glEnableVertexAttribArray(3);
		glVertexAttribDivisor(3, 1);

		DrawArraysIndirectCommand   indirectArrays   = { 0, 0, 0, 0 };
		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			indirectArrays.count			  = static_cast<GLuint>(coords.size());
			indirectArrays.primCount		  = 4;
			indirectArrays.first			  = 0;
			indirectArrays.reservedMustBeZero = 0;

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectArrays), &indirectArrays, GL_STATIC_DRAW);
				glDrawArraysIndirect(GL_TRIANGLES, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		case DRAW_ELEMENTS:
		{
			indirectElements.count				= static_cast<GLuint>(coords.size());
			indirectElements.primCount			= 4;
			indirectElements.firstIndex			= 0;
			indirectElements.baseVertex			= 0;
			indirectElements.reservedMustBeZero = 0;

			glGenBuffers(1, &_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
						 GL_STATIC_DRAW);

			{
				GLuint buffer;
				glGenBuffers(1, &buffer);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
				glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirectElements), &indirectElements, GL_STATIC_DRAW);
				glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
				glDeleteBuffers(1, &buffer);
			}
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		CColorArray bufferRef1(getWindowWidth() / 2 * getWindowHeight() / 2, tcu::Vec4(0.0f, 1.0f, 0.5f, 0.0f));
		CColorArray bufferRef2(getWindowWidth() / 2 * getWindowHeight() / 2, tcu::Vec4(0.0f, 1.0f, 0.75f, 0.0f));
		CColorArray bufferRef3(getWindowWidth() / 2 * getWindowHeight() / 2, tcu::Vec4(0.0f, 1.0f, 0.25f, 0.0f));
		CColorArray bufferRef4(getWindowWidth() / 2 * getWindowHeight() / 2, tcu::Vec4(0.0f, 1.0f, 0.0f, 0.0f));

		CColorArray bufferTest(getWindowWidth() / 2 * getWindowHeight() / 2, tcu::Vec4(0.0f));
		DIResult	result;

		ReadPixelsFloat<api>(0, 0, getWindowWidth() / 2, getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef1,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		ReadPixelsFloat<api>((getWindowWidth() + 1) / 2, 0, getWindowWidth() / 2, getWindowHeight() / 2,
							 &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef2,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth() / 2, getWindowHeight() / 2,
							 &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef3,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		ReadPixelsFloat<api>((getWindowWidth() + 1) / 2, (getWindowHeight() + 1) / 2, getWindowWidth() / 2,
							 getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth() / 2, getWindowHeight() / 2, bufferRef4,
										 getWindowWidth() / 2, getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		if (_vao)
		{
			glDeleteVertexArrays(1, &_vao);
		}
		if (_vbo)
		{
			glDeleteBuffers(1, &_vbo);
		}
		if (_ebo)
		{
			glDeleteBuffers(1, &_ebo);
		}
		if (_program)
		{
			glDeleteProgram(_program);
		}

		return NO_ERROR;
	}

	template <typename api>
	std::string Vsh()
	{
		return api::glslVer() + NL
			   "layout(location = 0) in vec4 i_vertex;" NL "layout(location = 1) in vec4 i_vertex_instanced;" NL
			   "layout(location = 2) in float i_ref_VertexId;" NL "layout(location = 3) in float i_ref_InstanceId;" NL
			   "out vec4 val_Result;" NL "void main()" NL "{" NL
			   "    gl_Position = vec4(i_vertex.xyz * .5, 1.0) + i_vertex_instanced;" NL
			   "    if ( gl_VertexID == int(i_ref_VertexId + .5) && gl_InstanceID == int(i_ref_InstanceId + .5)) {" NL
			   "        val_Result = vec4(0.0, 1.0, float(gl_InstanceID) / 4.0, 1.0);" NL "    } else {" NL
			   "        val_Result = vec4(1.0, 0.0, 0.0, 1.0);" NL "    }" NL "}";
	}

	template <typename api>
	std::string Fsh()
	{
		return api::glslVer() + NL "precision highp float; " NL "in vec4 val_Result;" NL "out vec4 outColor;" NL
								   "void main() {" NL "  outColor = val_Result;" NL "}";
	}

	CBasicVertexIDsDef(TDrawFunction drawFunc)
		: _drawFunc(drawFunc), _drawSizeX(2), _drawSizeY(2), _vao(0), _vbo(0), _ebo(0), _program(0)
	{
	}

private:
	TDrawFunction _drawFunc;
	unsigned int  _drawSizeX;
	unsigned int  _drawSizeY;

	GLuint _vao;
	GLuint _vbo, _ebo;
	GLuint _program;

	CBasicVertexIDsDef()
	{
	}
};

template <typename api>
class CBufferIndirectDrawArraysVertexIds : public CBasicVertexIDsDef
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawArraysIndirect: all non-zero arguments, verify vertex ids";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with all non-zero arguments" NL "in indirect buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		return CBasicVertexIDsDef::Run<api>();
	}

	CBufferIndirectDrawArraysVertexIds() : CBasicVertexIDsDef(DRAW_ARRAYS)
	{
	}
};

template <typename api>
class CBufferIndirectDrawElementsVertexIds : public CBasicVertexIDsDef
{
public:
	virtual std::string Title()
	{
		return "Indirect buffer glDrawElementsIndirect: all non-zero arguments, verify vertex ids";
	}

	virtual std::string Purpose()
	{
		return "Verify that it is possible to draw primitives with all non-zero arguments" NL "in indirect buffer";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Run()
	{
		return CBasicVertexIDsDef::Run<api>();
	}

	CBufferIndirectDrawElementsVertexIds() : CBasicVertexIDsDef(DRAW_ELEMENTS)
	{
	}
};

template <typename api>
class CIndicesDataTypeUnsignedShort : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect indices data type: unsigned short";
	}

	virtual std::string Purpose()
	{
		return "Verify that unsigned short indices are accepted by glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL "3. Create element buffer" NL
			   "4. Draw primitives using glDrawElementsIndirect" NL "5. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size()) / 2;
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = -static_cast<GLint>(coords.size()) / 4;
		indirectElements.firstIndex					 = static_cast<GLuint>(coords.size()) / 4;
		indirectElements.reservedMustBeZero			 = 0;

		std::vector<GLushort> elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLushort>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), &indirectElements);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0);

		CColorArray bufferRef1(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		CColorArray bufferRef2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef2,
										 getWindowWidth(), getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
class CIndicesDataTypeUnsignedByte : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect indices data type: unsigned byte";
	}

	virtual std::string Purpose()
	{
		return "Verify that unsigned byte indices are accepted by glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL "3. Create element buffer" NL
			   "4. Draw primitives using glDrawElementsIndirect" NL "5. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 2, 2, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size()) / 2;
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = -static_cast<GLint>(coords.size()) / 4;
		indirectElements.firstIndex					 = static_cast<GLuint>(coords.size()) / 4;
		indirectElements.reservedMustBeZero			 = 0;

		std::vector<GLubyte> elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLubyte>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), &indirectElements);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_BYTE, 0);

		CColorArray bufferRef1(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		CColorArray bufferRef2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef2,
										 getWindowWidth(), getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 1) / 2, getWindowWidth(), getWindowHeight() / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight() / 2, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 2));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

class CPrimitiveMode : public DrawIndirectBase
{
public:
	template <typename api>
	bool IsGeometryShaderSupported()
	{
		if (api::isES())
		{
			const glu::ContextInfo& info = m_context.getContextInfo();
			const glu::ContextType& type = m_context.getRenderContext().getType();

			/* ES 3.2+ included geometry shaders into the core */
			if (glu::contextSupports(type, glu::ApiType(3, 2, glu::PROFILE_ES)))
			{
				return true;
			}
			/* ES 3.1 may be able to support geometry shaders via extensions */
			else if ((glu::contextSupports(type, glu::ApiType(3, 1, glu::PROFILE_ES))) &&
					 ((true == info.isExtensionSupported("GL_EXT_geometry_shader")) ||
					  (true == info.isExtensionSupported("GL_OES_geometry_shader"))))
			{
				return true;
			}
			else
			{
				OutputNotSupported("Geometry shader is not supported\n");
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	virtual long Setup()
	{
		_sizeX = getWindowWidth();
		_sizeY = getWindowHeight();
		if (_primitiveType != GL_TRIANGLE_STRIP && _primitiveType != GL_TRIANGLE_STRIP_ADJACENCY &&
			_primitiveType != GL_TRIANGLES && _primitiveType != GL_TRIANGLES_ADJACENCY &&
			_primitiveType != GL_TRIANGLE_FAN)
		{
			_sizeX &= (-4);
			_sizeY &= (-4);
		}
		if ((int)_drawSizeX < 0 || (int)_drawSizeY < 0)
		{
			//no PrimitiveGen dimensions given. assume same dimensions as rendered image^
			_drawSizeX = _sizeX;
			_drawSizeY = _sizeY;
			if (_primitiveType == GL_POINTS)
			{
				//clamp vertex number (and rendering size) for points to max. 10000
				_sizeX = _drawSizeX = std::min(_drawSizeX, 100u);
				_sizeY = _drawSizeY = std::min(_drawSizeY, 100u);
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run(bool pointMode = false)
	{

		glClear(GL_COLOR_BUFFER_BIT);

		_program = CreateProgram(pointMode ? shaders::vshSimple_point<api>() : shaders::vshSimple<api>(), "",
								 shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(_primitiveType, _drawSizeX, _drawSizeY, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		CColorArray padding(10, tcu::Vec4(0.0f));

		glBufferData(GL_ARRAY_BUFFER,
					 (GLsizeiptr)(coords.size() * (sizeof(coords[0])) + padding.size() * (sizeof(padding[0]))), NULL,
					 GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(padding.size() * (sizeof(padding[0]))), &padding[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(padding.size() * (sizeof(padding[0]))),
						(GLsizeiptr)(coords.size() * (sizeof(coords[0]))), &coords[0]);

		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		DrawArraysIndirectCommand   indirectArrays   = { 0, 0, 0, 0 };

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			indirectArrays.count			  = static_cast<GLuint>(coords.size()) / 2;
			indirectArrays.primCount		  = 1;
			indirectArrays.first			  = 10;
			indirectArrays.reservedMustBeZero = 0;

			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), NULL, GL_STATIC_DRAW);
			glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysIndirectCommand), &indirectArrays);

			glDrawArraysIndirect(_primitiveType, 0);
		}
		break;
		case DRAW_ELEMENTS:
		{
			indirectElements.count				= static_cast<GLuint>(coords.size()) / 2;
			indirectElements.primCount			= 1;
			indirectElements.baseVertex			= 7;
			indirectElements.firstIndex			= 3;
			indirectElements.reservedMustBeZero = 0;

			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), NULL, GL_STATIC_DRAW);
			glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), &indirectElements);

			glGenBuffers(1, &_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
						 GL_STATIC_DRAW);

			glDrawElementsIndirect(_primitiveType, GL_UNSIGNED_INT, 0);
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		CColorArray bufferRef1(_sizeX * _sizeY / 2, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		CColorArray bufferRef2(_sizeX * _sizeY / 2, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(_sizeX * _sizeY, tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, (_sizeY + 1) / 2, _sizeX, _sizeY / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, _sizeX, _sizeY / 2, bufferRef1, _sizeX, _sizeY / 2));

		switch (_primitiveType)
		{
		case GL_TRIANGLES_ADJACENCY:
		case GL_TRIANGLE_STRIP_ADJACENCY:
		{
			CColorArray bufferRef3(_sizeX * _sizeY / 16, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
			CColorArray bufferRef4(_sizeX * _sizeY / 8, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
			CColorArray bufferRef5(_sizeX * _sizeY / 4, tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

			CColorArray bufferTest3(_sizeX * _sizeY / 16, tcu::Vec4(0.0f));
			CColorArray bufferTest4(_sizeX * _sizeY / 8, tcu::Vec4(0.0f));
			CColorArray bufferTest5(_sizeX * _sizeY / 4, tcu::Vec4(0.0f));

			ReadPixelsFloat<api>(0, (_sizeY + 3) / 4, _sizeX / 4, _sizeY / 4, &bufferTest3[0]);
			result.sub_result(BuffersCompare(bufferTest3, _sizeX / 4, _sizeY / 4, bufferRef3, _sizeX / 4, _sizeY / 4));

			ReadPixelsFloat<api>((_sizeX + 3) / 4, (_sizeY + 3) / 4, _sizeX / 2, _sizeY / 4, &bufferTest4[0]);
			result.sub_result(BuffersCompare(bufferTest4, _sizeX / 2, _sizeY / 4, bufferRef4, _sizeX / 2, _sizeY / 4));

			ReadPixelsFloat<api>((_sizeX * 3 + 3) / 4, (_sizeY + 3) / 4, _sizeX / 4, _sizeY / 4, &bufferTest3[0]);
			result.sub_result(BuffersCompare(bufferTest3, _sizeX / 4, _sizeY / 4, bufferRef3, _sizeX / 4, _sizeY / 4));

			ReadPixelsFloat<api>(0, 0, _sizeX, _sizeY / 4, &bufferTest5[0]);
			result.sub_result(BuffersCompare(bufferTest5, _sizeX, _sizeY / 4, bufferRef5, _sizeX, _sizeY / 4));
		}
		break;
		default:
		{
			ReadPixelsFloat<api>(0, 0, _sizeX, _sizeY / 2, &bufferTest[0]);
			result.sub_result(BuffersCompare(bufferTest, _sizeX, _sizeY / 2, bufferRef2, _sizeX, _sizeY / 2));
		}
		break;
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

	CPrimitiveMode(TDrawFunction drawFunc, GLenum primitiveType, unsigned int sizeX = -1, unsigned sizeY = -1)
		: _drawFunc(drawFunc)
		, _primitiveType(primitiveType)
		, _drawSizeX(sizeX)
		, _drawSizeY(sizeY)
		, _sizeX(0)
		, _sizeY(0)
		, _program(0)
		, _vao(0)
		, _buffer(0)
		, _ebo(0)
		, _bufferIndirect(0)
	{
	}

private:
	TDrawFunction _drawFunc;
	GLenum		  _primitiveType;
	unsigned int  _drawSizeX, _drawSizeY; //dims for primitive generator
	unsigned int  _sizeX, _sizeY;		  //rendering size
	GLuint		  _program;
	GLuint		  _vao, _buffer, _ebo, _bufferIndirect;

	CPrimitiveMode();
};

template <typename api>
class CModeDrawArraysPoints : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_POINTS";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_POINTS works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysPoints() : CPrimitiveMode(DRAW_ARRAYS, GL_POINTS)
	{
	}

	virtual long Run()
	{
		return CPrimitiveMode::Run<api>(true);
	}
};

template <typename api>
class CModeDrawArraysLines : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_LINES";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_LINES works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysLines() : CPrimitiveMode(DRAW_ARRAYS, GL_LINES)
	{
	}

	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysLineStrip : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_LINE_STRIP";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_LINE_STRIP works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysLineStrip() : CPrimitiveMode(DRAW_ARRAYS, GL_LINE_STRIP)
	{
	}
	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysLineLoop : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_LINE_LOOP";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_LINE_LOOP works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysLineLoop() : CPrimitiveMode(DRAW_ARRAYS, GL_LINE_LOOP)
	{
	}

	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysTriangleStrip : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_TRIANGLE_STRIP";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_TRIANGLE_STRIP works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysTriangleStrip() : CPrimitiveMode(DRAW_ARRAYS, GL_TRIANGLE_STRIP, 2, 2)
	{
	}

	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysTriangleFan : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_TRIANGLE_FAN";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_TRIANGLE_FAN works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysTriangleFan() : CPrimitiveMode(DRAW_ARRAYS, GL_TRIANGLE_FAN, 2, 2)
	{
	}

	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysLinesAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_LINES_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_LINES_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysLinesAdjacency() : CPrimitiveMode(DRAW_ARRAYS, GL_LINES_ADJACENCY)
	{
	}

	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysLineStripAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_LINE_STRIP_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_LINE_STRIP_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysLineStripAdjacency() : CPrimitiveMode(DRAW_ARRAYS, GL_LINE_STRIP_ADJACENCY)
	{
	}

	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysTrianglesAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_TRIANGLES_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_TRIANGLES_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysTrianglesAdjacency() : CPrimitiveMode(DRAW_ARRAYS, GL_TRIANGLES_ADJACENCY, 4, 4)
	{
	}

	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawArraysTriangleStripAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawArraysIndirect mode: GL_TRIANGLE_STRIP_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawArraysIndirect with GL_TRIANGLE_STRIP_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawArraysIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawArraysTriangleStripAdjacency() : CPrimitiveMode(DRAW_ARRAYS, GL_TRIANGLE_STRIP_ADJACENCY, 4, 4)
	{
	}
	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsPoints : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_POINTS";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_POINTS works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsPoints() : CPrimitiveMode(DRAW_ELEMENTS, GL_POINTS)
	{
	}

	virtual long Run()
	{
		return CPrimitiveMode::Run<api>(true);
	}
};

template <typename api>
class CModeDrawElementsLines : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_LINES";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_LINES works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsLines() : CPrimitiveMode(DRAW_ELEMENTS, GL_LINES)
	{
	}
	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsLineStrip : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_LINE_STRIP";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_LINE_STRIP works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsLineStrip() : CPrimitiveMode(DRAW_ELEMENTS, GL_LINE_STRIP)
	{
	}
	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsLineLoop : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_LINE_LOOP";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_LINE_LOOP works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsLineLoop() : CPrimitiveMode(DRAW_ELEMENTS, GL_LINE_LOOP)
	{
	}
	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsTriangleStrip : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_TRIANGLE_STRIP";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_TRIANGLE_STRIP works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsTriangleStrip() : CPrimitiveMode(DRAW_ELEMENTS, GL_TRIANGLE_STRIP, 2, 2)
	{
	}
	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsTriangleFan : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_TRIANGLE_FAN";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_TRIANGLE_FAN works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsTriangleFan() : CPrimitiveMode(DRAW_ELEMENTS, GL_TRIANGLE_FAN, 2, 2)
	{
	}
	virtual long Run()
	{
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsLinesAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_LINES_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_LINES_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsLinesAdjacency() : CPrimitiveMode(DRAW_ELEMENTS, GL_LINES_ADJACENCY)
	{
	}
	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsLineStripAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_LINE_STRIP_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_LINE_STRIP_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsLineStripAdjacency() : CPrimitiveMode(DRAW_ELEMENTS, GL_LINE_STRIP_ADJACENCY)
	{
	}
	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsTrianglesAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_TRIANGLES_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_TRIANGLES_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsTrianglesAdjacency() : CPrimitiveMode(DRAW_ELEMENTS, GL_TRIANGLES_ADJACENCY, 4, 4)
	{
	}
	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

template <typename api>
class CModeDrawElementsTriangleStripAdjacency : public CPrimitiveMode
{
public:
	virtual std::string Title()
	{
		return "glDrawElementsIndirect mode: GL_TRIANGLE_STRIP_ADJACENCY";
	}

	virtual std::string Purpose()
	{
		return "Verify that glDrawElementsIndirect with GL_TRIANGLE_STRIP_ADJACENCY works correctly";
	}

	virtual std::string Method()
	{
		return "1. Create and fill VBO" NL "2. Create indirect buffer" NL
			   "3. Draw primitives using glDrawElementsIndirect" NL "4. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CModeDrawElementsTriangleStripAdjacency() : CPrimitiveMode(DRAW_ELEMENTS, GL_TRIANGLE_STRIP_ADJACENCY, 4, 4)
	{
	}
	virtual long Run()
	{
		if (!IsGeometryShaderSupported<api>())
			return NOT_SUPPORTED;
		return CPrimitiveMode::Run<api>();
	}
};

class CTransformFeedback : public DrawIndirectBase
{
public:
	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run()
	{
		CColorArray coords;
		PrimitiveGen(GL_TRIANGLE_STRIP, 8, 8, coords);

		glClear(GL_COLOR_BUFFER_BIT);

		_program				 = CreateProgram(Vsh<api>(), "", shaders::fshSimple<api>(), false);
		const GLchar* varyings[] = { "dataOut" };
		glTransformFeedbackVaryings(_program, 1, varyings, GL_INTERLEAVED_ATTRIBS);
		glLinkProgram(_program);
		if (!CheckProgram(_program))
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenBuffers(1, &_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STATIC_DRAW);

		glGenBuffers(1, &_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
		glBufferData(GL_UNIFORM_BUFFER, 4 * 5 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformBlockIndex(_program, "BLOCK"), _ubo);
		std::vector<GLuint> uboData;

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			uboData.resize(4 * 4, 0);

			uboData[0]  = static_cast<GLuint>(coords.size()); //count
			uboData[4]  = 1;								  //primcount
			uboData[8]  = 0;								  //first
			uboData[12] = 0;								  //mbz
		}
		break;
		case DRAW_ELEMENTS:
		{
			uboData.resize(4 * 5, 0);
			uboData[0]  = static_cast<GLuint>(coords.size()); //count
			uboData[4]  = 1;								  //primcount
			uboData[8]  = 0;								  //firstindex
			uboData[12] = 0;								  //basevertex
			uboData[16] = 0;								  //mbz
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}
		glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)(uboData.size() * sizeof(uboData[0])), &uboData[0]);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glGenBuffers(1, &_bufferIndirect);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, _bufferIndirect);
		GLuint zeroes[] = { 0, 0, 0, 0, 0 };
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(zeroes), zeroes, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);

		glEnable(GL_RASTERIZER_DISCARD);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, 5);
		glEndTransformFeedback();
		glDisable(GL_RASTERIZER_DISCARD);

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
			glDrawArraysIndirect(GL_TRIANGLES, 0);
			break;
		case DRAW_ELEMENTS:
			glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

			break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);

		DIResult result;
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		if (_vao)
		{
			glDeleteVertexArrays(1, &_vao);
		}
		if (_vbo)
		{
			glDeleteBuffers(1, &_vbo);
		}
		if (_ebo)
		{
			glDeleteBuffers(1, &_ebo);
		}
		if (_ubo)
		{
			glDeleteBuffers(1, &_ubo);
		}
		if (_bufferIndirect)
		{
			glDeleteBuffers(1, &_bufferIndirect);
		}
		if (_program)
		{
			glDeleteProgram(_program);
		}
		return NO_ERROR;
	}

	CTransformFeedback(TDrawFunction drawFunc)
		: _drawFunc(drawFunc), _program(0), _vao(0), _vbo(0), _ebo(0), _ubo(0), _bufferIndirect(0)
	{
	}

private:
	TDrawFunction _drawFunc;
	GLuint		  _program;
	GLuint		  _vao, _vbo, _ebo, _ubo, _bufferIndirect;

	CTransformFeedback();

	template <typename api>
	std::string Vsh()
	{
		return api::glslVer() + NL "flat out highp uint dataOut;" NL "in vec4 i_vertex;" NL
								   "layout(std140) uniform  BLOCK {" NL " uint m[5];" NL "} b;" NL "void main() {" NL
								   "  dataOut = b.m[min(4, gl_VertexID)];" NL "    gl_Position = i_vertex;" NL "}";
	}
};

template <typename api>
struct CTransformFeedbackArray : public CTransformFeedback
{
	virtual std::string Title()
	{
		return "Transform feedback: glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that transform feedback works correctly with glDrawArrayIndirect";
	}

	virtual std::string Method()
	{
		return "1. Create data" NL "2. Use data as input to glDrawArrayIndirect" NL "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CTransformFeedbackArray() : CTransformFeedback(DRAW_ARRAYS)
	{
	}

	virtual long Run()
	{
		return CTransformFeedback::Run<api>();
	}
};

template <typename api>
struct CTransformFeedbackElements : public CTransformFeedback
{
	virtual std::string Title()
	{
		return "Transform feedback: glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that transform feedback works correctly with glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Create data" NL "2. Use data as input to glDrawElementsIndirect" NL "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CTransformFeedbackElements() : CTransformFeedback(DRAW_ELEMENTS)
	{
	}

	virtual long Run()
	{
		return CTransformFeedback::Run<api>();
	}
};

class CComputeBase : public DrawIndirectBase
{
public:
	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	template <typename api>
	long Run()
	{

		int width, height;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &width);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &height);

		width  = std::min(width, getWindowWidth());
		height = std::min(height, getWindowHeight());

		glViewport(0, 0, width, height);

		CColorArray coords(width * height, tcu::Vec4(0));
		CColorArray colors(width * height, tcu::Vec4(0));

		_program = CreateProgram(Vsh<api>(), "", Fsh<api>(), false);
		glBindAttribLocation(_program, 0, "in_coords");
		glBindAttribLocation(_program, 1, "in_colors");
		glLinkProgram(_program);
		if (!CheckProgram(_program))
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_bufferCoords);
		glBindBuffer(GL_ARRAY_BUFFER, _bufferCoords);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), 0, GL_STREAM_DRAW);

		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(tcu::Vec4), 0);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &_bufferColors);
		glBindBuffer(GL_ARRAY_BUFFER, _bufferColors);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(colors.size() * sizeof(colors[0])), 0, GL_STREAM_DRAW);

		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(tcu::Vec4), 0);
		glEnableVertexAttribArray(1);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		DrawArraysIndirectCommand   indirectArrays   = { 0, 0, 0, 0 };

		CElementArray elements(width * height, 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);
		}
		break;
		case DRAW_ELEMENTS:
		{
			glGenBuffers(1, &_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
						 GL_STATIC_DRAW);

			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements,
						 GL_STATIC_DRAW);
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		_programCompute = CreateComputeProgram(Csh<api>(), false);
		glLinkProgram(_programCompute);
		if (!CheckProgram(_programCompute))
		{
			return ERROR;
		}
		glUseProgram(_programCompute);
		glUniform1ui(glGetUniformLocation(_programCompute, "width"), width);
		glUniform1ui(glGetUniformLocation(_programCompute, "height"), height);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _bufferCoords);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _bufferColors);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _bufferIndirect);

		glDispatchCompute(width, height, 1);
		glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		glUseProgram(_program);

		switch (_drawFunc)
		{
		case DRAW_ARRAYS:
		{
			glDrawArraysIndirect(GL_POINTS, 0);
		}
		break;
		case DRAW_ELEMENTS:
		{
			glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, 0);
		}
		break;
		default:
			throw std::runtime_error("Unknown draw function!");
			break;
		}

		CColorArray bufferRef1(width * height / 4, tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		CColorArray bufferRef2(width * height / 4, tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f));
		CColorArray bufferRef3(width * height / 4, tcu::Vec4(0.0f, 0.5f, 0.0f, 1.0f));
		CColorArray bufferRef4(width * height / 4, tcu::Vec4(0.5f, 0.5f, 0.0f, 1.0f));
		CColorArray bufferTest(width * height / 4, tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, width / 2, height / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, width / 2, height / 2, bufferRef1, width / 2, height / 2))
			<< "Region 0 verification failed";

		ReadPixelsFloat<api>((width + 1) / 2, 0, width / 2, height / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, width / 2, height / 2, bufferRef2, width / 2, height / 2))
			<< "Region 1 verification failed";

		ReadPixelsFloat<api>(0, (height + 1) / 2, width / 2, height / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, width / 2, height / 2, bufferRef3, width / 2, height / 2))
			<< "Region 2 verification failed";

		ReadPixelsFloat<api>((width + 1) / 2, (height + 1) / 2, width / 2, height / 2, &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, width / 2, height / 2, bufferRef4, width / 2, height / 2))
			<< "Region 3 verification failed";

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);
		glDeleteProgram(_program);
		glDeleteProgram(_programCompute);
		glDeleteVertexArrays(1, &_vao);
		if (_ebo)
			glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferCoords);
		glDeleteBuffers(1, &_bufferColors);
		glDeleteBuffers(1, &_bufferIndirect);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		return NO_ERROR;
	}
	CComputeBase(TDrawFunction drawFunc)
		: _drawFunc(drawFunc)
		, _program(0)
		, _programCompute(0)
		, _vao(0)
		, _ebo(0)
		, _bufferCoords(0)
		, _bufferColors(0)
		, _bufferIndirect(0)
	{
	}

private:
	CComputeBase();
	TDrawFunction _drawFunc;

	template <typename api>
	std::string Vsh()
	{
		return api::glslVer() + NL "in vec4 in_coords;" NL "in vec4 in_colors;" NL "out vec4 colors;" NL
								   "void main() {" NL "  colors = in_colors;" NL "  gl_Position = in_coords;" NL
								   "#if defined(GL_ES)" NL "  gl_PointSize = 1.0;" NL "#endif" NL "}";
	}

	template <typename api>
	std::string Fsh()
	{
		return api::glslVer() + NL "precision highp float;" NL "in vec4 colors;" NL "out vec4 outColor;" NL
								   "void main() {" NL "  outColor = colors;" NL "}";
	}

	template <typename api>
	std::string Csh()
	{
		return api::glslVer(true) + NL "precision highp int;                                           " NL
									   "precision highp float;                                         " NL
									   "                                                               " NL
									   "layout(local_size_x = 1) in;                                   " NL
									   "layout(std430, binding = 0) buffer Vertices {                  " NL
									   "    vec4 vertices[];                                           " NL
									   "};                                                             " NL
									   "layout(std430, binding = 1) buffer Colors {                    " NL
									   "    vec4 colors[];                                             " NL
									   "};                                                             " NL
									   "layout(std430, binding = 2) buffer Indirect {                  " NL
									   "    uint indirect[4];                                          " NL
									   "};                                                             " NL
									   "                                                               " NL
									   "uniform uint height;                                           " NL
									   "uniform uint width;                                            " NL
									   "                                                               " NL
									   "void main() {                                                  " NL
									   "    uint w = gl_GlobalInvocationID.x;                          " NL
									   "    uint h = gl_GlobalInvocationID.y;                          " NL
									   "    float stepX = 2.0 / float(width);                          " NL
									   "    float stepY = 2.0 / float(height);                         " NL
									   "    float offsetX = -1.0 + stepX * float(w) + stepX / 2.0;     " NL
									   "    float offsetY = -1.0 + stepY * float(h) + stepY / 2.0;     " NL
									   "    uint arrayOffset = h * width + w;                          " NL
									   "    vertices[ arrayOffset ] = vec4(offsetX, offsetY, 0.0, 1.0);" NL
									   "    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);                     " NL
									   "    if(w > (width / 2u - 1u)) {                                " NL
									   "        color = color + vec4(0.5, 0.0, 0.0, 0.0);              " NL
									   "    }                                                          " NL
									   "    if(h > (height / 2u - 1u)) {                               " NL
									   "        color = color + vec4(0.0, 0.5, 0.0, 0.0);              " NL
									   "    }                                                          " NL
									   "    colors[ arrayOffset ] = color;                             " NL
									   "    if(w == 0u && h == 0u) {                                   " NL
									   "        indirect[0] = width * height;                          " NL
									   "        indirect[1] =  1u;                                     " NL
									   "    }                                                          " NL
									   "}                                                              ";
	}

	GLuint _program, _programCompute;
	GLuint _vao;
	GLuint _ebo;
	GLuint _bufferCoords;
	GLuint _bufferColors;
	GLuint _bufferIndirect;
};

template <typename api>
struct CComputeShaderArray : public CComputeBase
{
	virtual std::string Title()
	{
		return "Compute Shader: glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that data created by Compute Shader can be used as an input to glDrawArrayIndirect";
	}

	virtual std::string Method()
	{
		return "1. Create data by Compute Shader" NL "2. Use data as input to glDrawArrayIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CComputeShaderArray() : CComputeBase(DRAW_ARRAYS)
	{
	}

	virtual long Run()
	{
		return CComputeBase::Run<api>();
	}
};

template <typename api>
struct CComputeShaderElements : public CComputeBase
{
	virtual std::string Title()
	{
		return "Compute Shader: glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that data created by Compute Shader can be used as an input to glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Create data by Compute Shader" NL "2. Use data as input to glDrawElementsIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	CComputeShaderElements() : CComputeBase(DRAW_ELEMENTS)
	{
	}

	virtual long Run()
	{
		return CComputeBase::Run<api>();
	}
};

template <typename api>
class CPrimitiveRestartElements : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Primitive restart - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that primitive restart works correctly with glDrawElementsIndirect";
	}

	virtual std::string Method()
	{
		return "1. Define primitives using VBO" NL "2. Draw primitives using glDrawElementsIndirect" NL
			   "3. Verify results";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	int PrimitiveRestartIndex();

	void EnablePrimitiveRestart();

	void DisablePrimitiveRestart();

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords1;
		TriangleStipGen(coords1, -1.0f, 1.0f, -1.0, -0.5, 2);

		CColorArray coords2;
		TriangleStipGen(coords2, -1.0f, 1.0f, 0.5, 1.0, 4);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords1.size() + coords2.size()) * sizeof(coords1[0]), NULL,
					 GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(coords1.size() * sizeof(coords1[0])), &coords1[0]);
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(coords1.size() * sizeof(coords1[0])),
						(GLsizeiptr)(coords2.size() * sizeof(coords2[0])), &coords2[0]);
		glVertexAttribPointer(0, sizeof(coords1[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords1[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords1.size() + coords2.size() + 1);
		indirectElements.primCount					 = static_cast<GLuint>((coords1.size() + coords2.size()) / 2);

		CElementArray elements;
		for (size_t i = 0; i < coords1.size(); ++i)
		{
			elements.push_back(static_cast<GLuint>(i));
		}

		elements.push_back(PrimitiveRestartIndex());
		for (size_t i = 0; i < coords2.size(); ++i)
		{
			elements.push_back(static_cast<GLuint>(coords1.size() + i));
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		EnablePrimitiveRestart();

		glDrawElementsIndirect(GL_TRIANGLE_STRIP, GL_UNSIGNED_INT, 0);

		CColorArray bufferRef1(getWindowWidth() * getWindowHeight() / 4, tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferRef2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f));
		CColorArray bufferTest1(getWindowWidth() * getWindowHeight() / 4, tcu::Vec4(0.0f));
		CColorArray bufferTest2(getWindowWidth() * getWindowHeight() / 2, tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight() / 4, &bufferTest1[0]);
		result.sub_result(BuffersCompare(bufferTest1, getWindowWidth(), getWindowHeight() / 4, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 4));

		ReadPixelsFloat<api>(0, (getWindowHeight() + 3) / 4, getWindowWidth(), getWindowHeight() / 2, &bufferTest2[0]);
		result.sub_result(BuffersCompare(bufferTest2, getWindowWidth(), getWindowHeight() / 2, bufferRef2,
										 getWindowWidth(), getWindowHeight() / 2));

		ReadPixelsFloat<api>(0, (getWindowHeight() * 3 + 3) / 4, getWindowWidth(), getWindowHeight() / 4,
							 &bufferTest1[0]);
		result.sub_result(BuffersCompare(bufferTest1, getWindowWidth(), getWindowHeight() / 4, bufferRef1,
										 getWindowWidth(), getWindowHeight() / 4));

		return result.code();
	}

	virtual long Cleanup()
	{

		DisablePrimitiveRestart();
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	void TriangleStipGen(CColorArray& coords, float widthStart, float widthEnd, float heightStart, float heightEnd,
						 unsigned int primNum)
	{
		float widthStep  = (widthEnd - widthStart) / static_cast<float>(primNum);
		float heightStep = (heightEnd - heightStart) / static_cast<float>(primNum);
		for (unsigned int i = 0; i < primNum; ++i)
		{
			float heightOffset = heightStart + heightStep * static_cast<float>(i);
			for (unsigned int j = 0; j < primNum; ++j)
			{
				float widthOffset = widthStart + widthStep * static_cast<float>(j);

				coords.push_back(tcu::Vec4(widthOffset, heightOffset, 0.0f, 1.0f));
				coords.push_back(tcu::Vec4(widthOffset, heightOffset + heightStep, 0.0f, 1.0f));
				coords.push_back(tcu::Vec4(widthOffset + widthStep, heightOffset, 0.0f, 1.0f));
				coords.push_back(tcu::Vec4(widthOffset + widthStep, heightOffset + heightStep, 0.0f, 1.0f));
			}
		}
	}
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <>
int CPrimitiveRestartElements<test_api::ES3>::PrimitiveRestartIndex()
{
	return 0xffffffff;
}

template <>
int CPrimitiveRestartElements<test_api::GL>::PrimitiveRestartIndex()
{
	return 3432432;
}

template <>
void CPrimitiveRestartElements<test_api::ES3>::DisablePrimitiveRestart()
{
	glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
}

template <>
void CPrimitiveRestartElements<test_api::GL>::DisablePrimitiveRestart()
{
	glDisable(GL_PRIMITIVE_RESTART);
}

template <>
void CPrimitiveRestartElements<test_api::ES3>::EnablePrimitiveRestart()
{
	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
}

template <>
void CPrimitiveRestartElements<test_api::GL>::EnablePrimitiveRestart()
{
	glPrimitiveRestartIndex(PrimitiveRestartIndex());
	glEnable(GL_PRIMITIVE_RESTART);
}

template <typename api>
class CNonZeroReservedMustBeZeroArray : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "non-zero reservedMustBeZero - glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawArrayIndirect with non-zero ReservedMustBeZero";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;
		indirectArrays.first					 = 0;
		indirectArrays.reservedMustBeZero		 = 2312;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		glDrawArraysIndirect(GL_TRIANGLES, 0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
struct CNonZeroReservedMustBeZeroElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "non-zero reservedMustBeZero - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawElementsIndirect with non-zero ReservedMustBeZero";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if no OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;
		indirectElements.baseVertex					 = 0;
		indirectElements.firstIndex					 = 0;
		indirectElements.reservedMustBeZero			 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));

		DIResult result;
		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeZeroBufferArray : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: no indirect buffer/parameter - glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawArrayIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);

		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		glDrawArraysIndirect(GL_TRIANGLES, 0);
		DIResult result;
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer;
};

template <typename api>
struct CNegativeZeroBufferElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: no indirect buffer/parameter - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawElementsIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

		DIResult result;
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo;
};

template <typename api>
struct CNegativeInvalidModeArray : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: invalid mode - glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Set invalid mode to glDrawArrayIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		glDrawArraysIndirect(GL_FLOAT, 0);

		DIResult result;
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_FLOAT as mode";
		}

		glDrawArraysIndirect(GL_STATIC_DRAW, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_STATIC_DRAW as mode";
		}

		glDrawArraysIndirect(GL_DRAW_INDIRECT_BUFFER, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_DRAW_INDIRECT_BUFFER as mode";
		}

		glDrawArraysIndirect(GL_INVALID_ENUM, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_ENUM as mode";
		}

		glDrawArraysIndirect(GL_COLOR_BUFFER_BIT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_COLOR_BUFFER_BIT as mode";
		}

		glDrawArraysIndirect(GL_ARRAY_BUFFER, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_ARRAY_BUFFER as mode";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
struct CNegativeInvalidModeElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: invalid mode - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Set invalid mode to glDrawElemenetsIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;
		glDrawElementsIndirect(GL_INVALID_ENUM, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_FLOAT as mode";
		}

		glDrawElementsIndirect(GL_UNSIGNED_INT, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_UNSIGNED_INT as mode";
		}

		glDrawElementsIndirect(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_ELEMENT_ARRAY_BUFFER as mode";
		}

		glDrawElementsIndirect(GL_FASTEST, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_FASTEST as mode";
		}

		glDrawElementsIndirect(GL_PACK_ALIGNMENT, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_PACK_ALIGNMENT as mode";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeNoVAOArrays : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: no VAO - glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Use glDrawArraysIndirect with default VAO";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		DIResult result;
		glDrawArraysIndirect(GL_TRIANGLES, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver";
		}

		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
			glDisableVertexAttribArray(0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{

		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
			glDisableVertexAttribArray(0);

		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeNoVAOElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: no VAO - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Use glDrawElemenetsIndirect with default VAO";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;
		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver";
		}

		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
			glDisableVertexAttribArray(0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeNoVBOArrays : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: no VBO - glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Use glDrawArraysIndirect with enabled vertex array, that has no VBO bound";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		DIResult result;
		glDrawArraysIndirect(GL_TRIANGLES, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver";
		}

		glDisableVertexAttribArray(0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeNoVBOElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: no VBO - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Use glDrawElementsIndirect with enabled vertex array, that has no VBO bound";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{

		api::ES_Only();

		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;
		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver";
		}

		glDisableVertexAttribArray(0);

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeBufferMappedArray : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: buffer mapped - glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "1. Create and bind buffer" NL "2. Map buffer" NL "3. Call glDrawArrayIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		DIResult result;
		void*	buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysIndirectCommand), GL_MAP_READ_BIT);
		if (buf == 0)
		{
			result.error() << "glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_MAP_READ_BIT) returned NULL";
		}

		glDrawArraysIndirect(GL_TRIANGLES, 0);

		GLenum error = glGetError();
		if (error == GL_INVALID_OPERATION)
		{
			//GL error: nothing is rendered
			CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
			CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

			ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
			result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef,
											 getWindowWidth(), getWindowHeight()));
		}
		else if (error == GL_NO_ERROR)
		{
			//No GL error: undefined
		}
		else
		{
			result.error() << "Invalid error code returned by a driver";
		}

		if (buf)
		{
			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) != GL_TRUE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
struct CNegativeBufferMappedElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: buffer mapped - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "1. Create and bind buffer" NL "2. Map buffer" NL "3. Call glDrawElementsIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;
		void* buf = glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT);
		if (buf == 0)
		{
			result.error() << "glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_MAP_WRITE_BIT) returned NULL";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);

		GLenum error = glGetError();
		if (error == GL_INVALID_OPERATION)
		{
			//GL error: nothing is rendered
			CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
			CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

			ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
			result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef,
											 getWindowWidth(), getWindowHeight()));
		}
		else if (error == GL_NO_ERROR)
		{
			//No GL error: undefined
		}
		else
		{
			result.error() << "Invalid error code returned by a driver";
		}

		if (buf)
		{
			if (glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) != GL_TRUE)
			{
				result.error() << "glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) returned GL_FALSE, expected GL_TRUE";
			}
			buf = 0;
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeDataWrongElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: invalid type - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "1. Bind non-zero buffer" NL "2. Call glDrawElementsIndirect with invalid type";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;

		glDrawElementsIndirect(GL_TRIANGLES, GL_FLOAT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_FLOAT type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_INT type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_STATIC_DRAW, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_STATIC_DRAW type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_SHORT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_SHORT type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_BYTE, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_BYTE type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_DOUBLE, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_DOUBLE type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_INVALID_ENUM, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_ENUM type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
class CNegativeGshArray : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Negative: incompatible the input primitive type of gsh - glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "1. Bind non-zero buffer" NL "2. Set data" NL "3. Set wrong geometry shader" NL
			   "4. Call glDrawArrayIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(Vsh(), Gsh(), Fsh(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;
		indirectArrays.first					 = 0;
		indirectArrays.reservedMustBeZero		 = 2312;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		DIResult result;

		glDrawArraysIndirect(GL_POINTS, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	std::string Vsh()
	{
		return "#version 150" NL "in vec4 coords;" NL "void main() {" NL "  gl_Position = coords;" NL "}";
	}

	std::string Gsh()
	{
		return "#version 150" NL "layout(triangles) in;" NL "layout(triangle_strip, max_vertices = 10) out;" NL
			   "void main() {" NL " for (int i=0; i<gl_in.length(); ++i) {" NL
			   "     gl_Position = gl_in[i].gl_Position;" NL "     EmitVertex();" NL " }" NL "}";
	}

	std::string Fsh()
	{
		return "#version 140" NL "out vec4 outColor;" NL "void main() {" NL
			   "  outColor = vec4(0.1f, 0.2f, 0.3f, 1.0f);" NL "}";
	}
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
class CNegativeGshElements : public DrawIndirectBase
{
public:
	virtual std::string Title()
	{
		return "Negative: incompatible the input primitive type of gsh - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "1. Bind non-zero buffer" NL "2. Set data" NL "3. Set wrong geometry shader" NL
			   "4. Call glDrawElementsIndirect";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(Vsh(), Gsh(), Fsh(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	std::string Vsh()
	{
		return "#version 150" NL "in vec4 coords;" NL "void main() {" NL "  gl_Position = coords;" NL "}";
	}

	std::string Gsh()
	{
		return "#version 150" NL "layout(lines) in;" NL "layout(line_strip, max_vertices = 10) out;" NL
			   "void main() {" NL " for (int i=0; i<gl_in.length(); ++i) {" NL
			   "     gl_Position = gl_in[i].gl_Position;" NL "     EmitVertex();" NL " }" NL "}";
	}

	std::string Fsh()
	{
		return "#version 140" NL "out vec4 outColor;" NL "void main() {" NL
			   "  outColor = vec4(0.1f, 0.2f, 0.3f, 1.0f);" NL "}";
	}
	int	_program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeInvalidSizeArrays : public DrawIndirectBase
{
	struct TWrongStructure1
	{
		GLuint count;
		GLuint primCount;
	};

	struct TWrongStructure2
	{
		GLfloat count;
		GLuint  primCount;
	};

	virtual std::string Title()
	{
		return "Negative: wrong structure - glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawArrayIndirect with wrong structure";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		TWrongStructure1 indirectArrays = { 0, 0 };
		indirectArrays.count			= static_cast<GLuint>(coords.size());
		indirectArrays.primCount		= 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(TWrongStructure1), &indirectArrays, GL_STATIC_DRAW);

		DIResult result;

		glDrawArraysIndirect(GL_TRIANGLES, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		glDeleteBuffers(1, &_bufferIndirect);

		TWrongStructure2 indirectArrays2 = { 0, 0 };
		indirectArrays2.count			 = static_cast<GLfloat>(coords.size());
		indirectArrays2.primCount		 = 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(TWrongStructure2), &indirectArrays2, GL_STATIC_DRAW);

		glDrawArraysIndirect(GL_TRIANGLES, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
struct CNegativeInvalidSizeElements : public DrawIndirectBase
{
	struct TWrongStructure
	{
		GLfloat count;
		GLuint  primCount;
	};

	virtual std::string Title()
	{
		return "Negative: wrong structure - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawElementsIndirect with wrong structure";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectElements = { 0, 0, 0, 0 };
		indirectElements.count					   = static_cast<GLuint>(coords.size());
		indirectElements.primCount				   = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		TWrongStructure indirectElements2 = { 0, 0 };
		indirectElements2.count			  = static_cast<GLfloat>(coords.size());
		indirectElements2.primCount		  = 1;

		glDeleteBuffers(1, &_bufferIndirect);
		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(TWrongStructure), &indirectElements2, GL_STATIC_DRAW);

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeStructureWrongOffsetArray : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: wrong offset - glDrawArrayIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawArrayIndirect with wrong offset";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawArraysIndirectCommand indirectArrays = { 0, 0, 0, 0 };
		indirectArrays.count					 = static_cast<GLuint>(coords.size());
		indirectArrays.primCount				 = 1;

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &indirectArrays, GL_STATIC_DRAW);

		DIResult result;

		glDrawArraysIndirect(GL_TRIANGLES, (void*)(sizeof(DrawArraysIndirectCommand) * 2));
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		glDrawArraysIndirect(GL_TRIANGLES, (void*)(sizeof(GLuint)));
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _bufferIndirect;
};

template <typename api>
struct CNegativeStructureWrongOffsetElements : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: wrong offset - glDrawElementsIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call glDrawElementsIndirect with wrong structure";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Setup()
	{
		glClear(GL_COLOR_BUFFER_BIT);
		return NO_ERROR;
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		CColorArray coords;
		PrimitiveGen(GL_TRIANGLES, 8, 8, coords);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(coords.size() * sizeof(coords[0])), &coords[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, sizeof(coords[0]) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(coords[0]), 0);
		glEnableVertexAttribArray(0);

		DrawElementsIndirectCommand indirectElements = { 0, 0, 0, 0, 0 };
		indirectElements.count						 = static_cast<GLuint>(coords.size());
		indirectElements.primCount					 = 1;

		CElementArray elements(coords.size(), 0);
		for (size_t i = 0; i < elements.size(); ++i)
		{
			elements[i] = static_cast<GLuint>(i);
		}

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirectElements, GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(elements.size() * sizeof(elements[0])), &elements[0],
					 GL_STATIC_DRAW);

		DIResult result;

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(sizeof(DrawElementsIndirectCommand) * 2));
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(sizeof(GLuint)));
		if (glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}

		CColorArray bufferRef(getWindowWidth() * getWindowHeight(), tcu::Vec4(0.0f));
		CColorArray bufferTest(getWindowWidth() * getWindowHeight(), tcu::Vec4(1.0f));

		ReadPixelsFloat<api>(0, 0, getWindowWidth(), getWindowHeight(), &bufferTest[0]);
		result.sub_result(BuffersCompare(bufferTest, getWindowWidth(), getWindowHeight(), bufferRef, getWindowWidth(),
										 getWindowHeight()));

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeUnalignedOffset : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: unaligned offset - glDrawElementsIndirect and glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no system/driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call with unaligned offset (1, 3, 1023)";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Run()
	{
		_program = CreateProgram(shaders::vshSimple<api>(), "", shaders::fshSimple<api>(), true);
		if (!_program)
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		std::vector<GLuint> zarro(4096, 0);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);
		glBufferData(GL_ARRAY_BUFFER, 4096, &zarro[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, 4096, &zarro[0], GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4096, &zarro[0], GL_STATIC_DRAW);

		DIResult result;

		int offsets[] = { 1, 3, 1023 };
		for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
		{
			glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, reinterpret_cast<void*>((deUintptr)offsets[i]));
			if (glGetError() != GL_INVALID_VALUE)
			{
				result.error() << "Invalid error code returned by a driver for GL_INVALID_VALUE type";
			}
			glDrawArraysIndirect(GL_TRIANGLES, reinterpret_cast<void*>((deUintptr)offsets[i]));
			if (glGetError() != GL_INVALID_VALUE)
			{
				result.error() << "Invalid error code returned by a driver for GL_INVALID_VALUE type";
			}
		}

		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		return NO_ERROR;
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect;
};

template <typename api>
struct CNegativeXFB : public DrawIndirectBase
{
	virtual std::string Title()
	{
		return "Negative: transform feedback active and not paused - glDrawElementsIndirect and glDrawArraysIndirect";
	}

	virtual std::string Purpose()
	{
		return "Verify that a driver sets error and no system/driver crash occurred";
	}

	virtual std::string Method()
	{
		return "Call with transform feedback active";
	}

	virtual std::string PassCriteria()
	{
		return "The test will pass if OpenGL errors reported and no driver crash occurred";
	}

	virtual long Run()
	{
		api::ES_Only();

		bool drawWithXFBAllowed = m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader");

		_program				 = CreateProgram(Vsh(), "", shaders::fshSimple<api>(), false);
		const GLchar* varyings[] = { "dataOut" };
		glTransformFeedbackVaryings(_program, 1, varyings, GL_INTERLEAVED_ATTRIBS);
		glLinkProgram(_program);
		if (!CheckProgram(_program))
		{
			return ERROR;
		}
		glUseProgram(_program);

		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);

		std::vector<GLuint> zarro(4096, 0);

		glGenBuffers(1, &_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer);
		glBufferData(GL_ARRAY_BUFFER, 4096, &zarro[0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &_bufferIndirect);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _bufferIndirect);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, 4096, &zarro[0], GL_STATIC_DRAW);

		glGenBuffers(1, &_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4096, &zarro[0], GL_STATIC_DRAW);

		glGenBuffers(1, &_xfb);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, _xfb);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 4096, &zarro[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _xfb);

		DIResult result;

		//Without XFO
		glBeginTransformFeedback(GL_POINTS);

		glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, NULL);
		if (!drawWithXFBAllowed && glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}
		glDrawArraysIndirect(GL_POINTS, NULL);
		if (!drawWithXFBAllowed && glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}
		glEndTransformFeedback();

		//With XFO
		glGenTransformFeedbacks(1, &_xfo);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _xfo);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _xfb);
		glBeginTransformFeedback(GL_POINTS);
		glPauseTransformFeedback();
		glResumeTransformFeedback();

		glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, NULL);
		if (!drawWithXFBAllowed && glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}
		glDrawArraysIndirect(GL_POINTS, NULL);
		if (!drawWithXFBAllowed && glGetError() != GL_INVALID_OPERATION)
		{
			result.error() << "Invalid error code returned by a driver for GL_INVALID_OPERATION type";
		}
		glEndTransformFeedback();

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		return result.code();
	}

	virtual long Cleanup()
	{
		glDisableVertexAttribArray(0);
		glUseProgram(0);
		glDeleteProgram(_program);
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_buffer);
		glDeleteBuffers(1, &_ebo);
		glDeleteBuffers(1, &_bufferIndirect);
		glDeleteBuffers(1, &_xfb);
		glDeleteTransformFeedbacks(1, &_xfo);
		return NO_ERROR;
	}

	std::string Vsh()
	{
		return api::glslVer() + NL "out highp vec4 dataOut;" NL "in vec4 i_vertex;" NL "void main() {" NL
								   "  dataOut = i_vertex;" NL "  gl_Position = i_vertex;" NL "}";
	}

private:
	GLuint _program;
	GLuint _vao, _buffer, _ebo, _bufferIndirect, _xfo, _xfb;
};

} // namespace
DrawIndirectTestsGL40::DrawIndirectTestsGL40(glcts::Context& context) : TestCaseGroup(context, "draw_indirect", "")
{
}

DrawIndirectTestsGL40::~DrawIndirectTestsGL40(void)
{
}

void DrawIndirectTestsGL40::init()
{
	using namespace glcts;

	DILogger::setOutput(m_context.getTestContext().getLog());

	addChild(
		new TestSubcase(m_context, "basic-binding-default", TestSubcase::Create<CDefaultBindingPoint<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-binding-zero", TestSubcase::Create<CZeroBindingPoint<test_api::GL> >));
	addChild(
		new TestSubcase(m_context, "basic-binding-single", TestSubcase::Create<CSingleBindingPoint<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-binding-multi", TestSubcase::Create<CMultiBindingPoint<test_api::GL> >));
	addChild(
		new TestSubcase(m_context, "basic-binding-delete", TestSubcase::Create<CDeleteBindingPoint<test_api::GL> >));

	addChild(new TestSubcase(m_context, "basic-buffer-data", TestSubcase::Create<CBufferData<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-buffer-subData", TestSubcase::Create<CBufferSubData<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-buffer-unMap", TestSubcase::Create<CBufferMap<test_api::GL> >));
	addChild(
		new TestSubcase(m_context, "basic-buffer-getPointerv", TestSubcase::Create<CBufferGetPointerv<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-buffer-mapRange", TestSubcase::Create<CBufferMapRange<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-buffer-flushMappedRange",
							 TestSubcase::Create<CBufferFlushMappedRange<test_api::GL> >));
	addChild(
		new TestSubcase(m_context, "basic-buffer-copySubData", TestSubcase::Create<CBufferCopySubData<test_api::GL> >));

	addChild(new TestSubcase(m_context, "basic-drawArrays-singlePrimitive",
							 TestSubcase::Create<CVBODrawArraysSingle<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-manyPrimitives",
							 TestSubcase::Create<CVBODrawArraysMany<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-instancing",
							 TestSubcase::Create<CVBODrawArraysInstancing<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-xfbPaused",
							 TestSubcase::Create<CVBODrawArraysXFBPaused<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-singlePrimitive",
							 TestSubcase::Create<CVBODrawElementsSingle<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-manyPrimitives",
							 TestSubcase::Create<CVBODrawElementsMany<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-instancing",
							 TestSubcase::Create<CVBODrawElementsInstancing<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-xfbPaused",
							 TestSubcase::Create<CVBODrawArraysXFBPaused<test_api::GL> >));

	addChild(new TestSubcase(m_context, "basic-drawArrays-simple",
							 TestSubcase::Create<CBufferIndirectDrawArraysSimple<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-noFirst",
							 TestSubcase::Create<CBufferIndirectDrawArraysNoFirst<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-bufferOffset",
							 TestSubcase::Create<CBufferIndirectDrawArraysOffset<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-vertexIds",
							 TestSubcase::Create<CBufferIndirectDrawArraysVertexIds<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-simple",
							 TestSubcase::Create<CBufferIndirectDrawElementsSimple<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-noFirstIndex",
							 TestSubcase::Create<CBufferIndirectDrawElementsNoFirstIndex<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-basevertex",
							 TestSubcase::Create<CBufferIndirectDrawElementsNoBasevertex<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-bufferOffset",
							 TestSubcase::Create<CBufferIndirectDrawElementsOffset<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-vertexIds",
							 TestSubcase::Create<CBufferIndirectDrawElementsVertexIds<test_api::GL> >));

	addChild(new TestSubcase(m_context, "basic-indicesDataType-unsigned_short",
							 TestSubcase::Create<CIndicesDataTypeUnsignedShort<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-indicesDataType-unsigned_byte",
							 TestSubcase::Create<CIndicesDataTypeUnsignedByte<test_api::GL> >));

	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-points",
							 TestSubcase::Create<CModeDrawArraysPoints<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-lines",
							 TestSubcase::Create<CModeDrawArraysLines<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-line_strip",
							 TestSubcase::Create<CModeDrawArraysLineStrip<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-line_loop",
							 TestSubcase::Create<CModeDrawArraysLineLoop<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangle_strip",
							 TestSubcase::Create<CModeDrawArraysTriangleStrip<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangle_fan",
							 TestSubcase::Create<CModeDrawArraysTriangleFan<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-lines_adjacency",
							 TestSubcase::Create<CModeDrawArraysLinesAdjacency<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-line_strip_adjacency",
							 TestSubcase::Create<CModeDrawArraysLineStripAdjacency<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangles_adjacency",
							 TestSubcase::Create<CModeDrawArraysTrianglesAdjacency<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangle_strip_adjacency",
							 TestSubcase::Create<CModeDrawArraysTriangleStripAdjacency<test_api::GL> >));

	addChild(new TestSubcase(m_context, "basic-mode-drawElements-points",
							 TestSubcase::Create<CModeDrawElementsPoints<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-lines",
							 TestSubcase::Create<CModeDrawElementsLines<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-line_strip",
							 TestSubcase::Create<CModeDrawElementsLineStrip<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-line_loop",
							 TestSubcase::Create<CModeDrawElementsLineLoop<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangle_strip",
							 TestSubcase::Create<CModeDrawElementsTriangleStrip<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangle_fan",
							 TestSubcase::Create<CModeDrawElementsTriangleFan<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-lines_adjacency",
							 TestSubcase::Create<CModeDrawElementsLinesAdjacency<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-line_strip_adjacency",
							 TestSubcase::Create<CModeDrawElementsLineStripAdjacency<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangles_adjacency",
							 TestSubcase::Create<CModeDrawElementsTrianglesAdjacency<test_api::GL> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangle_strip_adjacency",
							 TestSubcase::Create<CModeDrawElementsTriangleStripAdjacency<test_api::GL> >));

	addChild(new TestSubcase(m_context, "advanced-twoPass-transformFeedback-arrays",
							 TestSubcase::Create<CTransformFeedbackArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "advanced-twoPass-transformFeedback-elements",
							 TestSubcase::Create<CTransformFeedbackElements<test_api::GL> >));

	addChild(new TestSubcase(m_context, "advanced-primitiveRestart-elements",
							 TestSubcase::Create<CPrimitiveRestartElements<test_api::GL> >));

	addChild(new TestSubcase(m_context, "misc-reservedMustBeZero-arrays",
							 TestSubcase::Create<CNonZeroReservedMustBeZeroArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "misc-reservedMustBeZero-elements",
							 TestSubcase::Create<CNonZeroReservedMustBeZeroElements<test_api::GL> >));

	addChild(new TestSubcase(m_context, "negative-noindirect-arrays",
							 TestSubcase::Create<CNegativeZeroBufferArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-noindirect-elements",
							 TestSubcase::Create<CNegativeZeroBufferElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-invalidMode-arrays",
							 TestSubcase::Create<CNegativeInvalidModeArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-invalidMode-elements",
							 TestSubcase::Create<CNegativeInvalidModeElements<test_api::GL> >));
	addChild(
		new TestSubcase(m_context, "negative-noVAO-arrays", TestSubcase::Create<CNegativeNoVAOArrays<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-noVAO-elements",
							 TestSubcase::Create<CNegativeNoVAOElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-bufferMapped-arrays",
							 TestSubcase::Create<CNegativeBufferMappedArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-bufferMapped-elements",
							 TestSubcase::Create<CNegativeBufferMappedElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-invalidType-elements",
							 TestSubcase::Create<CNegativeDataWrongElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-gshIncompatible-arrays",
							 TestSubcase::Create<CNegativeGshArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-gshIncompatible-elements",
							 TestSubcase::Create<CNegativeGshElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-wrongOffset-arrays",
							 TestSubcase::Create<CNegativeStructureWrongOffsetArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-wrongOffset-elements",
							 TestSubcase::Create<CNegativeStructureWrongOffsetElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-invalidSize-arrays",
							 TestSubcase::Create<CNegativeInvalidSizeArrays<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-invalidSize-elements",
							 TestSubcase::Create<CNegativeInvalidSizeElements<test_api::GL> >));
	addChild(new TestSubcase(m_context, "negative-unalignedOffset",
							 TestSubcase::Create<CNegativeUnalignedOffset<test_api::GL> >));
}

DrawIndirectTestsGL43::DrawIndirectTestsGL43(glcts::Context& context) : TestCaseGroup(context, "draw_indirect_43", "")
{
}

DrawIndirectTestsGL43::~DrawIndirectTestsGL43(void)
{
}

void DrawIndirectTestsGL43::init()
{
	using namespace glcts;

	DILogger::setOutput(m_context.getTestContext().getLog());
	addChild(new TestSubcase(m_context, "advanced-twoPass-Compute-arrays",
							 TestSubcase::Create<CComputeShaderArray<test_api::GL> >));
	addChild(new TestSubcase(m_context, "advanced-twoPass-Compute-elements",
							 TestSubcase::Create<CComputeShaderElements<test_api::GL> >));
}

DrawIndirectTestsES31::DrawIndirectTestsES31(glcts::Context& context) : TestCaseGroup(context, "draw_indirect", "")
{
}

DrawIndirectTestsES31::~DrawIndirectTestsES31(void)
{
}

void DrawIndirectTestsES31::init()
{
	using namespace glcts;

	DILogger::setOutput(m_context.getTestContext().getLog());

	addChild(
		new TestSubcase(m_context, "basic-binding-default", TestSubcase::Create<CDefaultBindingPoint<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-binding-zero", TestSubcase::Create<CZeroBindingPoint<test_api::ES3> >));
	addChild(
		new TestSubcase(m_context, "basic-binding-single", TestSubcase::Create<CSingleBindingPoint<test_api::ES3> >));
	addChild(
		new TestSubcase(m_context, "basic-binding-multi", TestSubcase::Create<CMultiBindingPoint<test_api::ES3> >));
	addChild(
		new TestSubcase(m_context, "basic-binding-delete", TestSubcase::Create<CDeleteBindingPoint<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "basic-buffer-data", TestSubcase::Create<CBufferData<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-buffer-subData", TestSubcase::Create<CBufferSubData<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-buffer-getPointerv",
							 TestSubcase::Create<CBufferGetPointerv<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-buffer-mapRange", TestSubcase::Create<CBufferMapRange<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-buffer-flushMappedRange",
							 TestSubcase::Create<CBufferFlushMappedRange<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-buffer-copySubData",
							 TestSubcase::Create<CBufferCopySubData<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "basic-drawArrays-singlePrimitive",
							 TestSubcase::Create<CVBODrawArraysSingle<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-manyPrimitives",
							 TestSubcase::Create<CVBODrawArraysMany<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-instancing",
							 TestSubcase::Create<CVBODrawArraysInstancing<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-xfbPaused",
							 TestSubcase::Create<CVBODrawArraysXFBPaused<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-singlePrimitive",
							 TestSubcase::Create<CVBODrawElementsSingle<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-manyPrimitives",
							 TestSubcase::Create<CVBODrawElementsMany<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-instancing",
							 TestSubcase::Create<CVBODrawElementsInstancing<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-xfbPaused",
							 TestSubcase::Create<CVBODrawArraysXFBPaused<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "basic-drawArrays-simple",
							 TestSubcase::Create<CBufferIndirectDrawArraysSimple<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-noFirst",
							 TestSubcase::Create<CBufferIndirectDrawArraysNoFirst<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-bufferOffset",
							 TestSubcase::Create<CBufferIndirectDrawArraysOffset<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawArrays-vertexIds",
							 TestSubcase::Create<CBufferIndirectDrawArraysVertexIds<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-simple",
							 TestSubcase::Create<CBufferIndirectDrawElementsSimple<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-noFirstIndex",
							 TestSubcase::Create<CBufferIndirectDrawElementsNoFirstIndex<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-basevertex",
							 TestSubcase::Create<CBufferIndirectDrawElementsNoBasevertex<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-bufferOffset",
							 TestSubcase::Create<CBufferIndirectDrawElementsOffset<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-drawElements-vertexIds",
							 TestSubcase::Create<CBufferIndirectDrawElementsVertexIds<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "basic-indicesDataType-unsigned_short",
							 TestSubcase::Create<CIndicesDataTypeUnsignedShort<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-indicesDataType-unsigned_byte",
							 TestSubcase::Create<CIndicesDataTypeUnsignedByte<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-points",
							 TestSubcase::Create<CModeDrawArraysPoints<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-lines",
							 TestSubcase::Create<CModeDrawArraysLines<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-line_strip",
							 TestSubcase::Create<CModeDrawArraysLineStrip<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-line_loop",
							 TestSubcase::Create<CModeDrawArraysLineLoop<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangle_strip",
							 TestSubcase::Create<CModeDrawArraysTriangleStrip<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangle_fan",
							 TestSubcase::Create<CModeDrawArraysTriangleFan<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-lines_adjacency",
							 TestSubcase::Create<CModeDrawArraysLinesAdjacency<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-line_strip_adjacency",
							 TestSubcase::Create<CModeDrawArraysLineStripAdjacency<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangles_adjacency",
							 TestSubcase::Create<CModeDrawArraysTrianglesAdjacency<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawArrays-triangle_strip_adjacency",
							 TestSubcase::Create<CModeDrawArraysTriangleStripAdjacency<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "basic-mode-drawElements-points",
							 TestSubcase::Create<CModeDrawElementsPoints<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-lines",
							 TestSubcase::Create<CModeDrawElementsLines<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-line_strip",
							 TestSubcase::Create<CModeDrawElementsLineStrip<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-line_loop",
							 TestSubcase::Create<CModeDrawElementsLineLoop<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangle_strip",
							 TestSubcase::Create<CModeDrawElementsTriangleStrip<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangle_fan",
							 TestSubcase::Create<CModeDrawElementsTriangleFan<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-lines_adjacency",
							 TestSubcase::Create<CModeDrawElementsLinesAdjacency<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-line_strip_adjacency",
							 TestSubcase::Create<CModeDrawElementsLineStripAdjacency<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangles_adjacency",
							 TestSubcase::Create<CModeDrawElementsTrianglesAdjacency<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "basic-mode-drawElements-triangle_strip_adjacency",
							 TestSubcase::Create<CModeDrawElementsTriangleStripAdjacency<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "advanced-twoPass-transformFeedback-arrays",
							 TestSubcase::Create<CTransformFeedbackArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "advanced-twoPass-transformFeedback-elements",
							 TestSubcase::Create<CTransformFeedbackElements<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "advanced-twoPass-Compute-arrays",
							 TestSubcase::Create<CComputeShaderArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "advanced-twoPass-Compute-elements",
							 TestSubcase::Create<CComputeShaderElements<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "advanced-primitiveRestart-elements",
							 TestSubcase::Create<CPrimitiveRestartElements<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "misc-reservedMustBeZero-arrays",
							 TestSubcase::Create<CNonZeroReservedMustBeZeroArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "misc-reservedMustBeZero-elements",
							 TestSubcase::Create<CNonZeroReservedMustBeZeroElements<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "negative-noindirect-arrays",
							 TestSubcase::Create<CNegativeZeroBufferArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-noindirect-elements",
							 TestSubcase::Create<CNegativeZeroBufferElements<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-invalidMode-arrays",
							 TestSubcase::Create<CNegativeInvalidModeArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-invalidMode-elements",
							 TestSubcase::Create<CNegativeInvalidModeElements<test_api::ES3> >));
	addChild(
		new TestSubcase(m_context, "negative-noVAO-arrays", TestSubcase::Create<CNegativeNoVAOArrays<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-noVAO-elements",
							 TestSubcase::Create<CNegativeNoVAOElements<test_api::ES3> >));
	addChild(
		new TestSubcase(m_context, "negative-noVBO-arrays", TestSubcase::Create<CNegativeNoVBOArrays<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-noVBO-elements",
							 TestSubcase::Create<CNegativeNoVBOElements<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-bufferMapped-arrays",
							 TestSubcase::Create<CNegativeBufferMappedArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-bufferMapped-elements",
							 TestSubcase::Create<CNegativeBufferMappedElements<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-invalidType-elements",
							 TestSubcase::Create<CNegativeDataWrongElements<test_api::ES3> >));

	addChild(new TestSubcase(m_context, "negative-wrongOffset-arrays",
							 TestSubcase::Create<CNegativeStructureWrongOffsetArray<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-wrongOffset-elements",
							 TestSubcase::Create<CNegativeStructureWrongOffsetElements<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-invalidSize-arrays",
							 TestSubcase::Create<CNegativeInvalidSizeArrays<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-invalidSize-elements",
							 TestSubcase::Create<CNegativeInvalidSizeElements<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-unalignedOffset",
							 TestSubcase::Create<CNegativeUnalignedOffset<test_api::ES3> >));
	addChild(new TestSubcase(m_context, "negative-xfb", TestSubcase::Create<CNegativeXFB<test_api::ES3> >));
}
}
