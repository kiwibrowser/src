/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief Compiler test case.
 */ /*-------------------------------------------------------------------*/

#include "glcShaderLibrary.hpp"
#include "glcShaderLibraryCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuResource.hpp"

#include "deInt32.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

using std::string;
using std::vector;
using std::ostringstream;

using namespace glu;

#if 0
#define PARSE_DBG(X) printf X
#else
#define PARSE_DBG(X) DE_NULL_STATEMENT
#endif

namespace deqp
{
namespace sl
{

static const glu::GLSLVersion DEFAULT_GLSL_VERSION = glu::GLSL_VERSION_100_ES;

DE_INLINE deBool isWhitespace(char c)
{
	return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

DE_INLINE deBool isEOL(char c)
{
	return (c == '\r') || (c == '\n');
}

DE_INLINE deBool isNumeric(char c)
{
	return deInRange32(c, '0', '9');
}

DE_INLINE deBool isAlpha(char c)
{
	return deInRange32(c, 'a', 'z') || deInRange32(c, 'A', 'Z');
}

DE_INLINE deBool isCaseNameChar(char c)
{
	return deInRange32(c, 'a', 'z') || deInRange32(c, 'A', 'Z') || deInRange32(c, '0', '9') || (c == '_') ||
		   (c == '-') || (c == '.');
}

// \todo [2011-02-11 pyry] Should not depend on Context or TestContext!
class ShaderParser
{
public:
	ShaderParser(tcu::TestContext& testCtx, RenderContext& renderCtx);
	~ShaderParser(void);

	vector<tcu::TestNode*> parse(const char* input);

private:
	enum Token
	{
		TOKEN_INVALID = 0,
		TOKEN_EOF,
		TOKEN_STRING,
		TOKEN_SHADER_SOURCE,

		TOKEN_INT_LITERAL,
		TOKEN_FLOAT_LITERAL,

		// identifiers
		TOKEN_IDENTIFIER,
		TOKEN_TRUE,
		TOKEN_FALSE,
		TOKEN_DESC,
		TOKEN_EXPECT,
		TOKEN_GROUP,
		TOKEN_CASE,
		TOKEN_END,
		TOKEN_VALUES,
		TOKEN_BOTH,
		TOKEN_VERTEX,
		TOKEN_FRAGMENT,
		TOKEN_UNIFORM,
		TOKEN_INPUT,
		TOKEN_OUTPUT,
		TOKEN_FLOAT,
		TOKEN_FLOAT_VEC2,
		TOKEN_FLOAT_VEC3,
		TOKEN_FLOAT_VEC4,
		TOKEN_FLOAT_MAT2,
		TOKEN_FLOAT_MAT2X3,
		TOKEN_FLOAT_MAT2X4,
		TOKEN_FLOAT_MAT3X2,
		TOKEN_FLOAT_MAT3,
		TOKEN_FLOAT_MAT3X4,
		TOKEN_FLOAT_MAT4X2,
		TOKEN_FLOAT_MAT4X3,
		TOKEN_FLOAT_MAT4,
		TOKEN_INT,
		TOKEN_INT_VEC2,
		TOKEN_INT_VEC3,
		TOKEN_INT_VEC4,
		TOKEN_UINT,
		TOKEN_UINT_VEC2,
		TOKEN_UINT_VEC3,
		TOKEN_UINT_VEC4,
		TOKEN_BOOL,
		TOKEN_BOOL_VEC2,
		TOKEN_BOOL_VEC3,
		TOKEN_BOOL_VEC4,
		TOKEN_VERSION,

		// symbols
		TOKEN_ASSIGN,
		TOKEN_PLUS,
		TOKEN_MINUS,
		TOKEN_COMMA,
		TOKEN_VERTICAL_BAR,
		TOKEN_SEMI_COLON,
		TOKEN_LEFT_PAREN,
		TOKEN_RIGHT_PAREN,
		TOKEN_LEFT_BRACKET,
		TOKEN_RIGHT_BRACKET,
		TOKEN_LEFT_BRACE,
		TOKEN_RIGHT_BRACE,

		TOKEN_LAST
	};

	void parseError(const std::string& errorStr);
	float parseFloatLiteral(const char* str);
	long long int parseIntLiteral(const char* str);
	string parseStringLiteral(const char* str);
	string parseShaderSource(const char* str);
	void advanceToken(void);
	void advanceToken(Token assumed);
	void assumeToken(Token token);
	DataType mapDataTypeToken(Token token);
	const char* getTokenName(Token token);

	void parseValueElement(DataType dataType, ShaderCase::Value& result);
	void parseValue(ShaderCase::ValueBlock& valueBlock);
	void parseValueBlock(ShaderCase::ValueBlock& valueBlock);
	void parseShaderCase(vector<tcu::TestNode*>& shaderNodeList);
	void parseShaderGroup(vector<tcu::TestNode*>& shaderNodeList);

	// Member variables.
	tcu::TestContext& m_testCtx;
	RenderContext&	m_renderCtx;
	std::string		  m_input;
	const char*		  m_curPtr;
	Token			  m_curToken;
	std::string		  m_curTokenStr;
};

ShaderParser::ShaderParser(tcu::TestContext& testCtx, RenderContext& renderCtx)
	: m_testCtx(testCtx), m_renderCtx(renderCtx), m_curPtr(DE_NULL), m_curToken(TOKEN_LAST)
{
}

ShaderParser::~ShaderParser(void)
{
	// nada
}

void ShaderParser::parseError(const std::string& errorStr)
{
	string atStr = string(m_curPtr, 80);
	throw tcu::InternalError((string("Parser error: ") + errorStr + " near '" + atStr + " ...'").c_str(), "", __FILE__,
							 __LINE__);
}

float ShaderParser::parseFloatLiteral(const char* str)
{
	return (float)atof(str);
}

long long int ShaderParser::parseIntLiteral(const char* str)
{
	return strtoll(str, NULL, 0);
}

string ShaderParser::parseStringLiteral(const char* str)
{
	const char*   p		  = str;
	char		  endChar = *p++;
	ostringstream o;

	while (*p != endChar && *p)
	{
		if (*p == '\\')
		{
			switch (p[1])
			{
			case 0:
				DE_ASSERT(DE_FALSE);
				break;
			case 'n':
				o << '\n';
				break;
			case 't':
				o << '\t';
				break;
			default:
				o << p[1];
				break;
			}

			p += 2;
		}
		else
			o << *p++;
	}

	return o.str();
}

static string removeExtraIndentation(const string& source)
{
	// Detect indentation from first line.
	int numIndentChars = 0;
	for (int ndx = 0; isWhitespace(source[ndx]) && ndx < (int)source.length(); ndx++)
		numIndentChars += source[ndx] == '\t' ? 4 : 1;

	// Process all lines and remove preceding indentation.
	ostringstream processed;
	{
		bool atLineStart		= true;
		int  indentCharsOmitted = 0;

		for (int pos = 0; pos < (int)source.length(); pos++)
		{
			char c = source[pos];

			if (atLineStart && indentCharsOmitted < numIndentChars && (c == ' ' || c == '\t'))
			{
				indentCharsOmitted += c == '\t' ? 4 : 1;
			}
			else if (isEOL(c))
			{
				if (source[pos] == '\r' && source[pos + 1] == '\n')
				{
					pos += 1;
					processed << '\n';
				}
				else
					processed << c;

				atLineStart		   = true;
				indentCharsOmitted = 0;
			}
			else
			{
				processed << c;
				atLineStart = false;
			}
		}
	}

	return processed.str();
}

string ShaderParser::parseShaderSource(const char* str)
{
	const char*   p = str + 2;
	ostringstream o;

	// Eat first empty line from beginning.
	while (*p == ' ')
		p++;
	while (isEOL(*p))
		p++;

	while ((p[0] != '"') || (p[1] != '"'))
	{
		if (*p == '\\')
		{
			switch (p[1])
			{
			case 0:
				DE_ASSERT(DE_FALSE);
				break;
			case 'n':
				o << '\n';
				break;
			case 't':
				o << '\t';
				break;
			default:
				o << p[1];
				break;
			}

			p += 2;
		}
		else
			o << *p++;
	}

	return removeExtraIndentation(o.str());
}

void ShaderParser::advanceToken(void)
{
	// Skip old token.
	m_curPtr += m_curTokenStr.length();

	// Reset token (for safety).
	m_curToken	= TOKEN_INVALID;
	m_curTokenStr = "";

	// Eat whitespace & comments while they last.
	for (;;)
	{
		while (isWhitespace(*m_curPtr))
			m_curPtr++;

		// Check for EOL comment.
		if (*m_curPtr == '#')
		{
			while (*m_curPtr && !isEOL(*m_curPtr))
				m_curPtr++;
		}
		else
			break;
	}

	if (!*m_curPtr)
	{
		m_curToken	= TOKEN_EOF;
		m_curTokenStr = "<EOF>";
	}
	else if (isAlpha(*m_curPtr))
	{
		struct Named
		{
			const char* str;
			Token		token;
		};

		static const Named s_named[] = { { "true", TOKEN_TRUE },
										 { "false", TOKEN_FALSE },
										 { "desc", TOKEN_DESC },
										 { "expect", TOKEN_EXPECT },
										 { "group", TOKEN_GROUP },
										 { "case", TOKEN_CASE },
										 { "end", TOKEN_END },
										 { "values", TOKEN_VALUES },
										 { "both", TOKEN_BOTH },
										 { "vertex", TOKEN_VERTEX },
										 { "fragment", TOKEN_FRAGMENT },
										 { "uniform", TOKEN_UNIFORM },
										 { "input", TOKEN_INPUT },
										 { "output", TOKEN_OUTPUT },
										 { "float", TOKEN_FLOAT },
										 { "vec2", TOKEN_FLOAT_VEC2 },
										 { "vec3", TOKEN_FLOAT_VEC3 },
										 { "vec4", TOKEN_FLOAT_VEC4 },
										 { "mat2", TOKEN_FLOAT_MAT2 },
										 { "mat2x3", TOKEN_FLOAT_MAT2X3 },
										 { "mat2x4", TOKEN_FLOAT_MAT2X4 },
										 { "mat3x2", TOKEN_FLOAT_MAT3X2 },
										 { "mat3", TOKEN_FLOAT_MAT3 },
										 { "mat3x4", TOKEN_FLOAT_MAT3X4 },
										 { "mat4x2", TOKEN_FLOAT_MAT4X2 },
										 { "mat4x3", TOKEN_FLOAT_MAT4X3 },
										 { "mat4", TOKEN_FLOAT_MAT4 },
										 { "int", TOKEN_INT },
										 { "ivec2", TOKEN_INT_VEC2 },
										 { "ivec3", TOKEN_INT_VEC3 },
										 { "ivec4", TOKEN_INT_VEC4 },
										 { "uint", TOKEN_UINT },
										 { "uvec2", TOKEN_UINT_VEC2 },
										 { "uvec3", TOKEN_UINT_VEC3 },
										 { "uvec4", TOKEN_UINT_VEC4 },
										 { "bool", TOKEN_BOOL },
										 { "bvec2", TOKEN_BOOL_VEC2 },
										 { "bvec3", TOKEN_BOOL_VEC3 },
										 { "bvec4", TOKEN_BOOL_VEC4 },
										 { "version", TOKEN_VERSION } };

		const char* end = m_curPtr + 1;
		while (isCaseNameChar(*end))
			end++;
		m_curTokenStr = string(m_curPtr, end - m_curPtr);

		m_curToken = TOKEN_IDENTIFIER;

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_named); ndx++)
		{
			if (m_curTokenStr == s_named[ndx].str)
			{
				m_curToken = s_named[ndx].token;
				break;
			}
		}
	}
	else if (isNumeric(*m_curPtr))
	{
		/* \todo [2010-03-31 petri] Hex? */
		const char* p = m_curPtr;
		while (isNumeric(*p))
			p++;
		if (*p == '.')
		{
			p++;
			while (isNumeric(*p))
				p++;

			if (*p == 'e' || *p == 'E')
			{
				p++;
				if (*p == '+' || *p == '-')
					p++;
				DE_ASSERT(isNumeric(*p));
				while (isNumeric(*p))
					p++;
			}

			m_curToken	= TOKEN_FLOAT_LITERAL;
			m_curTokenStr = string(m_curPtr, p - m_curPtr);
		}
		else
		{
			m_curToken	= TOKEN_INT_LITERAL;
			m_curTokenStr = string(m_curPtr, p - m_curPtr);
		}
	}
	else if (*m_curPtr == '"' && m_curPtr[1] == '"')
	{
		const char* p = m_curPtr + 2;

		while ((p[0] != '"') || (p[1] != '"'))
		{
			DE_ASSERT(*p);
			if (*p == '\\')
			{
				DE_ASSERT(p[1] != 0);
				p += 2;
			}
			else
				p++;
		}
		p += 2;

		m_curToken	= TOKEN_SHADER_SOURCE;
		m_curTokenStr = string(m_curPtr, (int)(p - m_curPtr));
	}
	else if (*m_curPtr == '"' || *m_curPtr == '\'')
	{
		char		endChar = *m_curPtr;
		const char* p		= m_curPtr + 1;

		while (*p != endChar)
		{
			DE_ASSERT(*p);
			if (*p == '\\')
			{
				DE_ASSERT(p[1] != 0);
				p += 2;
			}
			else
				p++;
		}
		p++;

		m_curToken	= TOKEN_STRING;
		m_curTokenStr = string(m_curPtr, (int)(p - m_curPtr));
	}
	else
	{
		struct SimpleToken
		{
			const char* str;
			Token		token;
		};

		static const SimpleToken s_simple[] = { { "=", TOKEN_ASSIGN },		 { "+", TOKEN_PLUS },
												{ "-", TOKEN_MINUS },		 { ",", TOKEN_COMMA },
												{ "|", TOKEN_VERTICAL_BAR }, { ";", TOKEN_SEMI_COLON },
												{ "(", TOKEN_LEFT_PAREN },   { ")", TOKEN_RIGHT_PAREN },
												{ "[", TOKEN_LEFT_BRACKET }, { "]", TOKEN_RIGHT_BRACKET },
												{ "{", TOKEN_LEFT_BRACE },   { "}", TOKEN_RIGHT_BRACE } };

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_simple); ndx++)
		{
			if (strncmp(s_simple[ndx].str, m_curPtr, strlen(s_simple[ndx].str)) == 0)
			{
				m_curToken	= s_simple[ndx].token;
				m_curTokenStr = s_simple[ndx].str;
				return;
			}
		}

		// Otherwise invalid token.
		m_curToken	= TOKEN_INVALID;
		m_curTokenStr = *m_curPtr;
	}
}

void ShaderParser::advanceToken(Token assumed)
{
	assumeToken(assumed);
	advanceToken();
}

void ShaderParser::assumeToken(Token token)
{
	if (m_curToken != token)
		parseError(
			(string("unexpected token '") + m_curTokenStr + "', expecting '" + getTokenName(token) + "'").c_str());
	DE_TEST_ASSERT(m_curToken == token);
}

DataType ShaderParser::mapDataTypeToken(Token token)
{
	switch (token)
	{
	case TOKEN_FLOAT:
		return TYPE_FLOAT;
	case TOKEN_FLOAT_VEC2:
		return TYPE_FLOAT_VEC2;
	case TOKEN_FLOAT_VEC3:
		return TYPE_FLOAT_VEC3;
	case TOKEN_FLOAT_VEC4:
		return TYPE_FLOAT_VEC4;
	case TOKEN_FLOAT_MAT2:
		return TYPE_FLOAT_MAT2;
	case TOKEN_FLOAT_MAT2X3:
		return TYPE_FLOAT_MAT2X3;
	case TOKEN_FLOAT_MAT2X4:
		return TYPE_FLOAT_MAT2X4;
	case TOKEN_FLOAT_MAT3X2:
		return TYPE_FLOAT_MAT3X2;
	case TOKEN_FLOAT_MAT3:
		return TYPE_FLOAT_MAT3;
	case TOKEN_FLOAT_MAT3X4:
		return TYPE_FLOAT_MAT3X4;
	case TOKEN_FLOAT_MAT4X2:
		return TYPE_FLOAT_MAT4X2;
	case TOKEN_FLOAT_MAT4X3:
		return TYPE_FLOAT_MAT4X3;
	case TOKEN_FLOAT_MAT4:
		return TYPE_FLOAT_MAT4;
	case TOKEN_INT:
		return TYPE_INT;
	case TOKEN_INT_VEC2:
		return TYPE_INT_VEC2;
	case TOKEN_INT_VEC3:
		return TYPE_INT_VEC3;
	case TOKEN_INT_VEC4:
		return TYPE_INT_VEC4;
	case TOKEN_UINT:
		return TYPE_UINT;
	case TOKEN_UINT_VEC2:
		return TYPE_UINT_VEC2;
	case TOKEN_UINT_VEC3:
		return TYPE_UINT_VEC3;
	case TOKEN_UINT_VEC4:
		return TYPE_UINT_VEC4;
	case TOKEN_BOOL:
		return TYPE_BOOL;
	case TOKEN_BOOL_VEC2:
		return TYPE_BOOL_VEC2;
	case TOKEN_BOOL_VEC3:
		return TYPE_BOOL_VEC3;
	case TOKEN_BOOL_VEC4:
		return TYPE_BOOL_VEC4;
	default:
		return TYPE_INVALID;
	}
}

const char* ShaderParser::getTokenName(Token token)
{
	switch (token)
	{
	case TOKEN_INVALID:
		return "<invalid>";
	case TOKEN_EOF:
		return "<eof>";
	case TOKEN_STRING:
		return "<string>";
	case TOKEN_SHADER_SOURCE:
		return "source";

	case TOKEN_INT_LITERAL:
		return "<int>";
	case TOKEN_FLOAT_LITERAL:
		return "<float>";

	// identifiers
	case TOKEN_IDENTIFIER:
		return "<identifier>";
	case TOKEN_TRUE:
		return "true";
	case TOKEN_FALSE:
		return "false";
	case TOKEN_DESC:
		return "desc";
	case TOKEN_EXPECT:
		return "expect";
	case TOKEN_GROUP:
		return "group";
	case TOKEN_CASE:
		return "case";
	case TOKEN_END:
		return "end";
	case TOKEN_VALUES:
		return "values";
	case TOKEN_BOTH:
		return "both";
	case TOKEN_VERTEX:
		return "vertex";
	case TOKEN_FRAGMENT:
		return "fragment";
	case TOKEN_UNIFORM:
		return "uniform";
	case TOKEN_INPUT:
		return "input";
	case TOKEN_OUTPUT:
		return "output";
	case TOKEN_FLOAT:
		return "float";
	case TOKEN_FLOAT_VEC2:
		return "vec2";
	case TOKEN_FLOAT_VEC3:
		return "vec3";
	case TOKEN_FLOAT_VEC4:
		return "vec4";
	case TOKEN_FLOAT_MAT2:
		return "mat2";
	case TOKEN_FLOAT_MAT2X3:
		return "mat2x3";
	case TOKEN_FLOAT_MAT2X4:
		return "mat2x4";
	case TOKEN_FLOAT_MAT3X2:
		return "mat3x2";
	case TOKEN_FLOAT_MAT3:
		return "mat3";
	case TOKEN_FLOAT_MAT3X4:
		return "mat3x4";
	case TOKEN_FLOAT_MAT4X2:
		return "mat4x2";
	case TOKEN_FLOAT_MAT4X3:
		return "mat4x3";
	case TOKEN_FLOAT_MAT4:
		return "mat4";
	case TOKEN_INT:
		return "int";
	case TOKEN_INT_VEC2:
		return "ivec2";
	case TOKEN_INT_VEC3:
		return "ivec3";
	case TOKEN_INT_VEC4:
		return "ivec4";
	case TOKEN_UINT:
		return "uint";
	case TOKEN_UINT_VEC2:
		return "uvec2";
	case TOKEN_UINT_VEC3:
		return "uvec3";
	case TOKEN_UINT_VEC4:
		return "uvec4";
	case TOKEN_BOOL:
		return "bool";
	case TOKEN_BOOL_VEC2:
		return "bvec2";
	case TOKEN_BOOL_VEC3:
		return "bvec3";
	case TOKEN_BOOL_VEC4:
		return "bvec4";

	case TOKEN_ASSIGN:
		return "=";
	case TOKEN_PLUS:
		return "+";
	case TOKEN_MINUS:
		return "-";
	case TOKEN_COMMA:
		return ",";
	case TOKEN_VERTICAL_BAR:
		return "|";
	case TOKEN_SEMI_COLON:
		return ";";
	case TOKEN_LEFT_PAREN:
		return "(";
	case TOKEN_RIGHT_PAREN:
		return ")";
	case TOKEN_LEFT_BRACKET:
		return "[";
	case TOKEN_RIGHT_BRACKET:
		return "]";
	case TOKEN_LEFT_BRACE:
		return "{";
	case TOKEN_RIGHT_BRACE:
		return "}";

	default:
		return "<unknown>";
	}
}

void ShaderParser::parseValueElement(DataType expectedDataType, ShaderCase::Value& result)
{
	DataType scalarType = getDataTypeScalarType(expectedDataType);
	int		 scalarSize = getDataTypeScalarSize(expectedDataType);

	/* \todo [2010-04-19 petri] Support arrays. */
	ShaderCase::Value::Element elems[16];

	if (scalarSize > 1)
	{
		DE_ASSERT(mapDataTypeToken(m_curToken) == expectedDataType);
		advanceToken(); // data type (float, vec2, etc.)
		advanceToken(TOKEN_LEFT_PAREN);
	}

	for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
	{
		if (scalarType == TYPE_FLOAT)
		{
			float signMult = 1.0f;
			if (m_curToken == TOKEN_MINUS)
			{
				signMult = -1.0f;
				advanceToken();
			}

			assumeToken(TOKEN_FLOAT_LITERAL);
			elems[scalarNdx].float32 = signMult * parseFloatLiteral(m_curTokenStr.c_str());
			advanceToken(TOKEN_FLOAT_LITERAL);
		}
		else if (scalarType == TYPE_INT || scalarType == TYPE_UINT)
		{
			int signMult = 1;
			if (m_curToken == TOKEN_MINUS)
			{
				signMult = -1;
				advanceToken();
			}

			assumeToken(TOKEN_INT_LITERAL);
			elems[scalarNdx].int32 = signMult * parseIntLiteral(m_curTokenStr.c_str());
			advanceToken(TOKEN_INT_LITERAL);
		}
		else
		{
			DE_ASSERT(scalarType == TYPE_BOOL);
			elems[scalarNdx].bool32 = (m_curToken == TOKEN_TRUE);
			if (m_curToken != TOKEN_TRUE && m_curToken != TOKEN_FALSE)
				parseError(string("unexpected token, expecting bool: " + m_curTokenStr));
			advanceToken(); // true/false
		}

		if (scalarNdx != (scalarSize - 1))
			advanceToken(TOKEN_COMMA);
	}

	if (scalarSize > 1)
		advanceToken(TOKEN_RIGHT_PAREN);

	// Store results.
	for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
		result.elements.push_back(elems[scalarNdx]);
}

void ShaderParser::parseValue(ShaderCase::ValueBlock& valueBlock)
{
	PARSE_DBG(("      parseValue()\n"));

	// Parsed results.
	ShaderCase::Value result;

	// Parse storage.
	if (m_curToken == TOKEN_UNIFORM)
		result.storageType = ShaderCase::Value::STORAGE_UNIFORM;
	else if (m_curToken == TOKEN_INPUT)
		result.storageType = ShaderCase::Value::STORAGE_INPUT;
	else if (m_curToken == TOKEN_OUTPUT)
		result.storageType = ShaderCase::Value::STORAGE_OUTPUT;
	else
		parseError(string("unexpected token encountered when parsing value classifier"));
	advanceToken();

	// Parse data type.
	result.dataType = mapDataTypeToken(m_curToken);
	if (result.dataType == TYPE_INVALID)
		parseError(string("unexpected token when parsing value data type: " + m_curTokenStr));
	advanceToken();

	// Parse value name.
	if (m_curToken == TOKEN_IDENTIFIER || m_curToken == TOKEN_STRING)
	{
		if (m_curToken == TOKEN_IDENTIFIER)
			result.valueName = m_curTokenStr;
		else
			result.valueName = parseStringLiteral(m_curTokenStr.c_str());
	}
	else
		parseError(string("unexpected token when parsing value name: " + m_curTokenStr));
	advanceToken();

	// Parse assignment operator.
	advanceToken(TOKEN_ASSIGN);

	// Parse actual value.
	if (m_curToken == TOKEN_LEFT_BRACKET) // value list
	{
		advanceToken(TOKEN_LEFT_BRACKET);
		result.arrayLength = 0;

		for (;;)
		{
			parseValueElement(result.dataType, result);
			result.arrayLength++;

			if (m_curToken == TOKEN_RIGHT_BRACKET)
				break;
			else if (m_curToken == TOKEN_VERTICAL_BAR)
			{
				advanceToken();
				continue;
			}
			else
				parseError(string("unexpected token in value element array: " + m_curTokenStr));
		}

		advanceToken(TOKEN_RIGHT_BRACKET);
	}
	else // arrays, single elements
	{
		parseValueElement(result.dataType, result);
		result.arrayLength = 1;
	}

	advanceToken(TOKEN_SEMI_COLON); // end of declaration

	valueBlock.values.push_back(result);
}

void ShaderParser::parseValueBlock(ShaderCase::ValueBlock& valueBlock)
{
	PARSE_DBG(("    parseValueBlock()\n"));
	advanceToken(TOKEN_VALUES);
	advanceToken(TOKEN_LEFT_BRACE);

	for (;;)
	{
		if (m_curToken == TOKEN_UNIFORM || m_curToken == TOKEN_INPUT || m_curToken == TOKEN_OUTPUT)
			parseValue(valueBlock);
		else if (m_curToken == TOKEN_RIGHT_BRACE)
			break;
		else
			parseError(string("unexpected token when parsing a value block: " + m_curTokenStr));
	}

	advanceToken(TOKEN_RIGHT_BRACE);

	// Compute combined array length of value block.
	int arrayLength = 1;
	for (int valueNdx = 0; valueNdx < (int)valueBlock.values.size(); valueNdx++)
	{
		const ShaderCase::Value& val = valueBlock.values[valueNdx];
		if (val.arrayLength > 1)
		{
			DE_ASSERT(arrayLength == 1 || arrayLength == val.arrayLength);
			arrayLength = val.arrayLength;
		}
	}
	valueBlock.arrayLength = arrayLength;
}

void ShaderParser::parseShaderCase(vector<tcu::TestNode*>& shaderNodeList)
{
	// Parse 'case'.
	PARSE_DBG(("  parseShaderCase()\n"));
	advanceToken(TOKEN_CASE);

	// Parse case name.
	string caseName = m_curTokenStr;
	advanceToken(); // \note [pyry] All token types are allowed here.

	// Setup case.
	vector<ShaderCase::ValueBlock> valueBlockList;

	GLSLVersion				 version	  = DEFAULT_GLSL_VERSION;
	ShaderCase::ExpectResult expectResult = ShaderCase::EXPECT_PASS;
	string					 description;
	string					 bothSource;
	string					 vertexSource;
	string					 fragmentSource;

	for (;;)
	{
		if (m_curToken == TOKEN_END)
			break;
		else if (m_curToken == TOKEN_DESC)
		{
			advanceToken();
			assumeToken(TOKEN_STRING);

			description = parseStringLiteral(m_curTokenStr.c_str());
			advanceToken();
		}
		else if (m_curToken == TOKEN_EXPECT)
		{
			advanceToken();
			assumeToken(TOKEN_IDENTIFIER);

			if (m_curTokenStr == "pass")
				expectResult = ShaderCase::EXPECT_PASS;
			else if (m_curTokenStr == "compile_fail")
				expectResult = ShaderCase::EXPECT_COMPILE_FAIL;
			else if (m_curTokenStr == "link_fail")
				expectResult = ShaderCase::EXPECT_LINK_FAIL;
			else
				parseError(string("invalid expected result value: " + m_curTokenStr));

			advanceToken();
		}
		else if (m_curToken == TOKEN_VALUES)
		{
			ShaderCase::ValueBlock block;
			parseValueBlock(block);
			valueBlockList.push_back(block);
		}
		else if (m_curToken == TOKEN_BOTH || m_curToken == TOKEN_VERTEX || m_curToken == TOKEN_FRAGMENT)
		{
			Token token = m_curToken;
			advanceToken();
			assumeToken(TOKEN_SHADER_SOURCE);
			string source = parseShaderSource(m_curTokenStr.c_str());
			advanceToken();
			if (token == TOKEN_BOTH)
				bothSource = source;
			else if (token == TOKEN_VERTEX)
				vertexSource = source;
			else if (token == TOKEN_FRAGMENT)
				fragmentSource = source;
			else
				DE_ASSERT(DE_FALSE);
		}
		else if (m_curToken == TOKEN_VERSION)
		{
			advanceToken();

			int			versionNum = 0;
			std::string postfix	= "";

			assumeToken(TOKEN_INT_LITERAL);
			versionNum = parseIntLiteral(m_curTokenStr.c_str());
			advanceToken();

			if (m_curToken == TOKEN_IDENTIFIER)
			{
				postfix = m_curTokenStr;
				advanceToken();
			}

			if (versionNum == 100 && postfix == "es")
				version = glu::GLSL_VERSION_100_ES;
			else if (versionNum == 300 && postfix == "es")
				version = glu::GLSL_VERSION_300_ES;
			else if (versionNum == 310 && postfix == "es")
				version = glu::GLSL_VERSION_310_ES;
			else if (versionNum == 130)
				version = glu::GLSL_VERSION_130;
			else if (versionNum == 140)
				version = glu::GLSL_VERSION_140;
			else if (versionNum == 150)
				version = glu::GLSL_VERSION_150;
			else if (versionNum == 330)
				version = glu::GLSL_VERSION_330;
			else if (versionNum == 400)
				version = glu::GLSL_VERSION_400;
			else if (versionNum == 410)
				version = glu::GLSL_VERSION_410;
			else if (versionNum == 420)
				version = glu::GLSL_VERSION_420;
			else if (versionNum == 430)
				version = glu::GLSL_VERSION_430;
			else if (versionNum == 440)
				version = glu::GLSL_VERSION_440;
			else if (versionNum == 450)
				version = glu::GLSL_VERSION_450;
			else
				parseError("Unknown GLSL version");
		}
		else
			parseError(string("unexpected token while parsing shader case: " + m_curTokenStr));
	}

	advanceToken(TOKEN_END); // case end

	if (bothSource.length() > 0)
	{
		DE_ASSERT(vertexSource.length() == 0);
		DE_ASSERT(fragmentSource.length() == 0);

		string vertName = caseName + "_vertex";
		string fragName = caseName + "_fragment";
		shaderNodeList.push_back(new ShaderCase(m_testCtx, m_renderCtx, vertName.c_str(), description.c_str(),
												expectResult, valueBlockList, version, bothSource.c_str(), DE_NULL));
		shaderNodeList.push_back(new ShaderCase(m_testCtx, m_renderCtx, fragName.c_str(), description.c_str(),
												expectResult, valueBlockList, version, DE_NULL, bothSource.c_str()));
	}
	else
	{
		shaderNodeList.push_back(new ShaderCase(m_testCtx, m_renderCtx, caseName.c_str(), description.c_str(),
												expectResult, valueBlockList, version, vertexSource.c_str(),
												fragmentSource.c_str()));
	}
}

void ShaderParser::parseShaderGroup(vector<tcu::TestNode*>& shaderNodeList)
{
	// Parse 'case'.
	PARSE_DBG(("  parseShaderGroup()\n"));
	advanceToken(TOKEN_GROUP);

	// Parse case name.
	string name = m_curTokenStr;
	advanceToken(); // \note [pyry] We don't want to check token type here (for instance to allow "uniform") group.

	// Parse description.
	assumeToken(TOKEN_STRING);
	string description = parseStringLiteral(m_curTokenStr.c_str());
	advanceToken(TOKEN_STRING);

	std::vector<tcu::TestNode*> children;

	// Parse group children.
	for (;;)
	{
		if (m_curToken == TOKEN_END)
			break;
		else if (m_curToken == TOKEN_GROUP)
			parseShaderGroup(children);
		else if (m_curToken == TOKEN_CASE)
			parseShaderCase(children);
		else
			parseError(string("unexpected token while parsing shader group: " + m_curTokenStr));
	}

	advanceToken(TOKEN_END); // group end

	// Create group node.
	tcu::TestCaseGroup* groupNode = new tcu::TestCaseGroup(m_testCtx, name.c_str(), description.c_str(), children);
	shaderNodeList.push_back(groupNode);
}

vector<tcu::TestNode*> ShaderParser::parse(const char* input)
{
	// Initialize parser.
	m_input		  = input;
	m_curPtr	  = m_input.c_str();
	m_curToken	= TOKEN_INVALID;
	m_curTokenStr = "";
	advanceToken();

	vector<tcu::TestNode*> nodeList;

	// Parse all cases.
	PARSE_DBG(("parse()\n"));
	for (;;)
	{
		if (m_curToken == TOKEN_CASE)
			parseShaderCase(nodeList);
		else if (m_curToken == TOKEN_GROUP)
			parseShaderGroup(nodeList);
		else if (m_curToken == TOKEN_EOF)
			break;
		else
			parseError(string("invalid token encountered at main level: '") + m_curTokenStr + "'");
	}

	assumeToken(TOKEN_EOF);
	//  printf("  parsed %d test cases.\n", caseList.size());
	return nodeList;
}

} // sl

ShaderLibrary::ShaderLibrary(tcu::TestContext& testCtx, RenderContext& renderCtx)
	: m_testCtx(testCtx), m_renderCtx(renderCtx)
{
}

ShaderLibrary::~ShaderLibrary(void)
{
}

vector<tcu::TestNode*> ShaderLibrary::loadShaderFile(const char* fileName)
{
	tcu::Resource*	resource = m_testCtx.getArchive().getResource(fileName);
	std::vector<char> buf;

	/*  printf("  loading '%s'\n", fileName);*/

	try
	{
		int size = resource->getSize();
		buf.resize(size + 1);
		resource->read((deUint8*)&buf[0], size);
		buf[size] = '\0';
	}
	catch (const std::exception&)
	{
		delete resource;
		throw;
	}

	delete resource;

	sl::ShaderParser	   parser(m_testCtx, m_renderCtx);
	vector<tcu::TestNode*> nodes = parser.parse(&buf[0]);

	return nodes;
}

// ShaderLibraryGroup

ShaderLibraryGroup::ShaderLibraryGroup(Context& context, const char* name, const char* description,
									   const char* filename)
	: TestCaseGroup(context, name, description), m_filename(filename)
{
}

ShaderLibraryGroup::~ShaderLibraryGroup(void)
{
}

void ShaderLibraryGroup::init(void)
{
	deqp::ShaderLibrary			shaderLibrary(m_testCtx, m_context.getRenderContext());
	std::vector<tcu::TestNode*> children = shaderLibrary.loadShaderFile(m_filename.c_str());

	for (int i = 0; i < (int)children.size(); i++)
		addChild(children[i]);
}

} // deqp
