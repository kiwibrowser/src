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

#include "glwEnums.hpp"

#include "gluContextInfo.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include <assert.h>
#include <map>

#include "es31cExplicitUniformLocationTest.hpp"

namespace glcts
{
using namespace glw;
namespace
{

class Logger
{
public:
	Logger() : null_log_(0)
	{
	}

	Logger(const Logger& rhs)
	{
		null_log_ = rhs.null_log_;
		if (!null_log_)
		{
			str_ << rhs.str_.str();
		}
	}

	~Logger()
	{
		s_tcuLog->writeMessage(str_.str().c_str());
		if (!str_.str().empty())
		{
			s_tcuLog->writeMessage(NL);
		}
	}

	template <class T>
	Logger& operator<<(const T& t)
	{
		if (!null_log_)
		{
			str_ << t;
		}
		return *this;
	}

	static tcu::TestLog* Get()
	{
		return s_tcuLog;
	}

	static void setOutput(tcu::TestLog& log)
	{
		s_tcuLog = &log;
	}

private:
	void				 operator=(const Logger&);
	bool				 null_log_;
	std::ostringstream   str_;
	static tcu::TestLog* s_tcuLog;
};
tcu::TestLog* Logger::s_tcuLog = NULL;

class DefOccurence
{
public:
	enum DefOccurenceEnum
	{
		ALL_SH,
		VSH,
		FSH_OR_CSH, //"one shader"
		ALL_BUT_FSH,
		ALL_BUT_VSH,
		NONE_SH,
	} occurence;

	DefOccurence(DefOccurenceEnum _occurence) : occurence(_occurence)
	{
	}

	bool occurs(GLenum shader) const
	{
		if (occurence == NONE_SH)
		{
			return false;
		}
		if (occurence == ALL_SH)
		{
			return true;
		}
		if (occurence == FSH_OR_CSH)
		{
			return shader == GL_FRAGMENT_SHADER || shader == GL_COMPUTE_SHADER;
		}
		if (occurence == VSH)
		{
			return shader == GL_VERTEX_SHADER;
		}
		if (occurence == ALL_BUT_FSH)
		{
			return shader != GL_FRAGMENT_SHADER;
		}
		if (occurence == ALL_BUT_VSH)
		{
			return shader != GL_VERTEX_SHADER;
		}
		assert(0);
		return false;
	}
};

class LocationSpecifier
{
};
class IndexSpecifier
{
};

class LayoutSpecifierBase
{
public:
	enum NumSys
	{
		Dec,
		Oct,
		Hex,
	};

	LayoutSpecifierBase(int _val, NumSys _numSys, DefOccurence _occurence)
		: val(_val), numSys(_numSys), occurence(_occurence)
	{
	}

	bool isImplicit(const std::vector<GLenum> stages) const
	{
		bool implicit = true;
		for (size_t i = 0; i < stages.size(); i++)
		{
			implicit &= !occurence.occurs(stages[i]);
		}

		return implicit;
	}

	int			 val;
	NumSys		 numSys;
	DefOccurence occurence;
};

template <class T>
class LayoutSpecifier : public LayoutSpecifierBase
{
public:
	LayoutSpecifier(int _val, NumSys _numSys, DefOccurence _occurence) : LayoutSpecifierBase(_val, _numSys, _occurence)
	{
	}

	static LayoutSpecifier<T> C(int _val, NumSys _sys = Dec)
	{
		return LayoutSpecifier<T>(_val, _sys, DefOccurence::ALL_SH);
	}

	static LayoutSpecifier<T> C(int _val, DefOccurence _occurence)
	{
		return LayoutSpecifier<T>(_val, Dec, _occurence);
	}

	static LayoutSpecifier<T> Implicit()
	{
		return LayoutSpecifier<T>(1999999, Dec, DefOccurence::NONE_SH);
	}

	void streamDefinition(std::ostringstream& str, GLenum shader) const;
};

typedef LayoutSpecifier<LocationSpecifier> Loc;
typedef LayoutSpecifier<IndexSpecifier>	Index;

template <>
void LayoutSpecifier<LocationSpecifier>::streamDefinition(std::ostringstream& str, GLenum shader) const
{
	if (val < 0 || !occurence.occurs(shader))
	{
		return;
	}

	str << "layout(location = ";
	if (numSys == Loc::Oct)
	{
		str << std::oct << "0";
	}
	else if (numSys == Loc::Hex)
	{
		str << std::hex << "0x";
	}
	str << val << std::dec << ") ";
}

template <>
void LayoutSpecifier<IndexSpecifier>::streamDefinition(std::ostringstream& str, GLenum shader) const
{
	if (val < 0 || !occurence.occurs(shader))
	{
		return;
	}

	str << "layout(index = ";
	if (numSys == Loc::Oct)
	{
		str << std::oct << "0";
	}
	else if (numSys == Loc::Hex)
	{
		str << std::hex << "0x";
	}
	str << val << std::dec << ") ";
}

class UniformStructCounter
{
public:
	UniformStructCounter() : counter(0)
	{
	}
	GLint getNextCount()
	{
		return counter++;
	}

private:
	UniformStructCounter(const UniformStructCounter&);
	GLint counter;
};

class UniformType
{
public:
	UniformType(GLenum _enumType, int _arraySize = 0)
		: enumType(_enumType), arraySize(_arraySize), isArray(_arraySize > 0), signedType(true)
	{
		if (!arraySize)
		{
			arraySize = 1;
		}
		arraySizesSegmented.push_back(arraySize);
		fill();
	}
	UniformType(GLenum _enumType, const std::vector<int>& _arraySizesSegmented)
		: enumType(_enumType), arraySizesSegmented(_arraySizesSegmented), isArray(true), signedType(true)
	{
		arraySize = 1;
		for (size_t i = 0; i < arraySizesSegmented.size(); i++)
		{
			assert(arraySizesSegmented[i] > 0);
			arraySize *= arraySizesSegmented[i];
		}
		fill();
	}
	UniformType(UniformStructCounter& structCounter, std::vector<UniformType> _childTypes, int _arraySize = 0)
		: enumType(0), arraySize(_arraySize), childTypes(_childTypes), isArray(_arraySize > 0), signedType(true)
	{
		baseType = 0;
		std::ostringstream _str;
		_str << "S" << structCounter.getNextCount();
		strType = _str.str();
		if (!arraySize)
		{
			arraySize = 1;
		}
		arraySizesSegmented.push_back(arraySize);
	}

	inline const std::string& str() const
	{
		return strType;
	}

	inline const std::string& refStr() const
	{
		return refStrType;
	}

	bool isStruct() const
	{
		return (baseType == 0);
	}

	bool isSigned() const
	{
		return signedType;
	}

	const char* abs() const
	{
		switch (baseType)
		{
		case GL_FLOAT:
		case GL_SAMPLER:
			return "0.1";
		case GL_UNSIGNED_INT:
			return "0u";
		case GL_INT:
			return "0";
		default:
			assert(0);
			return "";
		}
	}

	std::pair<int, int> getSize() const
	{
		return size;
	}

	GLenum getBaseType() const
	{
		return baseType;
	}

	void streamArrayStr(std::ostringstream& _str, int arrayElem = -1) const
	{
		if (!isArray)
		{
			return;
		}
		if (arrayElem < 0)
		{
			for (size_t segment = 0; segment < arraySizesSegmented.size(); segment++)
			{
				_str << "[" << arraySizesSegmented[segment] << "]";
			}
		}
		else
		{
			int tailSize = arraySize;
			for (size_t segment = 0; segment < arraySizesSegmented.size(); segment++)
			{
				tailSize /= arraySizesSegmented[segment];
				_str << "[" << arrayElem / tailSize << "]";
				arrayElem %= tailSize;
			}
		}
	}

	GLenum enumType;

	//arrays-of-arrays size
	std::vector<int> arraySizesSegmented;

	//premultiplied array size
	int arraySize;

	//child types for nested (struct) types;
	std::vector<UniformType> childTypes;

private:
	void fill()
	{

		size = std::pair<int, int>(1, 1);

		switch (enumType)
		{
		case GL_SAMPLER_2D:
			refStrType = "vec4";
			strType	= "sampler2D";
			baseType   = GL_SAMPLER;
			break;
		case GL_FLOAT:
			refStrType = strType = "float";
			baseType			 = GL_FLOAT;
			break;
		case GL_INT:
			refStrType = strType = "int";
			baseType			 = GL_INT;
			break;
		case GL_UNSIGNED_INT:
			refStrType = strType = "uint";
			baseType			 = GL_UNSIGNED_INT;
			signedType			 = false;
			break;
		case GL_FLOAT_VEC2:
			refStrType = strType = "vec2";
			baseType			 = GL_FLOAT;
			size.first			 = 2;
			break;
		case GL_FLOAT_VEC3:
			refStrType = strType = "vec3";
			baseType			 = GL_FLOAT;
			size.first			 = 3;
			break;
		case GL_FLOAT_VEC4:
			refStrType = strType = "vec4";
			baseType			 = GL_FLOAT;
			size.first			 = 4;
			break;
		case GL_FLOAT_MAT2:
			strType	= "mat2";
			refStrType = "vec2";
			baseType   = GL_FLOAT;
			size.first = size.second = 2;
			break;
		case GL_FLOAT_MAT3:
			strType	= "mat3";
			refStrType = "vec3";
			baseType   = GL_FLOAT;
			size.first = size.second = 3;
			break;
		case GL_FLOAT_MAT4:
			strType	= "mat4";
			refStrType = "vec4";
			baseType   = GL_FLOAT;
			size.first = size.second = 4;
			break;
		case GL_FLOAT_MAT2x3:
			strType		= "mat2x3";
			refStrType  = "vec3";
			baseType	= GL_FLOAT;
			size.first  = 3;
			size.second = 2;
			break;
		case GL_FLOAT_MAT4x3:
			strType		= "mat4x3";
			refStrType  = "vec3";
			baseType	= GL_FLOAT;
			size.first  = 3;
			size.second = 4;
			break;
		case GL_FLOAT_MAT2x4:
			strType		= "mat2x4";
			refStrType  = "vec4";
			baseType	= GL_FLOAT;
			size.first  = 4;
			size.second = 2;
			break;
		case GL_FLOAT_MAT3x4:
			strType		= "mat3x4";
			refStrType  = "vec4";
			baseType	= GL_FLOAT;
			size.first  = 4;
			size.second = 3;
			break;
		case GL_FLOAT_MAT3x2:
			strType		= "mat3x2";
			refStrType  = "vec2";
			baseType	= GL_FLOAT;
			size.first  = 2;
			size.second = 3;
			break;
		case GL_FLOAT_MAT4x2:
			strType		= "mat4x2";
			refStrType  = "vec2";
			baseType	= GL_FLOAT;
			size.first  = 2;
			size.second = 4;
			break;
		case GL_INT_VEC2:
			refStrType = strType = "ivec2";
			baseType			 = GL_INT;
			size.first			 = 2;
			break;
		case GL_INT_VEC3:
			refStrType = strType = "ivec3";
			baseType			 = GL_INT;
			size.first			 = 3;
			break;
		case GL_INT_VEC4:
			refStrType = strType = "ivec4";
			baseType			 = GL_INT;
			size.first			 = 4;
			break;
		default:
			assert(0);
		}
	}

	std::string strType, refStrType;
	std::pair<int, int> size;
	GLenum baseType;
	bool   isArray;
	bool   signedType;
};

class UniformValueGenerator
{
public:
	UniformValueGenerator() : fValue(0.0f), iValue(0)
	{
	}
	GLfloat genF()
	{
		if (fValue > 99999.0f)
		{
			fValue = 0.0f;
		}
		return (fValue += 1.0f);
	}
	GLint genI()
	{
		return (iValue += 1);
	}

private:
	UniformValueGenerator(const UniformValueGenerator&);
	GLfloat fValue;
	GLint   iValue;
};

class UniformValue
{
public:
	void streamValue(std::ostringstream& str, int arrayElement = 0, int column = 0) const
	{
		int arrayElementSize = type.getSize().first * type.getSize().second;

		str << type.refStr() << "(";

		if (type.getBaseType() == GL_SAMPLER)
		{
			for (size_t elem = 0; elem < 4; elem++)
			{
				if (elem)
					str << ", ";
				str << fValues[arrayElement * 4 + elem];
			}
			str << ")";
			return;
		}

		for (int elem = 0; fValues.size() && elem < type.getSize().first; elem++)
		{
			if (elem)
				str << ", ";
			str << fValues[arrayElement * arrayElementSize + column * type.getSize().first + elem] << ".0";
		}
		for (int elem = 0; iValues.size() && elem < type.getSize().first; elem++)
		{
			if (elem)
				str << ", ";
			str << iValues[arrayElement * arrayElementSize + elem];
		}
		for (int elem = 0; uValues.size() && elem < type.getSize().first; elem++)
		{
			if (elem)
				str << ", ";
			str << uValues[arrayElement * arrayElementSize + elem] << "u";
		}
		str << ")";
	}

	const void* getPtr(int arrayElement) const
	{
		int arrayElementSize = type.getSize().first * type.getSize().second;
		if (type.getBaseType() == GL_INT || type.getBaseType() == GL_SAMPLER)
		{
			return &iValues[arrayElement * arrayElementSize];
		}
		else if (type.getBaseType() == GL_UNSIGNED_INT)
		{
			return &uValues[arrayElement * arrayElementSize];
		}
		else if (type.getBaseType() == GL_FLOAT)
		{
			return &fValues[arrayElement * arrayElementSize];
		}
		assert(0);
		return NULL;
	}

	UniformValue(const UniformType& _type, UniformValueGenerator& generator) : type(_type)
	{
		const int sizeRow	= type.getSize().first;
		const int sizeColumn = type.getSize().second;

		if (type.isStruct())
		{
			return;
		}

		if (type.getBaseType() == GL_INT)
		{
			assert(sizeColumn == 1);
			iValues.resize(sizeRow * type.arraySize);
			for (size_t elem = 0; elem < iValues.size(); elem++)
			{
				iValues[elem] = generator.genI();
			}
		}
		else if (type.getBaseType() == GL_UNSIGNED_INT)
		{
			assert(sizeColumn == 1);
			uValues.resize(sizeRow * type.arraySize);
			for (size_t elem = 0; elem < uValues.size(); elem++)
			{
				uValues[elem] = static_cast<GLuint>(generator.genI());
			}
		}
		else if (type.getBaseType() == GL_FLOAT)
		{
			fValues.resize(sizeColumn * sizeRow * type.arraySize);
			for (size_t elem = 0; elem < fValues.size(); elem++)
			{
				fValues[elem] = generator.genF();
			}
		}
		else if (type.getBaseType() == GL_SAMPLER)
		{
			//color ref value
			fValues.resize(4 * type.arraySize);
			for (size_t elem = 0; elem < fValues.size(); elem++)
			{
				fValues[elem] = float(elem) / float(fValues.size());
			}
			//uniform value
			iValues.resize(type.arraySize);
			for (size_t elem = 0; elem < iValues.size(); elem++)
			{
				iValues[elem] = generator.genI() % 16;
			}
		}
		else
		{
			assert(0);
		}
	}

	std::vector<GLfloat> fValues;
	std::vector<GLint>   iValues;
	std::vector<GLint>   uValues;

private:
	UniformType type;
};

class Uniform
{
public:
	Uniform(UniformValueGenerator& generator, UniformType _type, Loc _location,
			DefOccurence _declOccurence = DefOccurence::ALL_SH, DefOccurence _usageOccurence = DefOccurence::ALL_SH)
		: type(_type)
		, location(_location)
		, declOccurence(_declOccurence)
		, usageOccurence(_usageOccurence)
		, value(_type, generator)
	{

		if (type.isStruct())
		{
			int currentLocation = location.val;
			for (int arrayElem = 0; arrayElem < type.arraySize; arrayElem++)
			{
				for (size_t child = 0; child < type.childTypes.size(); child++)
				{
					Loc childLocation = Loc::Implicit();
					if (currentLocation > 0)
					{
						childLocation = Loc::C(currentLocation);
					}
					childUniforms.push_back(
						Uniform(generator, type.childTypes[child], childLocation, declOccurence, usageOccurence));
					currentLocation += type.childTypes[child].arraySize;
				}
			}
		}
	}

	void setName(const std::string& parentName, const std::string& _name)
	{
		shortName = _name;
		{
			std::ostringstream __name;
			__name << parentName << _name;
			name = __name.str();
		}
		if (type.isStruct())
		{
			for (size_t i = 0; i < childUniforms.size(); i++)
			{
				std::ostringstream childName;
				childName << "m" << (i % (childUniforms.size() / type.arraySize));
				std::ostringstream childParentName;
				childParentName << name;
				type.streamArrayStr(childParentName, (int)(i / type.arraySize));
				childParentName << ".";
				childUniforms[i].setName(childParentName.str(), childName.str());
			}
		}
	}
	const std::string& getName() const
	{
		return name;
	}

	void streamDefinition(std::ostringstream& str) const
	{
		str << type.str() << " " << shortName;
		type.streamArrayStr(str);
	}

	UniformType  type;
	Loc			 location;
	DefOccurence declOccurence, usageOccurence;
	UniformValue value;

	std::vector<Uniform> childUniforms;
	std::string			 name, shortName;
};

class SubroutineFunction
{
public:
	SubroutineFunction(UniformValueGenerator& generator, Index _index = Index::Implicit())
		: index(_index), embeddedRetVal(GL_FLOAT_VEC4, generator)
	{
	}
	const UniformValue& getRetVal() const
	{
		return embeddedRetVal;
	}

	inline const std::string& getName() const
	{
		return name;
	}

	void setName(int _name)
	{
		std::ostringstream __name;
		__name << "sf" << _name;
		name = __name.str();
	}

	Index index;

private:
	UniformValue embeddedRetVal;
	std::string  name;
};

class SubroutineFunctionSet
{
public:
	SubroutineFunctionSet(UniformValueGenerator& generator, size_t count = 0) : fn(count, SubroutineFunction(generator))
	{
	}

	void push_back(const SubroutineFunction& _fn)
	{
		fn.push_back(_fn);
	}

	inline const std::string& getTypeName() const
	{
		return typeName;
	}

	void setTypeName(int _name)
	{
		std::ostringstream __name;
		__name << "st" << _name;
		typeName = __name.str();
	}

	std::vector<SubroutineFunction> fn;
	std::string						typeName;
};

class SubroutineUniform
{
public:
	SubroutineUniform(UniformValueGenerator& generator, SubroutineFunctionSet& _functions, Loc _location,
					  int _arraySize = 0, DefOccurence _defOccurence = DefOccurence::ALL_SH, bool _used = true)
		: functions(_functions)
		, location(_location)
		, arraySize(_arraySize)
		, defOccurence(_defOccurence)
		, used(_used)
		, embeddedUIntUniform(GL_UNSIGNED_INT, generator)
	{

		assert(arraySize >= 0);

		if (!arraySize)
		{
			arraySize = 1;
			isArray   = false;
		}
		else
		{
			isArray = true;
		}

		arraySizesSegmented.push_back(arraySize);

		embeddedUIntUniform = UniformValue(UniformType(GL_UNSIGNED_INT, arraySize), generator);
		for (int i = 0; i < arraySize; i++)
		{
			embeddedUIntUniform.uValues[i] = static_cast<GLint>(embeddedUIntUniform.uValues[i] % functions.fn.size());
		}
	}

	SubroutineUniform(UniformValueGenerator& generator, SubroutineFunctionSet& _functions, Loc _location,
					  std::vector<int> _arraySizesSegmented, DefOccurence _defOccurence = DefOccurence::ALL_SH,
					  bool _used = true)
		: functions(_functions)
		, location(_location)
		, defOccurence(_defOccurence)
		, used(_used)
		, arraySizesSegmented(_arraySizesSegmented)
		, isArray(true)
		, embeddedUIntUniform(GL_UNSIGNED_INT, generator)
	{

		arraySize = 1;
		for (size_t i = 0; i < arraySizesSegmented.size(); i++)
		{
			assert(arraySizesSegmented[i] > 0);
			arraySize *= arraySizesSegmented[i];
		}

		embeddedUIntUniform = UniformValue(UniformType(GL_UNSIGNED_INT, arraySize), generator);
		for (int i = 0; i < arraySize; i++)
		{
			embeddedUIntUniform.uValues[i] = static_cast<GLint>(embeddedUIntUniform.uValues[i] % functions.fn.size());
		}
	}
	void setName(const std::string& _name)
	{
		name = _name;
	}

	const std::string& getName() const
	{
		return name;
	}

	void streamArrayStr(std::ostringstream& str, int arrayElem = -1) const
	{
		if (!isArray)
		{
			return;
		}
		if (arrayElem < 0)
		{
			for (size_t segment = 0; segment < arraySizesSegmented.size(); segment++)
			{
				str << "[" << arraySizesSegmented[segment] << "]";
			}
		}
		else
		{
			int tailSize = arraySize;
			for (size_t segment = 0; segment < arraySizesSegmented.size(); segment++)
			{
				tailSize /= arraySizesSegmented[segment];
				str << "[" << arrayElem / tailSize << "]";
				arrayElem %= tailSize;
			}
		}
	}

	const SubroutineFunction& getSelectedFunction(int arrayElem) const
	{
		assert(arrayElem < arraySize);
		return functions.fn[embeddedUIntUniform.uValues[arrayElem]];
	}

	SubroutineFunctionSet functions;
	Loc					  location;
	int					  arraySize;
	DefOccurence		  defOccurence;
	bool				  used;

private:
	std::vector<int> arraySizesSegmented;
	bool			 isArray;
	UniformValue	 embeddedUIntUniform;

	std::string name;
};

class ShaderKey
{
public:
	ShaderKey()
	{
	}
	ShaderKey(GLenum _stage, const std::string& _input, const std::string& _output)
		: stage(_stage), input(_input), output(_output)
	{
	}
	GLenum		stage;
	std::string input, output;

	bool operator<(const ShaderKey& rhs) const
	{
		if (stage == rhs.stage)
		{
			if (input == rhs.input)
			{
				return (output < rhs.output);
			}
			return input < rhs.input;
		}
		return stage < rhs.stage;
	}
};

class CompiledProgram
{
public:
	GLuint				name;
	std::vector<GLenum> stages;
};

class ShaderSourceFactory
{

	static void streamUniformDefinitions(const std::vector<Uniform>& uniforms, GLenum shader, std::ostringstream& ret)
	{
		for (size_t i = 0; i < uniforms.size(); i++)
		{
			if (uniforms[i].declOccurence.occurs(shader))
			{
				if (uniforms[i].type.isStruct())
				{
					ret << "struct " << uniforms[i].type.str() << " {" << std::endl;
					for (size_t child = 0; child < uniforms[i].childUniforms.size() / uniforms[i].type.arraySize;
						 child++)
					{
						ret << "    ";
						uniforms[i].childUniforms[child].streamDefinition(ret);
						ret << ";" << std::endl;
					}
					ret << "};" << std::endl;
				}
				uniforms[i].location.streamDefinition(ret, shader);
				ret << "uniform ";
				uniforms[i].streamDefinition(ret);
				ret << ";" << std::endl;
			}
		}
	}

	static void streamSubroutineDefinitions(const std::vector<SubroutineUniform>& subroutineUniforms, GLenum shader,
											std::ostringstream& ret)
	{
		if (subroutineUniforms.size())
		{
			//add a "zero" uniform;
			ret << "uniform float zero;" << std::endl;
		}

		for (size_t i = 0; i < subroutineUniforms.size(); i++)
		{
			if (subroutineUniforms[i].defOccurence.occurs(shader))
			{

				//subroutine vec4 st0(float param);
				ret << "subroutine vec4 " << subroutineUniforms[i].functions.getTypeName() << "(float param);"
					<< std::endl;

				for (size_t fn = 0; fn < subroutineUniforms[i].functions.fn.size(); fn++)
				{
					//layout(index = X) subroutine(st0) vec4 sf0(float param) { .... };
					subroutineUniforms[i].functions.fn[fn].index.streamDefinition(ret, shader);
					ret << "subroutine(" << subroutineUniforms[i].functions.getTypeName() << ") vec4 "
						<< subroutineUniforms[i].functions.fn[fn].getName() << "(float param) { return zero + ";
					subroutineUniforms[i].functions.fn[fn].getRetVal().streamValue(ret);
					ret << "; }" << std::endl;
				}

				//layout(location = X) subroutine uniform stX uX[...];
				subroutineUniforms[i].location.streamDefinition(ret, shader);
				ret << "subroutine uniform " << subroutineUniforms[i].functions.getTypeName() << " "
					<< subroutineUniforms[i].getName();
				subroutineUniforms[i].streamArrayStr(ret);
				ret << ";" << std::endl;
			}
		}
	}

	static void streamUniformValidator(std::ostringstream& ret, const Uniform& uniform, GLenum shader,
									   const char* outTemporary)
	{
		if (uniform.declOccurence.occurs(shader) && uniform.usageOccurence.occurs(shader))
		{
			if (uniform.type.isStruct())
			{
				for (size_t child = 0; child < uniform.childUniforms.size(); child++)
				{
					streamUniformValidator(ret, uniform.childUniforms[child], shader, outTemporary);
				}
			}
			else
			{
				for (int arrayElement = 0; arrayElement < uniform.type.arraySize; arrayElement++)
				{
					for (int column = 0; column < uniform.type.getSize().second; column++)
					{
						std::string columnIndex;
						if (uniform.type.getSize().second > 1)
						{
							std::ostringstream str;
							str << "[" << column << "]";
							columnIndex = str.str();
						}
						std::string absoluteF;
						if (uniform.type.isSigned())
						{
							absoluteF = "abs";
						}

						if (uniform.type.getBaseType() == GL_SAMPLER)
						{
							ret << NL "    if (any(greaterThan(" << absoluteF << "(texture(" << uniform.getName();
							uniform.type.streamArrayStr(ret, arrayElement);
							ret << columnIndex << ", vec2(0.5)) - ";
							uniform.value.streamValue(ret, arrayElement, column);
							ret << " ), " << uniform.type.refStr() << "(" << uniform.type.abs() << ")))) {";
						}
						else if (uniform.type.getSize().first > 1)
						{
							ret << NL "    if (any(greaterThan(" << absoluteF << "(" << uniform.getName();
							uniform.type.streamArrayStr(ret, arrayElement);
							ret << columnIndex << " - ";
							uniform.value.streamValue(ret, arrayElement, column);
							ret << "), " << uniform.type.refStr() << "(" << uniform.type.abs() << ")))) {";
						}
						else
						{
							ret << NL "    if (" << absoluteF << "(" << uniform.getName();
							uniform.type.streamArrayStr(ret, arrayElement);
							ret << " - ";
							uniform.value.streamValue(ret, arrayElement);
							ret << ") >" << uniform.type.refStr() << "(" << uniform.type.abs() << ")) {";
						}
						ret << NL "       " << outTemporary << " = vec4 (1.0, 0.0, 0.0, 1.0);";
						ret << NL "    }";
					}
				}
			}
		}
	}

	static void streamUniformValidators(std::ostringstream& ret, const std::vector<Uniform>& uniforms, GLenum shader,
										const char* outTemporary)
	{
		for (size_t i = 0; i < uniforms.size(); i++)
		{
			streamUniformValidator(ret, uniforms[i], shader, outTemporary);
		}
	}

	static void streamSubroutineValidator(std::ostringstream& ret, const SubroutineUniform& subroutineUniform,
										  GLenum shader, const char* outTemporary)
	{
		if (subroutineUniform.defOccurence.occurs(shader) && subroutineUniform.used)
		{
			for (int arrayElem = 0; arrayElem < subroutineUniform.arraySize; arrayElem++)
			{
				ret << NL "    if (any(greaterThan(abs(" << subroutineUniform.getName();
				subroutineUniform.streamArrayStr(ret, arrayElem);
				ret << "(zero) - ";
				subroutineUniform.getSelectedFunction(arrayElem).getRetVal().streamValue(ret);
				ret << "), vec4(0.1)))) {";
				ret << NL "       " << outTemporary << " = vec4 (1.0, 0.0, 0.0, 1.0);";
				ret << NL "    }";
			}
		}
	}

	static void streamSubroutineValidators(std::ostringstream&					 ret,
										   const std::vector<SubroutineUniform>& subroutineUniforms, GLenum shader,
										   const char* outTemporary)
	{
		for (size_t i = 0; i < subroutineUniforms.size(); i++)
		{
			streamSubroutineValidator(ret, subroutineUniforms[i], shader, outTemporary);
		}
	}

	static void streamShaderHeader(std::ostringstream& str, const glu::ContextType type)
	{
		if (glu::isContextTypeES(type))
		{
			str << "#version 310 es" NL "precision highp float;" NL "precision highp int;";
		}
		else
		{
			str << "#version 430 core" NL;
		}
	}

	static std::string generateFragmentShader(const ShaderKey& key, const std::vector<Uniform>& uniforms,
											  const std::vector<SubroutineUniform>& subroutineUniforms,
											  const std::string& additionalDef, const glu::ContextType type)
	{

		std::ostringstream ret;
		streamShaderHeader(ret, type);
		ret << NL;
		streamUniformDefinitions(uniforms, GL_FRAGMENT_SHADER, ret);
		ret << NL;
		streamSubroutineDefinitions(subroutineUniforms, GL_FRAGMENT_SHADER, ret);
		ret << NL << additionalDef << NL "in vec4 " << key.input << ";" << NL "out vec4 out_FragColor;"
			<< NL "void main() {" << NL "    vec4 validationResult = " << key.input << ";" << NL;
		streamUniformValidators(ret, uniforms, GL_FRAGMENT_SHADER, "validationResult");
		ret << NL;
		streamSubroutineValidators(ret, subroutineUniforms, GL_FRAGMENT_SHADER, "validationResult");
		ret << NL "    out_FragColor =  validationResult;" << NL "}";

		return ret.str();
	}

	static std::string generateVertexShader(const ShaderKey& key, const std::vector<Uniform>& uniforms,
											const std::vector<SubroutineUniform>& subroutineUniforms,
											const std::string& additionalDef, const glu::ContextType type)
	{

		std::ostringstream ret;
		streamShaderHeader(ret, type);
		ret << NL;
		streamUniformDefinitions(uniforms, GL_VERTEX_SHADER, ret);
		ret << NL;
		streamSubroutineDefinitions(subroutineUniforms, GL_VERTEX_SHADER, ret);
		ret << NL << additionalDef << NL "in vec4 in_Position;" << NL "out vec4 " << key.output << ";"
			<< NL "void main() {" << NL "    vec4 validationResult = vec4(0.0, 1.0, 0.0, 1.0);" << NL;
		streamUniformValidators(ret, uniforms, GL_VERTEX_SHADER, "validationResult");
		ret << NL;
		streamSubroutineValidators(ret, subroutineUniforms, GL_VERTEX_SHADER, "validationResult");
		ret << NL "    " << key.output << " = validationResult;" << NL "    gl_Position = in_Position;" << NL "}";
		return ret.str();
	}

	static std::string generateComputeShader(const ShaderKey&, const std::vector<Uniform>& uniforms,
											 const std::vector<SubroutineUniform>& subroutineUniforms,
											 const std::string& additionalDef, const glu::ContextType type)
	{

		std::ostringstream ret;
		streamShaderHeader(ret, type);
		ret << NL "layout (local_size_x = 1, local_size_y = 1) in;"
			<< NL "layout (std430, binding = 1) buffer ResultBuffer {" << NL "    vec4 cs_ValidationResult;" << NL "};"
			<< NL;
		streamUniformDefinitions(uniforms, GL_COMPUTE_SHADER, ret);
		ret << NL;
		streamSubroutineDefinitions(subroutineUniforms, GL_COMPUTE_SHADER, ret);
		ret << NL << additionalDef << NL "void main() {" << NL "    vec4 validationResult = vec4(0.0, 1.0, 0.0, 1.0);"
			<< NL;
		streamUniformValidators(ret, uniforms, GL_COMPUTE_SHADER, "validationResult");
		ret << NL;
		streamSubroutineValidators(ret, subroutineUniforms, GL_COMPUTE_SHADER, "validationResult");
		ret << NL "    cs_ValidationResult =  validationResult;" << NL "}";
		return ret.str();
	}

public:
	static std::string generateShader(const ShaderKey& key, const std::vector<Uniform>& uniforms,
									  const std::vector<SubroutineUniform>& subroutineUniforms,
									  const std::string& additionalDef, const glu::ContextType type)
	{

		switch (key.stage)
		{
		case GL_VERTEX_SHADER:
			return generateVertexShader(key, uniforms, subroutineUniforms, additionalDef, type);
		case GL_FRAGMENT_SHADER:
			return generateFragmentShader(key, uniforms, subroutineUniforms, additionalDef, type);
			break;
		case GL_COMPUTE_SHADER:
			return generateComputeShader(key, uniforms, subroutineUniforms, additionalDef, type);
			break;
		default:
			assert(0);
			return "";
		}
	}
};

class ExplicitUniformLocationCaseBase : public glcts::SubcaseBase
{
	virtual std::string Title()
	{
		return "";
	}
	virtual std::string Purpose()
	{
		return "";
	}
	virtual std::string Method()
	{
		return "";
	}
	virtual std::string PassCriteria()
	{
		return "";
	}

	int getWindowWidth()
	{
		return m_context.getRenderContext().getRenderTarget().getWidth();
	}

	int getWindowHeight()
	{
		return m_context.getRenderContext().getRenderTarget().getHeight();
	}

	std::map<ShaderKey, GLuint> CreateShaders(const std::vector<std::vector<ShaderKey> >& programConfigs,
											  const std::vector<Uniform>&			uniforms,
											  const std::vector<SubroutineUniform>& subroutineUniforms,
											  const std::string&					additionalDef)
	{
		std::map<ShaderKey, GLuint> ret;

		//create shaders
		for (size_t config = 0; config < programConfigs.size(); config++)
		{
			for (size_t target = 0; target < programConfigs[config].size(); target++)
			{

				if (ret.find(programConfigs[config][target]) == ret.end())
				{
					GLuint shader = glCreateShader(programConfigs[config][target].stage);

					std::string source = ShaderSourceFactory::generateShader(programConfigs[config][target], uniforms,
																			 subroutineUniforms, additionalDef,
																			 m_context.getRenderContext().getType());
					const char* cSource[] = { source.c_str() };
					glShaderSource(shader, 1, cSource, NULL);
					ret[programConfigs[config][target]] = shader;
				}
			}
		}

		//compile shaders
		for (std::map<ShaderKey, GLuint>::iterator i = ret.begin(); i != ret.end(); i++)
		{
			glCompileShader(i->second);
		}

		return ret;
	}

	long CreatePrograms(std::vector<CompiledProgram>& programs, const std::vector<Uniform>& uniforms,
						const std::vector<SubroutineUniform>& subroutineUniforms, const std::string& additionalDef,
						bool negativeCompile, bool negativeLink)
	{

		long ret = NO_ERROR;

		std::vector<std::vector<ShaderKey> > programConfigs;
		{
			std::vector<ShaderKey> vsh_fsh(2);
			vsh_fsh[0] = ShaderKey(GL_VERTEX_SHADER, "", "vs_ValidationResult");
			vsh_fsh[1] = ShaderKey(GL_FRAGMENT_SHADER, "vs_ValidationResult", "");
			programConfigs.push_back(vsh_fsh);
		}
		{
			std::vector<ShaderKey> csh(1);
			csh[0] = ShaderKey(GL_COMPUTE_SHADER, "", "");
			programConfigs.push_back(csh);
		}

		std::map<ShaderKey, GLuint> shaders =
			CreateShaders(programConfigs, uniforms, subroutineUniforms, additionalDef);

		//query compilation results
		for (std::map<ShaderKey, GLuint>::iterator it = shaders.begin(); it != shaders.end(); it++)
		{
			GLint status;
			glGetShaderiv(it->second, GL_COMPILE_STATUS, &status);
			GLchar infoLog[1000], source[4000];
			glGetShaderSource(it->second, 4000, NULL, source);
			glGetShaderInfoLog(it->second, 1000, NULL, infoLog);
			Logger::Get()->writeKernelSource(source);
			Logger::Get()->writeCompileInfo("shader", "", status == GL_TRUE, infoLog);

			if (!negativeLink)
			{
				if (!negativeCompile)
				{
					if (status != GL_TRUE)
					{
						Logger() << "Shader compilation failed";
						ret |= ERROR;
					}
				}
				else
				{
					if (status)
					{
						Logger() << "Negative compilation case failed: shader shoult not compile, but "
									"GL_COMPILE_STATUS != 0";
						ret |= ERROR;
					}
				}
			}
		}

		if (negativeCompile)
		{

			//delete shaders
			for (std::map<ShaderKey, GLuint>::iterator it = shaders.begin(); it != shaders.end(); it++)
			{
				glDeleteShader(it->second);
			}

			return ret;
		}

		//assemble programs and link
		for (size_t config = 0; config < programConfigs.size(); config++)
		{
			CompiledProgram program;
			program.name = glCreateProgram();

			for (size_t target = 0; target < programConfigs[config].size(); target++)
			{

				GLuint shader = shaders.find(programConfigs[config][target])->second;

				glAttachShader(program.name, shader);

				program.stages.push_back(programConfigs[config][target].stage);
			}
			programs.push_back(program);
			glLinkProgram(programs[config].name);
		}
		for (size_t config = 0; config < programConfigs.size(); config++)
		{
			glLinkProgram(programs[config].name);
		}

		//delete shaders
		for (std::map<ShaderKey, GLuint>::iterator it = shaders.begin(); it != shaders.end(); it++)
		{
			glDeleteShader(it->second);
		}

		//query link status:
		for (size_t config = 0; config < programConfigs.size(); config++)
		{
			GLint status;

			glGetProgramiv(programs[config].name, GL_LINK_STATUS, &status);
			GLchar infoLog[1000];
			glGetProgramInfoLog(programs[config].name, 1000, NULL, infoLog);
			Logger::Get()->writeCompileInfo("program", "", status == GL_TRUE, infoLog);

			if (!negativeLink)
			{
				if (status != GL_TRUE)
				{
					Logger() << "Shader link failed";
					ret |= ERROR;
				}
			}
			else
			{
				if (status)
				{
					Logger() << "Negative link case failed: program should not link, but GL_LINK_STATUS != 0";
					ret |= ERROR;
				}
			}
		}
		return ret;
	}

	long DeletePrograms(std::vector<CompiledProgram>& programs)
	{
		for (size_t i = 0; i < programs.size(); i++)
		{
			glDeleteProgram(programs[i].name);
		}
		programs.resize(0);
		return NO_ERROR;
	}

	void setUniform(const Uniform& uniform, const CompiledProgram& program)
	{

		bool used = false;
		for (size_t i = 0; i < program.stages.size(); i++)
		{
			used |= uniform.declOccurence.occurs(program.stages[i]) && uniform.usageOccurence.occurs(program.stages[i]);
		}
		if (!used)
			return;

		if (uniform.type.isStruct())
		{
			for (size_t j = 0; j < uniform.childUniforms.size(); j++)
			{
				setUniform(uniform.childUniforms[j], program);
			}
		}
		else
		{
			GLint loc;
			if (uniform.location.isImplicit(program.stages))
			{
				std::ostringstream name;
				name << uniform.getName();
				uniform.type.streamArrayStr(name, 0);
				loc = glGetUniformLocation(program.name, name.str().c_str());
			}
			else
			{
				loc = uniform.location.val;
			}

			for (int arrayElem = 0; arrayElem < uniform.type.arraySize; arrayElem++)
			{
				switch (uniform.type.enumType)
				{
				case GL_FLOAT:
					glUniform1f(loc, *(GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_VEC2:
					glUniform2fv(loc, 1, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_VEC3:
					glUniform3fv(loc, 1, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_VEC4:
					glUniform4fv(loc, 1, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT2:
					glUniformMatrix2fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT3:
					glUniformMatrix3fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT4:
					glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT2x3:
					glUniformMatrix2x3fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT4x3:
					glUniformMatrix4x3fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT2x4:
					glUniformMatrix2x4fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT3x4:
					glUniformMatrix3x4fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT3x2:
					glUniformMatrix3x2fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_FLOAT_MAT4x2:
					glUniformMatrix4x2fv(loc, 1, GL_FALSE, (GLfloat*)uniform.value.getPtr(arrayElem));
					break;
				case GL_INT:
				case GL_SAMPLER_2D:
					glUniform1i(loc, *(GLint*)uniform.value.getPtr(arrayElem));
					break;
				case GL_INT_VEC2:
					glUniform2iv(loc, 1, (GLint*)uniform.value.getPtr(arrayElem));
					break;
				case GL_INT_VEC3:
					glUniform3iv(loc, 1, (GLint*)uniform.value.getPtr(arrayElem));
					break;
				case GL_INT_VEC4:
					glUniform4iv(loc, 1, (GLint*)uniform.value.getPtr(arrayElem));
					break;
				case GL_UNSIGNED_INT:
					glUniform1ui(loc, *(GLuint*)uniform.value.getPtr(arrayElem));
					break;
				default:
					assert(0);
				}
				loc++;
			}
		}
	}

	void setSubroutineUniform(const SubroutineUniform& subroutineUniform, const CompiledProgram& program, GLenum stage,
							  std::vector<glw::GLuint>& indicesOut)
	{
		bool used = subroutineUniform.defOccurence.occurs(stage) && subroutineUniform.used;
		if (used)
		{

			for (int arrayElem = 0; arrayElem < subroutineUniform.arraySize; arrayElem++)
			{
				GLint loc = -1;
				if (subroutineUniform.location.isImplicit(program.stages))
				{
					std::ostringstream name;
					name << subroutineUniform.getName();
					subroutineUniform.streamArrayStr(name, arrayElem);
					loc = glGetSubroutineUniformLocation(program.name, stage, name.str().c_str());
				}
				else
				{
					loc = subroutineUniform.location.val + arrayElem;
				}

				if (loc >= 0)
				{
					const SubroutineFunction& selectedFunction = subroutineUniform.getSelectedFunction(arrayElem);

					int index = -1;
					if (selectedFunction.index.isImplicit(std::vector<GLenum>(1, stage)))
					{
						index = glGetSubroutineIndex(program.name, stage, selectedFunction.getName().c_str());
					}
					else
					{
						index = selectedFunction.index.val;
					}

					if (loc < (int)indicesOut.size())
					{
						indicesOut[loc] = index;
					}
					else
					{
						assert(0);
					}
				}
				else
				{
					assert(0);
				}
			}
		}
	}

	long runExecuteProgram(const CompiledProgram& program, const std::vector<Uniform>& uniforms,
						   const std::vector<SubroutineUniform>& subroutineUniforms)
	{
		long ret = NO_ERROR;

		glUseProgram(program.name);

		for (size_t i = 0; i < uniforms.size(); i++)
		{
			setUniform(uniforms[i], program);
		}

		for (size_t stage = 0; stage < program.stages.size() && subroutineUniforms.size(); stage++)
		{

			glw::GLint numactive;
			glGetProgramStageiv(program.name, program.stages[stage], GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS,
								&numactive);
			if (numactive)
			{
				std::vector<glw::GLuint> indices(numactive, 0);

				for (size_t i = 0; i < subroutineUniforms.size(); i++)
				{
					setSubroutineUniform(subroutineUniforms[i], program, program.stages[stage], indices);
				}
				glUniformSubroutinesuiv(program.stages[stage], numactive, &indices[0]);
			}
		}

		if (program.stages[0] != GL_COMPUTE_SHADER)
		{
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			std::vector<GLubyte> pixels(getWindowWidth() * getWindowHeight() * 4);

			glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
			for (size_t i = 0; i < pixels.size(); i += 4)
			{
				if (pixels[i] != 0 || pixels[i + 1] != 255 || pixels[i + 2] != 0)
				{
					ret |= ERROR;
					Logger() << "Program " << program.name << ": Wrong color. Expected green, got (" << (int)pixels[i]
							 << ", " << (int)pixels[i + 1] << ", " << (int)pixels[i + 2] << ", " << (int)pixels[i + 3]
							 << ").";
					break;
				}
			}
			Logger().Get()->writeImage("rendered image", "", QP_IMAGE_COMPRESSION_MODE_BEST, QP_IMAGE_FORMAT_RGBA8888,
									   getWindowWidth(), getWindowHeight(), 0, &pixels[0]);
		}
		else
		{
			GLuint buffer;
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_READ);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffer);

			glDispatchCompute(1, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			GLfloat* color = reinterpret_cast<GLfloat*>(
				glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(GLfloat), GL_MAP_READ_BIT));

			if (color[0] != 0 || color[1] != 1.0 || color[2] != 0)
			{
				ret |= ERROR;
				Logger() << "Program " << program.name << ": Wrong color. Expected green, got (" << (int)color[0]
						 << ", " << (int)color[1] << ", " << (int)color[2] << ", " << (int)color[3] << ").";
			}

			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			glDeleteBuffers(1, &buffer);
		}

		return ret;
	}

	long runQueryUniform(const CompiledProgram& program, const Uniform& uniform, std::set<GLuint>& usedLocations,
						 GLint max)
	{
		long ret = NO_ERROR;

		/*
		 glGetUniformLocation(program, name);
		 Query passes if returned value is unique in current program, matches
		 explicit location (if passed in GLSL code) and is less than value of
		 GL_MAX_UNIFORM_LOCATIONS.

		 glGetProgramResourceLocation(program, GL_UNIFIORM, name);
		 Query passes if returned value matches value returned from
		 glGetUniformLocation().
		 */

		if (uniform.type.isStruct())
		{
			for (size_t i = 0; i < uniform.childUniforms.size(); i++)
			{
				ret |= runQueryUniform(program, uniform.childUniforms[i], usedLocations, max);
			}
		}
		else
		{
			for (int arrayElem = 0; arrayElem < uniform.type.arraySize; arrayElem++)
			{

				/* Location that is taken by this uniform (even if not used).*/
				GLint reservedLocation = -1;
				if (!uniform.location.isImplicit(program.stages))
				{
					reservedLocation = uniform.location.val + arrayElem;
				}

				//optimization: for continuous arrays run queries at the beging and end only.
				bool runQueries = uniform.location.isImplicit(program.stages) ||
								  (arrayElem < 1000 || arrayElem > uniform.type.arraySize - 1000);

				if (runQueries)
				{
					std::ostringstream name;
					name << uniform.getName();
					uniform.type.streamArrayStr(name, arrayElem);
					GLint returned = glGetUniformLocation(program.name, name.str().c_str());

					GLint returnedPIQ = glGetProgramResourceLocation(program.name, GL_UNIFORM, name.str().c_str());

					if (returned != returnedPIQ)
					{
						ret |= ERROR;
						Logger()
							<< "Locations of  uniform \"" << name.str()
							<< "\" returned by glGetUniformLocation and differ glGetProgramResourceLocation differ: "
							<< returned << " != " << returnedPIQ << ".";
					}

					bool used = false;
					for (size_t i = 0; i < program.stages.size(); i++)
					{
						used |= uniform.declOccurence.occurs(program.stages[i]) &&
								uniform.usageOccurence.occurs(program.stages[i]);
					}

					if (!uniform.location.isImplicit(program.stages))
					{
						//Validate uniform location against explicit value
						GLint expected = reservedLocation;
						if (!(expected == returned || (!used && returned == -1)))
						{
							ret |= ERROR;
							Logger() << "Unexpected uniform \"" << name.str() << "\" location: expected " << expected
									 << ", got " << returned << ".";
						}
					}
					else
					{
						//Check if location > 0 if used;
						if (used)
						{
							if (returned < 0)
							{
								ret |= ERROR;
								Logger() << "Unexpected uniform \"" << name.str()
										 << "\" location: expected positive value, got " << returned << ".";
							}
							else
							{
								reservedLocation = returned;
							}
						}
					}

					if (returned >= 0)
					{
						//check if location is less than max

						if (returned >= max)
						{
							ret |= ERROR;
							Logger() << "Uniform \"" << name.str() << "\" returned location (" << returned
									 << ") is greater than implementation dependent limit (" << max << ").";
						}
					}
				} //if (runQueries)

				//usedLocations is always checked (even if queries were not run.
				if (reservedLocation >= 0)
				{
					//check if location is unique
					if (usedLocations.find(reservedLocation) != usedLocations.end())
					{
						ret |= ERROR;
						Logger() << "Uniform location (" << reservedLocation << ") is not unique.";
					}
					usedLocations.insert(reservedLocation);
				}
			}
		}
		return ret;
	}

	long runQueryUniformSubroutine(const CompiledProgram& program, GLenum stage,
								   const SubroutineUniform& subroutineUniform, std::set<GLuint>& usedLocations,
								   GLint max)
	{
		long ret = NO_ERROR;
		/*
		 glGetSubroutineUniformLocation(program, shaderType, name)
		 Query passes if returned value is unique in current program stage,
		 matches explicit location (if passed in GLSL code) and is less than
		 value of GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS.

		 glGetProgramResourceLocation(program, GL_(VERTEX|FRAGMENT|COMPUTE|...
		 ..._SUBROUTINE_UNIFORM, name)
		 Query passes if returned value matches value returned from
		 glGetUniformLocation().
		 */

		for (int arrayElem = 0; arrayElem < subroutineUniform.arraySize; arrayElem++)
		{
			std::ostringstream name;
			name << subroutineUniform.getName();

			subroutineUniform.streamArrayStr(name, arrayElem);

			GLint returned = glGetSubroutineUniformLocation(program.name, stage, name.str().c_str());

			glw::GLenum piqStage = 0;
			switch (stage)
			{
			case GL_VERTEX_SHADER:
				piqStage = GL_VERTEX_SUBROUTINE_UNIFORM;
				break;
			case GL_FRAGMENT_SHADER:
				piqStage = GL_FRAGMENT_SUBROUTINE_UNIFORM;
				break;
			case GL_COMPUTE_SHADER:
				piqStage = GL_COMPUTE_SUBROUTINE_UNIFORM;
				break;
			default:
				assert(0);
			}

			GLint returnedPIQ = glGetProgramResourceLocation(program.name, piqStage, name.str().c_str());

			if (returned != returnedPIQ)
			{
				ret |= ERROR;
				Logger() << "Locations of subrutine uniform \"" << name.str()
						 << "\" returned by glGetUniformLocation and differ glGetProgramResourceLocation differ: "
						 << returned << " != " << returnedPIQ << ".";
			}

			bool used = subroutineUniform.defOccurence.occurs(stage) && subroutineUniform.used;

			GLint reservedLocation = -1;

			if (!subroutineUniform.location.isImplicit(std::vector<glw::GLenum>(1, stage)))
			{
				//Validate uniform location against explicit value
				GLint expected = subroutineUniform.location.val + arrayElem;
				if (!(expected == returned || (!used && returned == -1)))
				{
					ret |= ERROR;
					Logger() << "Unexpected subroutine uniform \"" << name.str() << "\" location: expected " << expected
							 << ", got " << returned << ".";
				}

				reservedLocation = expected;
			}
			else
			{
				//Check if location > 0 if used;
				if (used)
				{
					if (returned < 0)
					{
						ret |= ERROR;
						Logger() << "Unexpected subroutine uniform \"" << name.str()
								 << "\" location: expected positive value, got " << returned << ".";
					}
					else
					{
						reservedLocation = returned;
					}
				}
			}

			if (reservedLocation >= 0)
			{
				//check if location is unique
				if (usedLocations.find(reservedLocation) != usedLocations.end())
				{
					ret |= ERROR;
					Logger() << "Subroutine uniform \"" << name.str() << "\" location (" << reservedLocation
							 << ") is not unique.";
				}
				usedLocations.insert(reservedLocation);
			}

			if (returned >= 0)
			{
				//check if location is less than max

				if (returned >= max)
				{
					ret |= ERROR;
					Logger() << "Subroutine uniform \"" << name.str() << "\" returned location (" << returned
							 << ") is greater than implementation dependent limit (" << max << ").";
				}
			}
		}
		return ret;
	}

	long runQueryUniformSubroutineFunction(const CompiledProgram& program, GLenum stage,
										   const SubroutineFunction& subroutineFunction, std::set<GLuint>& usedIndices,
										   GLint max, bool used)
	{
		long ret = NO_ERROR;
		/*
		 glGetSubroutineIndex(program, shaderType, name)
		 Query passes if returned value is unique in current program stage,
		 matches explicit index (if passed in GLSL code) and is less than value
		 of GL_MAX_SUBROUTINES.

		 glGetProgramResourceIndex(program, GL_(VERTEX|FRAGMENT|COMPUTE|...
		 ..._SUBROUTINE, name)
		 Query passes if returned value matches value returned from
		 glGetSubroutineIndex().
		 */

		std::string name = subroutineFunction.getName();

		GLint returned = glGetSubroutineIndex(program.name, stage, name.c_str());

		glw::GLenum piqStage = 0;
		switch (stage)
		{
		case GL_VERTEX_SHADER:
			piqStage = GL_VERTEX_SUBROUTINE;
			break;
		case GL_FRAGMENT_SHADER:
			piqStage = GL_FRAGMENT_SUBROUTINE;
			break;
		case GL_COMPUTE_SHADER:
			piqStage = GL_COMPUTE_SUBROUTINE;
			break;
		default:
			assert(0);
		}

		GLint returnedPIQ = glGetProgramResourceIndex(program.name, piqStage, name.c_str());

		if (returned != returnedPIQ)
		{
			ret |= ERROR;
			Logger() << "Indices of subroutine function \"" << name
					 << "\" returned by glGetSubroutineIndex and differ glGetProgramResourceIndex differ: " << returned
					 << " != " << returnedPIQ << ".";
		}

		GLint reservedIndex = -1;

		if (!subroutineFunction.index.isImplicit(std::vector<glw::GLenum>(1, stage)))
		{
			//Validate uniform location against explicit value
			GLint expected = subroutineFunction.index.val;
			if (!(expected == returned || (!used && returned == -1)))
			{
				ret |= ERROR;
				Logger() << "Unexpected subroutine function \"" << name << "\" index: expected " << expected << ", got "
						 << returned << ".";
			}

			reservedIndex = expected;
		}
		else
		{
			//Check if location > 0 if used;
			if (used)
			{
				if (returned < 0)
				{
					ret |= ERROR;
					Logger() << "Unexpected subroutine function \"" << name << "\" index: expected positive value, got "
							 << returned << ".";
				}
				else
				{
					reservedIndex = returned;
				}
			}
		}

		if (reservedIndex >= 0)
		{
			//check if location is unique
			if (usedIndices.find(reservedIndex) != usedIndices.end())
			{
				ret |= ERROR;
				Logger() << "Subroutine function \"" << name << "\" index (" << reservedIndex << ") is not unique.";
			}
			usedIndices.insert(reservedIndex);
		}

		if (returned >= 0)
		{
			//check if location is less than max

			if (returned >= max)
			{
				ret |= ERROR;
				Logger() << "Subroutine function \"" << name << "\" returned index (" << returned
						 << ") is greater than implementation dependent limit (" << max << ").";
			}
		}

		return ret;
	}

	long runQueryProgram(const CompiledProgram& program, const std::vector<Uniform>& uniforms,
						 const std::vector<SubroutineUniform>& subroutineUniforms)
	{
		long ret = NO_ERROR;

		{
			std::set<GLuint> usedLocations;

			GLint max;
			glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max);

			for (size_t i = 0; i < uniforms.size(); i++)
			{
				ret |= runQueryUniform(program, uniforms[i], usedLocations, max);
			}
		}

		if (subroutineUniforms.size())
		{
			GLint maxLocation, maxIndex;
			glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxLocation);
			glGetIntegerv(GL_MAX_SUBROUTINES, &maxIndex);

			for (size_t stage = 0; stage < program.stages.size(); stage++)
			{
				std::set<GLuint> usedLocations;
				std::set<GLuint> usedIndices;
				for (size_t i = 0; i < subroutineUniforms.size(); i++)
				{
					ret |= runQueryUniformSubroutine(program, program.stages[stage], subroutineUniforms[i],
													 usedLocations, maxLocation);
					for (size_t fn = 0; fn < subroutineUniforms[i].functions.fn.size(); fn++)
					{
						ret |= runQueryUniformSubroutineFunction(
							program, program.stages[stage], subroutineUniforms[i].functions.fn[fn], usedIndices,
							maxIndex,
							subroutineUniforms[i].defOccurence.occurs(program.stages[stage]) &&
								subroutineUniforms[i].used);
					}
				}
			}
		}

		return ret;
	}

protected:
	UniformValueGenerator uniformValueGenerator;
	UniformStructCounter  uniformStructCounter;

	long doRun(std::vector<SubroutineUniform>& subroutineUniforms)
	{
		assert(subroutineUniforms.size());
		std::vector<Uniform> noUniforms;
		return doRun(noUniforms, subroutineUniforms);
	}

	long doRun(std::vector<Uniform>& uniforms)
	{
		assert(uniforms.size());
		std::vector<SubroutineUniform> noSubroutineUniforms;
		return doRun(uniforms, noSubroutineUniforms);
	}

	long doRunNegativeCompile(const std::string additionalDef)
	{
		std::vector<Uniform>		   noUniforms;
		std::vector<SubroutineUniform> noSubroutineUniforms;
		return doRun(noUniforms, noSubroutineUniforms, additionalDef, true);
	}

	long doRunNegativeLink(std::vector<Uniform>& uniforms)
	{
		std::vector<SubroutineUniform> noSubroutineUniforms;
		return doRun(uniforms, noSubroutineUniforms, "", false, true);
	}

	long doRunNegativeLink(std::vector<SubroutineUniform>& subroutineUniforms)
	{
		std::vector<Uniform> noUniforms;
		return doRun(noUniforms, subroutineUniforms, "", false, true);
	}

	long doRun(std::vector<Uniform>& uniforms, std::vector<SubroutineUniform>& subroutineUniforms,
			   std::string additionalDef = "", bool negativeCompile = false, bool negativeLink = false)
	{
		long		ret				  = NO_ERROR;
		std::string parentUniformName = "";
		for (size_t i = 0; i < uniforms.size(); i++)
		{
			std::ostringstream name;
			name << "u" << i;
			uniforms[i].setName(parentUniformName, name.str());
		}
		int subroutineTypeCounter	 = 0;
		int subroutineFunctionCounter = 0;
		for (size_t i = 0; i < subroutineUniforms.size(); i++)
		{
			std::ostringstream name;
			name << "u" << i + uniforms.size();
			subroutineUniforms[i].setName(name.str());
			if (!subroutineUniforms[i].functions.getTypeName().size())
			{
				subroutineUniforms[i].functions.setTypeName(subroutineTypeCounter++);
				for (size_t fn = 0; fn < subroutineUniforms[i].functions.fn.size(); fn++)
				{
					subroutineUniforms[i].functions.fn[fn].setName(subroutineFunctionCounter++);
				}
			}
		}

		GLfloat coords[] = {
			1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
		};

		GLuint vbo, vao;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		std::vector<CompiledProgram> programs;
		ret |= CreatePrograms(programs, uniforms, subroutineUniforms, additionalDef, negativeCompile, negativeLink);

		for (size_t i = 0; i < programs.size() && ret == NO_ERROR && !negativeCompile && !negativeLink; i++)
		{
			ret |= runExecuteProgram(programs[i], uniforms, subroutineUniforms);
			ret |= runQueryProgram(programs[i], uniforms, subroutineUniforms);
		}

		glUseProgram(0);

		DeletePrograms(programs);

		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);

		return ret;
	}
};

class UniformLoc : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform vec4 u0;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2), DefOccurence::FSH_OR_CSH));
		return doRun(uniforms);
	}
};

class UniformLocNonDec : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 0x0a) uniform vec4 u0;
		//layout (location = 010) uniform vec4  u1;
		std::vector<Uniform> uniforms;
		uniforms.push_back(
			Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(0x0a, Loc::Hex), DefOccurence::FSH_OR_CSH));
		uniforms.push_back(
			Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(010, Loc::Oct), DefOccurence::FSH_OR_CSH));
		return doRun(uniforms);
	}
};

class UniformLocMultipleStages : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform vec4 u0;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2)));
		return doRun(uniforms);
	}
};

class UniformLocMultipleUniforms : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform vec4 u0;
		//layout (location = 3) uniform vec4 u1;
		//layout (location = 5) uniform vec4 u2;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(3)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(5)));
		return doRun(uniforms);
	}
};

class UniformLocTypesMix : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform float u0;
		//layout (location = 3) uniform vec3  u1;
		//layout (location = 0) uniform uint  u2;
		//layout (location = 1) uniform ivec3 u3;
		//layout (location = 4) uniform mat2  u4;
		//layout (location = 7) uniform mat2  u5;
		//layout (location = 5) uniform mat2  u6;
		//layout (location = 6) uniform mat3  u7;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(2)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC3, Loc::C(3)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_UNSIGNED_INT, Loc::C(0)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_INT_VEC3, Loc::C(1)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT2, Loc::C(4)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT2, Loc::C(7)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT2, Loc::C(5)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT3, Loc::C(6)));
		return doRun(uniforms);
	}
};

class UniformLocTypesMat : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::vector<Uniform> uniforms;
		//layout (location = 1) uniform mat2x3   u0;
		//layout (location = 2) uniform mat3x2   u1;
		//layout (location = 0) uniform mat2     u2;
		//layout (location = 3) uniform imat3x4  u3;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT2x3, Loc::C(1)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT3x2, Loc::C(2)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT2, Loc::C(0)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_MAT4x3, Loc::C(3)));
		return doRun(uniforms);
	}
};

class UniformLocTypesSamplers : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 1) uniform sampler2D s0[3];
		//layout (location = 13) uniform sampler2D s1;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_SAMPLER_2D, 3), Loc::C(1)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_SAMPLER_2D, Loc::C(13)));

		std::vector<GLuint>				   texUnits;
		std::vector<std::vector<GLubyte> > colors;

		for (size_t i = 0; i < uniforms.size(); i++)
		{
			for (int elem = 0; elem < uniforms[i].type.arraySize; elem++)
			{
				texUnits.push_back(uniforms[i].value.iValues[elem]);

				std::vector<GLubyte> color(4);
				color[0] = static_cast<GLubyte>(255. * uniforms[i].value.fValues[4 * elem + 0] + 0.5);
				color[1] = static_cast<GLubyte>(255. * uniforms[i].value.fValues[4 * elem + 1] + 0.5);
				color[2] = static_cast<GLubyte>(255. * uniforms[i].value.fValues[4 * elem + 2] + 0.5);
				color[3] = static_cast<GLubyte>(255. * uniforms[i].value.fValues[4 * elem + 3] + 0.5);
				colors.push_back(color);
			}
		}

		std::vector<GLuint> textures(texUnits.size());
		glGenTextures((GLsizei)(textures.size()), &textures[0]);

		for (size_t i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + texUnits[i]);
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &colors[i][0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glActiveTexture(GL_TEXTURE0);
		long ret = doRun(uniforms);
		glDeleteTextures((GLsizei)(textures.size()), &textures[0]);
		return ret;
	}
};

class UniformLocTypesStructs : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		/**
		 * This test case uses following uniform declarations:
		 *
		 * struct S {
		 *   vec4  u0;
		 *   float u1[2];
		 *   mat2  u2;
		 * };
		 * layout (location = 1) uniform S s0[3];
		 * layout (location = 13) uniform S s1;
		 */

		std::vector<UniformType> members;
		members.push_back(GL_FLOAT_VEC4);
		members.push_back(UniformType(GL_FLOAT, 2));
		members.push_back(GL_FLOAT_MAT2);
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(uniformStructCounter, members, 3), Loc::C(1)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(uniformStructCounter, members), Loc::C(13)));
		return doRun(uniforms);
	}
};

class UniformLocArraysNonSpaced : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform float[3] u0;
		//layout (location = 5) uniform vec3[2]  u1;
		//layout (location = 7) uniform int[3]   u2;
		//layout (location = 10) uniform ivec4   u3;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, 3), Loc::C(2)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT_VEC3, 2), Loc::C(5)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_INT, 3), Loc::C(7)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_INT_VEC4, Loc::C(10)));
		return doRun(uniforms);
	}
};

class UniformLocArraySpaced : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform float     u0;
		//layout (location = 5) uniform vec3[2]   u1;
		//layout (location = 8) uniform int[3]    u2;
		//layout (location = 12) uniform ivec4[1] u3;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(2)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT_VEC3, 2), Loc::C(5)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_INT, 3), Loc::C(8)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_INT_VEC4, 1), Loc::C(12)));
		return doRun(uniforms);
	}
};

class UniformLocArrayofArrays : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform float[2][3]  u0;
		//layout (location = 8) uniform vec3[2][2]   u1;
		//layout (location = 12) uniform float       u2;
		std::vector<Uniform> uniforms;
		{
			std::vector<int> arraySizesSegmented(2);
			arraySizesSegmented[0] = 2;
			arraySizesSegmented[1] = 3;
			uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, arraySizesSegmented), Loc::C(2)));
		}
		{
			std::vector<int> arraySizesSegmented(2);
			arraySizesSegmented[0] = arraySizesSegmented[1] = 2;
			uniforms.push_back(
				Uniform(uniformValueGenerator, UniformType(GL_FLOAT_VEC3, arraySizesSegmented), Loc::C(8)));
		}
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(12)));
		return doRun(uniforms);
	}
};

class UniformLocMixWithImplicit : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 0) uniform float     u0;
		//layout (location = 2) uniform vec3      u1;
		//layout (location = 3) uniform int       u2;

		//uniform float     u0;
		//uniform vec3      u1;
		//uniform int       u2;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(0, DefOccurence::FSH_OR_CSH)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC3, Loc::C(2, DefOccurence::FSH_OR_CSH)));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_INT, Loc::C(3, DefOccurence::FSH_OR_CSH)));
		return doRun(uniforms);
	}
};

class UniformLocMixWithImplicit2 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//uniform float[3] u0;
		//layout (location = 3) uniform vec3[2]  u1;
		//uniform int[3]   u2;
		//layout (location = 8) uniform ivec4   u3;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, 3), Loc::Implicit()));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT_VEC3, 2), Loc::C(3)));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_INT, 3), Loc::Implicit()));
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_INT_VEC4, 2), Loc::C(8)));
		return doRun(uniforms);
	}
};

class UniformLocMixWithImplicit3 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 0) uniform float     u0; //unused
		//layout (location = 2) uniform vec3      u1; //unused
		//layout (location = 3) uniform int       u2; //unused

		//uniform float     u3;
		//uniform vec3      u4;
		//uniform int       u5;
		std::vector<Uniform> uniforms;
		uniforms.push_back(
			Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(0), DefOccurence::ALL_SH, DefOccurence::NONE_SH));
		uniforms.push_back(
			Uniform(uniformValueGenerator, GL_FLOAT_VEC3, Loc::C(2), DefOccurence::ALL_SH, DefOccurence::NONE_SH));
		uniforms.push_back(
			Uniform(uniformValueGenerator, GL_INT, Loc::C(3), DefOccurence::ALL_SH, DefOccurence::NONE_SH));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::Implicit()));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC3, Loc::Implicit()));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_INT, Loc::Implicit()));
		return doRun(uniforms);
	}
};

class UniformLocMixWithImplicitMax : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		long ret = NO_ERROR;

		GLint max;
		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max);

		const int implicitCount = 1;

		int tests[3] = { 0, 3, max - implicitCount };

		for (int test = 0; test < 3; test++)
		{
			std::vector<Uniform> uniforms;

			//for performance reasons fill-up all avaliable locations with an unused arrays.
			if (tests[test] > 0)
			{
				//[0..test - 1]
				uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, tests[test]), Loc::C(0),
										   DefOccurence::ALL_SH, DefOccurence::NONE_SH));
				assert(uniforms[uniforms.size() - 1].location.val + uniforms[uniforms.size() - 1].type.arraySize ==
					   tests[test]);
			}

			if (tests[test] < max - implicitCount)
			{
				//[test + 1..max]
				uniforms.push_back(
					Uniform(uniformValueGenerator, UniformType(GL_FLOAT, max - implicitCount - tests[test]),
							Loc::C(tests[test] + implicitCount), DefOccurence::ALL_SH, DefOccurence::NONE_SH));
				assert(uniforms[uniforms.size() - 1].location.val + uniforms[uniforms.size() - 1].type.arraySize ==
					   max);
			}

			uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::Implicit()));
			ret |= doRun(uniforms);
		}
		return ret;
	}
};

class UniformLocMixWithImplicitMaxArray : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		long ret = NO_ERROR;

		GLint max;
		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max);

		const int implicitCount = 3;

		int tests[3] = { 0, 3, max - 4 };

		for (int test = 0; test < 3; test++)
		{
			std::vector<Uniform> uniforms;

			//for performance reasons fill-up all avaliable locations with an unused arrays.
			if (tests[test] > 0)
			{
				//[0..test - 1]
				uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, tests[test]), Loc::C(0),
										   DefOccurence::ALL_SH, DefOccurence::NONE_SH));
				assert(uniforms[uniforms.size() - 1].location.val + uniforms[uniforms.size() - 1].type.arraySize ==
					   tests[test]);
			}

			if (tests[test] < max - implicitCount)
			{
				//[test + 3 ..max]
				uniforms.push_back(
					Uniform(uniformValueGenerator, UniformType(GL_FLOAT, max - implicitCount - tests[test]),
							Loc::C(tests[test] + implicitCount), DefOccurence::ALL_SH, DefOccurence::NONE_SH));
				assert(uniforms[uniforms.size() - 1].location.val + uniforms[uniforms.size() - 1].type.arraySize ==
					   max);
			}
			uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, implicitCount), Loc::Implicit()));
			ret |= doRun(uniforms);
		}
		return ret;
	}
};

class UniformLocImplicitInSomeStages : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//One shader: uniform float u0;
		//Another shader: layout (location = 3) uniform float  u0;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(3, DefOccurence::FSH_OR_CSH)));
		return doRun(uniforms);
	}
};

class UniformLocImplicitInSomeStages2 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//One shader: uniform float u0;
		//Another shader: layout (location = 3) uniform float  u0; //not used!
		std::vector<Uniform> uniforms;

		//location only in fsh, declaration in all shaders, usage in all shaders but fsh.
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(3, DefOccurence::FSH_OR_CSH),
								   DefOccurence::ALL_SH, DefOccurence::ALL_BUT_FSH));
		return doRun(uniforms);
	}
};

class UniformLocImplicitInSomeStages3 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::vector<Uniform> uniforms;

		//location only in fsh, declaration in all shaders, usage in all shaders but fsh.
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(3, DefOccurence::FSH_OR_CSH),
								   DefOccurence::ALL_SH, DefOccurence::ALL_BUT_FSH));

		//location in all but fsh, declaration in all shaders, usage in fsh.
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(2, DefOccurence::ALL_BUT_FSH),
								   DefOccurence::ALL_SH, DefOccurence::FSH_OR_CSH));

		//location only in fsh, declaration in all shaders, usage in all shaders but fsh.
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, 3), Loc::C(7, DefOccurence::FSH_OR_CSH),
								   DefOccurence::ALL_SH, DefOccurence::ALL_BUT_FSH));

		//location in all but fsh, declaration in all shaders, usage in fsh.
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, 3),
								   Loc::C(4, DefOccurence::ALL_BUT_FSH), DefOccurence::ALL_SH,
								   DefOccurence::FSH_OR_CSH));

		//location only in vsh, declaration in all shaders, usage in all shaders but vsh.
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(0, DefOccurence::VSH), DefOccurence::ALL_SH,
								   DefOccurence::ALL_BUT_VSH));

		//location only in vsh, declaration in all shaders, usage in all shaders but vsh.
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::C(1, DefOccurence::ALL_BUT_FSH),
								   DefOccurence::ALL_SH, DefOccurence::ALL_BUT_VSH));

		return doRun(uniforms);
	}
};

class UniformLocNegativeCompileNonNumberLiteral : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::string def = "layout (location = x) uniform float u0;";
		return doRunNegativeCompile(def);
	}
};

class UniformLocNegativeCompileNonConstLoc : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::string def = NL "const int i = 1;" NL "layout (location = i) uniform float u0;";
		return doRunNegativeCompile(def);
	}
};

class UniformLocNegativeLinkLocationReused1 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = 2) uniform float u0;
		//layout (location = 2) uniform float u1;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2), DefOccurence::FSH_OR_CSH));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2), DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(uniforms);
	}
};

class UniformLocNegativeLinkLocationReused2 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		///layout (location = 2) uniform float u0;
		//layout (location = 2) uniform float u1;
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2), DefOccurence::FSH_OR_CSH));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(2), DefOccurence::ALL_BUT_FSH));
		return doRunNegativeLink(uniforms);
	}
};

class UniformLocNegativeLinkMaxLocation : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout (location = X) uniform float u0;
		//Where X is substituted with value of GL_MAX_UNIFORM_LOCATIONS.

		GLint max;
		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max);

		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT_VEC4, Loc::C(max), DefOccurence::FSH_OR_CSH));

		return doRunNegativeLink(uniforms);
	}
};

class UniformLocNegativeLinkMaxMaxNumOfLocation : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		GLint max;
		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max);
		std::vector<Uniform> uniforms;
		uniforms.push_back(Uniform(uniformValueGenerator, UniformType(GL_FLOAT, max), Loc::C(0),
								   DefOccurence::FSH_OR_CSH, DefOccurence::NONE_SH));
		uniforms.push_back(Uniform(uniformValueGenerator, GL_FLOAT, Loc::Implicit(), DefOccurence::ALL_BUT_FSH));
		return doRunNegativeLink(uniforms);
	}
};

class SubRoutineLoc : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		//one shader:
		//subroutine vec4 st0(float param);
		//subroutine(st0) vec4 sf0(float param) { .... };
		//subroutine(st0) vec4 sf1(float param) { .... };
		SubroutineFunctionSet functions_st0(uniformValueGenerator, 2);

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 2) subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(2), 0, DefOccurence::FSH_OR_CSH));
		return doRun(subroutineUniforms);
	}
};

class SubRoutineLocNonDecimal : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//one shader:
		//subroutine vec4 st0(float param);
		//subroutine(st0) vec4 sf0(float param) { .... };
		//subroutine(st0) vec4 sf1(float param) { .... };
		SubroutineFunctionSet functions_st0(uniformValueGenerator, 2);

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 0x0a) subroutine uniform st0 u0;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(0x0a, Loc::Hex), 0,
													   DefOccurence::FSH_OR_CSH));
		//layout(location = 010 ) subroutine uniform st0 u1;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(010, Loc::Oct), 0,
													   DefOccurence::FSH_OR_CSH));
		return doRun(subroutineUniforms);
	}
};

class SubRoutineLocAllStages : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//subroutine vec4 st0(float param);
		//subroutine(st0) vec4 sf0(float param) { .... };
		//subroutine(st0) vec4 sf1(float param) { .... };
		SubroutineFunctionSet functions_st0(uniformValueGenerator, 2);

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 2) subroutine uniform st0 u0;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(2)));
		return doRun(subroutineUniforms);
	}
};

class SubRoutineLocArrays : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		//subroutine vec4 st0(float param);
		//subroutine(st0) vec4 sf0(float param) { .... };
		//subroutine(st0) vec4 sf1(float param) { .... };
		SubroutineFunctionSet functions_st0(uniformValueGenerator, 2);

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 1) subroutine uniform st0 u0[2];
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(1), 2));
		return doRun(subroutineUniforms);
	}
};

class SubRoutineLocArraysMix : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//subroutine vec4 st0(float param);
		//subroutine(st0) vec4 sf0(float param) { .... };
		//subroutine(st0) vec4 sf1(float param) { .... };
		SubroutineFunctionSet functions_st0(uniformValueGenerator, 2);

		//subroutine vec4 st1(float param);
		//subroutine(st1) vec4 sf2(float param) { .... };
		//subroutine(st1) vec4 sf3(float param) { .... };
		SubroutineFunctionSet functions_st1(uniformValueGenerator, 2);

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 1) subroutine uniform st0 u0[2];
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(1), 2));

		////layout(location = 3) subroutine uniform st0 u1[2][3];
		std::vector<int> arraySizesSegmented(2);
		arraySizesSegmented[0] = 2;
		arraySizesSegmented[1] = 3;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(3), arraySizesSegmented));

		//layout(location = 9) subroutine uniform st1 u2;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st1, Loc::C(9)));

		return doRun(subroutineUniforms);
	}
};

class SubRoutineLocMixWithImplicit : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//subroutine vec4 st0(float param);
		//subroutine(st0) vec4 sf0(float param) { .... };
		//subroutine(st0) vec4 sf1(float param) { .... };
		SubroutineFunctionSet functions_st0(uniformValueGenerator, 2);

		std::vector<SubroutineUniform> subroutineUniforms;
		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit()));
		//layout(location = 1 ) subroutine uniform st0 u1;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(0)));
		//layout(location = 0 ) subroutine uniform st0 u2;
		subroutineUniforms.push_back(SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(1)));

		return doRun(subroutineUniforms);
	}
};

class SubRoutineLocCompilationNonNumberLiteral : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		std::string def =
			NL "subroutine vec4 st0(float param);" NL "subroutine(st0) vec4 sf0(float param) { return param; }" NL
			   "layout(location = x ) subroutine uniform st0 u0;";

		return doRunNegativeCompile(def);
	}
};

class SubRoutineLocCompilationNonConstLoc : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::string def = NL "const int i = 1;" NL "subroutine vec4 st0(float param);" NL
							 "subroutine(st0) vec4 sf0(float param) { return param; }" NL
							 "layout(location = i ) subroutine uniform st0 u0;";
		return doRunNegativeCompile(def);
	}
};

class SubRoutineLocLinkLocationReused1 : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout(location = 1) subroutine uniform st0 u0;
		//layout(location = 1) subroutine uniform st0 u0;
		SubroutineFunctionSet		   functions_st0(uniformValueGenerator, 2);
		std::vector<SubroutineUniform> subroutineUniforms;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(2), 0, DefOccurence::FSH_OR_CSH));
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(2), 0, DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(subroutineUniforms);
	}
};

class SubRoutineLocLinkLocationMaxLocation : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//layout(location = N) subroutine uniform st0 u0;
		//Where X is substituted with value of GL_MAX_UNIFORM_LOCATIONS.

		GLint max;
		glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &max);
		SubroutineFunctionSet		   functions_st0(uniformValueGenerator, 2);
		std::vector<SubroutineUniform> subroutineUniforms;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(max), 0, DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(subroutineUniforms);
	}
};

class SubRoutineLocLinkMaxNumOfLocations : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		GLint max;
		glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &max);
		SubroutineFunctionSet		   functions_st0(uniformValueGenerator, 2);
		std::vector<SubroutineUniform> subroutineUniforms;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(0), max, DefOccurence::FSH_OR_CSH, false));
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(subroutineUniforms);
	}
};

class SubroutineIndex : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//one shader:

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 1) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(1)));
		//layout(index = 2) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::FSH_OR_CSH));
		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexNonDecimal : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//one shader:

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 0x0a) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(0x0a, Index::Hex)));
		//layout(index = 010 ) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(010, Index::Oct)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::FSH_OR_CSH));

		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexLoc : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		//one shader:

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 1) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(1)));
		//layout(index = 2) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 3) subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(3), 0, DefOccurence::FSH_OR_CSH));
		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexNonCont : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//one shader:

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 0) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(0)));
		//layout(index = 2) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 2) subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(2), 0, DefOccurence::FSH_OR_CSH));
		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexMultUniforms : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		//one shader:

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 1) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(1)));
		//layout(index = 3) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(3)));

		//subroutine vec4 st1(float param);
		SubroutineFunctionSet functions_st1(uniformValueGenerator);
		//layout(index = 2) subroutine(st1) vec4 sf2(float param) { .... };
		functions_st1.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));
		//layout(index = 0) subroutine(st1) vec4 sf3(float param) { .... };
		functions_st1.push_back(SubroutineFunction(uniformValueGenerator, Index::C(0)));

		std::vector<SubroutineUniform> subroutineUniforms;
		//layout(location = 1) subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(1), 0, DefOccurence::FSH_OR_CSH));
		//layout(location = 9) subroutine uniform st1 u1;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st1, Loc::C(9), 0, DefOccurence::FSH_OR_CSH));

		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexAllstages : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 1) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(1)));
		//layout(index = 2) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::ALL_SH));
		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexMixImplicit : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::Implicit()));
		//layout(index = 1) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(1)));
		//layout(index = 0) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(0)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//layout(location = 2) subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::C(2), 0, DefOccurence::FSH_OR_CSH));
		return doRun(subroutineUniforms);
	}
};

class SubroutineIndexNegativeCompilationNonNumberLiteral : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::string def = NL "subroutine vec4 st0(float param);" NL
							 "layout(index = x) subroutine(st0) vec4 sf1(float param) { return param; };";
		return doRunNegativeCompile(def);
	}
};

class SubroutineIndexNegativeCompilationNonConstIndex : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		std::string def =
			NL "const int i = 1;" NL "layout(index = i) subroutine(st0) vec4 sf1(float param) { return param; };";
		return doRunNegativeCompile(def);
	}
};

class SubroutineIndexNegativeLinkIndexReused : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = 2) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));
		//layout(index = 2) subroutine(st0) vec4 sf1(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(2)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(subroutineUniforms);
	}
};

class SubroutineIndexNegativeLinkMaxIndex : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{

		GLint max;
		glGetIntegerv(GL_MAX_SUBROUTINES, &max);

		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);
		//layout(index = N) subroutine(st0) vec4 sf0(float param) { .... };
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(max)));

		std::vector<SubroutineUniform> subroutineUniforms;

		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(subroutineUniforms);
	}
};

class SubroutineIndexNegativeLinkMaxNumOfIndices : public ExplicitUniformLocationCaseBase
{
	virtual long Run()
	{
		//subroutine vec4 st0(float param);
		SubroutineFunctionSet functions_st0(uniformValueGenerator);

		glw::GLint max;
		glGetIntegerv(GL_MAX_SUBROUTINES, &max);

		for (int i = 0; i < max; i++)
		{
			//layout(index = N) subroutine(st0) vec4 sf0(float param) { .... };
			functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::C(i)));
		}
		functions_st0.push_back(SubroutineFunction(uniformValueGenerator, Index::Implicit()));

		std::vector<SubroutineUniform> subroutineUniforms;

		//subroutine uniform st0 u0;
		subroutineUniforms.push_back(
			SubroutineUniform(uniformValueGenerator, functions_st0, Loc::Implicit(), 0, DefOccurence::FSH_OR_CSH));
		return doRunNegativeLink(subroutineUniforms);
	}
};
}

ExplicitUniformLocationGLTests::ExplicitUniformLocationGLTests(glcts::Context& context)
	: TestCaseGroup(context, "explicit_uniform_location", "")
{
}

ExplicitUniformLocationGLTests::~ExplicitUniformLocationGLTests(void)
{
}

void ExplicitUniformLocationGLTests::init()
{
	using namespace glcts;

	Logger::setOutput(m_context.getTestContext().getLog());
	addChild(new TestSubcase(m_context, "uniform-loc", TestSubcase::Create<UniformLoc>));
	addChild(new TestSubcase(m_context, "uniform-loc-nondecimal", TestSubcase::Create<UniformLocNonDec>));
	addChild(new TestSubcase(m_context, "uniform-loc-all-stages", TestSubcase::Create<UniformLocMultipleStages>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-multiple-uniforms", TestSubcase::Create<UniformLocMultipleUniforms>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-mix", TestSubcase::Create<UniformLocTypesMix>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-mat", TestSubcase::Create<UniformLocTypesMat>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-structs", TestSubcase::Create<UniformLocTypesStructs>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-samplers", TestSubcase::Create<UniformLocTypesSamplers>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-arrays-nonspaced", TestSubcase::Create<UniformLocArraysNonSpaced>));
	addChild(new TestSubcase(m_context, "uniform-loc-arrays-spaced", TestSubcase::Create<UniformLocArraySpaced>));

	addChild(new TestSubcase(m_context, "uniform-loc-arrays-of-arrays", TestSubcase::Create<UniformLocArrayofArrays>));

	addChild(
		new TestSubcase(m_context, "uniform-loc-mix-with-implicit", TestSubcase::Create<UniformLocMixWithImplicit>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-mix-with-implicit2", TestSubcase::Create<UniformLocMixWithImplicit2>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-mix-with-implicit3", TestSubcase::Create<UniformLocMixWithImplicit3>));
	addChild(new TestSubcase(m_context, "uniform-loc-mix-with-implicit-max",
							 TestSubcase::Create<UniformLocMixWithImplicitMax>));
	addChild(new TestSubcase(m_context, "uniform-loc-mix-with-implicit-max-array",
							 TestSubcase::Create<UniformLocMixWithImplicitMaxArray>));

	addChild(new TestSubcase(m_context, "uniform-loc-implicit-in-some-stages",
							 TestSubcase::Create<UniformLocImplicitInSomeStages>));
	addChild(new TestSubcase(m_context, "uniform-loc-implicit-in-some-stages2",
							 TestSubcase::Create<UniformLocImplicitInSomeStages2>));
	addChild(new TestSubcase(m_context, "uniform-loc-implicit-in-some-stages3",
							 TestSubcase::Create<UniformLocImplicitInSomeStages3>));

	addChild(new TestSubcase(m_context, "uniform-loc-negative-compile-non-number-literal",
							 TestSubcase::Create<UniformLocNegativeCompileNonNumberLiteral>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-compile-nonconst-loc",
							 TestSubcase::Create<UniformLocNegativeCompileNonConstLoc>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-location-reused1",
							 TestSubcase::Create<UniformLocNegativeLinkLocationReused1>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-location-reused2",
							 TestSubcase::Create<UniformLocNegativeLinkLocationReused2>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-max-location",
							 TestSubcase::Create<UniformLocNegativeLinkMaxLocation>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-max-num-of-locations",
							 TestSubcase::Create<UniformLocNegativeLinkMaxMaxNumOfLocation>));

	addChild(new TestSubcase(m_context, "subroutine-loc", TestSubcase::Create<SubRoutineLoc>));
	addChild(new TestSubcase(m_context, "subroutine-loc-nondecimal", TestSubcase::Create<SubRoutineLocNonDecimal>));
	addChild(new TestSubcase(m_context, "subroutine-loc-all-stages", TestSubcase::Create<SubRoutineLocAllStages>));
	addChild(new TestSubcase(m_context, "subroutine-loc-arrays", TestSubcase::Create<SubRoutineLocArrays>));
	addChild(new TestSubcase(m_context, "subroutine-loc-arrays-mix", TestSubcase::Create<SubRoutineLocArraysMix>));
	addChild(new TestSubcase(m_context, "subroutine-loc-mix-with-implicit",
							 TestSubcase::Create<SubRoutineLocMixWithImplicit>));
	addChild(new TestSubcase(m_context, "subroutine-loc-negative-compilation-non-number-literal",
							 TestSubcase::Create<SubRoutineLocCompilationNonNumberLiteral>));
	addChild(new TestSubcase(m_context, "subroutine-loc-negative-compilation-nonconst-loc",
							 TestSubcase::Create<SubRoutineLocCompilationNonConstLoc>));
	addChild(new TestSubcase(m_context, "subroutine-loc-negative-link-location-reused1",
							 TestSubcase::Create<SubRoutineLocLinkLocationReused1>));
	addChild(new TestSubcase(m_context, "subroutine-loc-negative-link-location-max-location",
							 TestSubcase::Create<SubRoutineLocLinkLocationMaxLocation>));
	addChild(new TestSubcase(m_context, "subroutine-loc-negative-link-max-num-of-locations",
							 TestSubcase::Create<SubRoutineLocLinkMaxNumOfLocations>));
	addChild(new TestSubcase(m_context, "subroutine-index", TestSubcase::Create<SubroutineIndex>));
	addChild(new TestSubcase(m_context, "subroutine-index-nondecimal", TestSubcase::Create<SubroutineIndexNonDecimal>));
	addChild(new TestSubcase(m_context, "subroutine-index-loc", TestSubcase::Create<SubroutineIndexLoc>));
	addChild(
		new TestSubcase(m_context, "subroutine-index-non-continuous", TestSubcase::Create<SubroutineIndexNonCont>));
	addChild(new TestSubcase(m_context, "subroutine-index-multiple-uniforms",
							 TestSubcase::Create<SubroutineIndexMultUniforms>));
	addChild(new TestSubcase(m_context, "subroutine-index-all-stages", TestSubcase::Create<SubroutineIndexAllstages>));
	addChild(
		new TestSubcase(m_context, "subroutine-index-mix-implicit", TestSubcase::Create<SubroutineIndexMixImplicit>));
	addChild(new TestSubcase(m_context, "subroutine-index-negative-compilation-non-number-literal",
							 TestSubcase::Create<SubroutineIndexNegativeCompilationNonNumberLiteral>));
	addChild(new TestSubcase(m_context, "subroutine-index-negative-compilation-nonconst-index",
							 TestSubcase::Create<SubroutineIndexNegativeCompilationNonConstIndex>));
	addChild(new TestSubcase(m_context, "subroutine-index-negative-link-index-reused",
							 TestSubcase::Create<SubroutineIndexNegativeLinkIndexReused>));
	addChild(new TestSubcase(m_context, "subroutine-index-negative-link-location-maxindex",
							 TestSubcase::Create<SubroutineIndexNegativeLinkMaxIndex>));
	addChild(new TestSubcase(m_context, "subroutine-index-negative-link-max-num-of-indices",
							 TestSubcase::Create<SubroutineIndexNegativeLinkMaxNumOfIndices>));
}

ExplicitUniformLocationES31Tests::ExplicitUniformLocationES31Tests(glcts::Context& context)
	: TestCaseGroup(context, "explicit_uniform_location", "")
{
}

ExplicitUniformLocationES31Tests::~ExplicitUniformLocationES31Tests(void)
{
}

void ExplicitUniformLocationES31Tests::init()
{
	using namespace glcts;
	Logger::setOutput(m_context.getTestContext().getLog());
	addChild(new TestSubcase(m_context, "uniform-loc", TestSubcase::Create<UniformLoc>));
	addChild(new TestSubcase(m_context, "uniform-loc-nondecimal", TestSubcase::Create<UniformLocNonDec>));
	addChild(new TestSubcase(m_context, "uniform-loc-all-stages", TestSubcase::Create<UniformLocMultipleStages>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-multiple-uniforms", TestSubcase::Create<UniformLocMultipleUniforms>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-mix", TestSubcase::Create<UniformLocTypesMix>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-mat", TestSubcase::Create<UniformLocTypesMat>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-structs", TestSubcase::Create<UniformLocTypesStructs>));
	addChild(new TestSubcase(m_context, "uniform-loc-types-samplers", TestSubcase::Create<UniformLocTypesSamplers>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-arrays-nonspaced", TestSubcase::Create<UniformLocArraysNonSpaced>));
	addChild(new TestSubcase(m_context, "uniform-loc-arrays-spaced", TestSubcase::Create<UniformLocArraySpaced>));
	addChild(new TestSubcase(m_context, "uniform-loc-arrays-of-arrays", TestSubcase::Create<UniformLocArrayofArrays>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-mix-with-implicit", TestSubcase::Create<UniformLocMixWithImplicit>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-mix-with-implicit2", TestSubcase::Create<UniformLocMixWithImplicit2>));
	addChild(
		new TestSubcase(m_context, "uniform-loc-mix-with-implicit3", TestSubcase::Create<UniformLocMixWithImplicit3>));
	addChild(new TestSubcase(m_context, "uniform-loc-mix-with-implicit-max",
							 TestSubcase::Create<UniformLocMixWithImplicitMax>));
	addChild(new TestSubcase(m_context, "uniform-loc-mix-with-implicit-max-array",
							 TestSubcase::Create<UniformLocMixWithImplicitMaxArray>));
	addChild(new TestSubcase(m_context, "uniform-loc-implicit-in-some-stages",
							 TestSubcase::Create<UniformLocImplicitInSomeStages>));
	addChild(new TestSubcase(m_context, "uniform-loc-implicit-in-some-stages2",
							 TestSubcase::Create<UniformLocImplicitInSomeStages2>));
	addChild(new TestSubcase(m_context, "uniform-loc-implicit-in-some-stages3",
							 TestSubcase::Create<UniformLocImplicitInSomeStages3>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-compile-non-number-literal",
							 TestSubcase::Create<UniformLocNegativeCompileNonNumberLiteral>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-compile-nonconst-loc",
							 TestSubcase::Create<UniformLocNegativeCompileNonConstLoc>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-location-reused1",
							 TestSubcase::Create<UniformLocNegativeLinkLocationReused1>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-location-reused2",
							 TestSubcase::Create<UniformLocNegativeLinkLocationReused2>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-max-location",
							 TestSubcase::Create<UniformLocNegativeLinkMaxLocation>));
	addChild(new TestSubcase(m_context, "uniform-loc-negative-link-max-num-of-locations",
							 TestSubcase::Create<UniformLocNegativeLinkMaxMaxNumOfLocation>));
}
}
