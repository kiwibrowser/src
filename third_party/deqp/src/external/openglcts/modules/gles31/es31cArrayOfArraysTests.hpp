#ifndef _ES31CARRAYOFARRAYSTESTS_HPP
#define _ES31CARRAYOFARRAYSTESTS_HPP
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

#include <map>

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

namespace glcts
{
typedef enum {
	VAR_TYPE_BOOL,
	VAR_TYPE_INT,
	VAR_TYPE_UINT,
	VAR_TYPE_FLOAT,
	VAR_TYPE_VEC2,
	VAR_TYPE_VEC3,
	VAR_TYPE_VEC4,
	VAR_TYPE_BVEC2,
	VAR_TYPE_BVEC3,
	VAR_TYPE_BVEC4,
	VAR_TYPE_IVEC2,
	VAR_TYPE_IVEC3,
	VAR_TYPE_IVEC4,
	VAR_TYPE_UVEC2,
	VAR_TYPE_UVEC3,
	VAR_TYPE_UVEC4,
	VAR_TYPE_MAT2,
	VAR_TYPE_MAT3,
	VAR_TYPE_MAT4,
	VAR_TYPE_MAT2X2,
	VAR_TYPE_MAT2X3,
	VAR_TYPE_MAT2X4,
	VAR_TYPE_MAT3X2,
	VAR_TYPE_MAT3X3,
	VAR_TYPE_MAT3X4,
	VAR_TYPE_MAT4X2,
	VAR_TYPE_MAT4X3,
	VAR_TYPE_MAT4X4,
	/** List of all supported interface resource types. */
	VAR_TYPE_IMAGEBUFFER,
	VAR_TYPE_IIMAGEBUFFER,
	VAR_TYPE_UIMAGEBUFFER,
	VAR_TYPE_SAMPLERBUFFER,
	VAR_TYPE_ISAMPLERBUFFER,
	VAR_TYPE_USAMPLERBUFFER,
	/** List of all supported opaque types. */
	//Floating Point Sampler Types (opaque)
	VAR_TYPE_SAMPLER2D,
	VAR_TYPE_SAMPLER3D,
	VAR_TYPE_SAMPLERCUBE,
	VAR_TYPE_SAMPLERCUBESHADOW,
	VAR_TYPE_SAMPLER2DSHADOW,
	VAR_TYPE_SAMPLER2DARRAY,
	VAR_TYPE_SAMPLER2DARRAYSHADOW,
	//Signed Integer Sampler Types (opaque)
	VAR_TYPE_ISAMPLER2D,
	VAR_TYPE_ISAMPLER3D,
	VAR_TYPE_ISAMPLERCUBE,
	VAR_TYPE_ISAMPLER2DARRAY,
	//Unsigned Integer Sampler Types (opaque)
	VAR_TYPE_USAMPLER2D,
	VAR_TYPE_USAMPLER3D,
	VAR_TYPE_USAMPLERCUBE,
	VAR_TYPE_USAMPLER2DARRAY,
	/* Double types */
	VAR_TYPE_DOUBLE,
	VAR_TYPE_DMAT2,
	VAR_TYPE_DMAT3,
	VAR_TYPE_DMAT4,
	VAR_TYPE_DMAT2X2,
	VAR_TYPE_DMAT2X3,
	VAR_TYPE_DMAT2X4,
	VAR_TYPE_DMAT3X2,
	VAR_TYPE_DMAT3X3,
	VAR_TYPE_DMAT3X4,
	VAR_TYPE_DMAT4X2,
	VAR_TYPE_DMAT4X3,
	VAR_TYPE_DMAT4X4,
} test_var_type;

struct var_descriptor
{
	std::string type;
	std::string precision;
	std::string initializer_with_ones;
	std::string initializer_with_zeroes;
	std::string iterator_initialization;
	std::string iterator_type;
	std::string specific_element;
	std::string variable_type_initializer1;
	std::string variable_type_initializer2;
	std::string coord_param_for_texture_function;
	std::string type_of_result_of_texture_function;
};

// This iterator and map are used to simplify the lookup of type names, initialisation
//  values, etc., associated with each of the types used within the array tests
typedef std::map<glcts::test_var_type, var_descriptor> _supported_variable_types_map;
typedef _supported_variable_types_map::const_iterator _supported_variable_types_map_const_iterator;

/* Groups all tests that verify "arrays of arrays" functionality */
class ArrayOfArraysTestGroup : public glcts::TestCaseGroup
{
public:
	/* Public methods */
	ArrayOfArraysTestGroup(Context& context);

	virtual void init(void);
};

/* Groups all tests that verify "arrays of arrays" functionality */
class ArrayOfArraysTestGroupGL : public glcts::TestCaseGroup
{
public:
	/* Public methods */
	ArrayOfArraysTestGroupGL(Context& context);

	virtual void init(void);
};

namespace ArraysOfArrays
{
namespace Interface
{
/** Represents ES 3.1 core capabilities **/
struct ES
{
	enum
	{
		ALLOW_UNSIZED_DECLARATION = 0
	};
	enum
	{
		ALLOW_A_OF_A_ON_INTERFACE_BLOCKS = 0
	};
	enum
	{
		ALLOW_IN_OUT_INTERFACE_BLOCKS = 0
	};
	enum
	{
		USE_ALL_SHADER_STAGES = 0
	};
	enum
	{
		USE_ATOMIC = 0
	};
	enum
	{
		USE_DOUBLE = 0
	};
	enum
	{
		USE_SUBROUTINE = 0
	};
	enum
	{
		USE_STORAGE_BLOCK = 0
	};

	static const size_t			MAX_ARRAY_DIMENSIONS;
	static const test_var_type* var_types;
	static const size_t			n_var_types;

	static const char* shader_version_gpu5;
	static const char* shader_version;

	static const char* test_group_name;
};

/** Represents GL 4.3 core capabilities **/
struct GL
{
	enum
	{
		ALLOW_UNSIZED_DECLARATION = 1
	};
	enum
	{
		ALLOW_A_OF_A_ON_INTERFACE_BLOCKS = 1
	};
	enum
	{
		ALLOW_IN_OUT_INTERFACE_BLOCKS = 1
	};
	enum
	{
		USE_ALL_SHADER_STAGES = 1
	};
	enum
	{
		USE_ATOMIC = 1
	};
	enum
	{
		USE_DOUBLE = 1
	};
	enum
	{
		USE_SUBROUTINE = 1
	};
	enum
	{
		USE_STORAGE_BLOCK = 1
	};

	static const size_t			MAX_ARRAY_DIMENSIONS;
	static const test_var_type* var_types;
	static const size_t			n_var_types;

	static const char* shader_version_gpu5;
	static const char* shader_version;
};
} /* Interface */

/** Base test class for all arrays_of_arrays tests
 **/
template <class API>
class TestCaseBase : public tcu::TestCase
{
public:
	TestCaseBase(Context& context, const char* name, const char* description);

	virtual ~TestCaseBase(void)
	{
	}

	virtual void						 deinit(void);
	virtual void						 delete_objects(void);
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected declarations */
	enum TestShaderType
	{
		FRAGMENT_SHADER_TYPE,
		VERTEX_SHADER_TYPE,
		COMPUTE_SHADER_TYPE,
		GEOMETRY_SHADER_TYPE,
		TESSELATION_CONTROL_SHADER_TYPE,
		TESSELATION_EVALUATION_SHADER_TYPE,

		/* */
		SHADER_TYPE_LAST
	};

	/* Protected methods */
	virtual std::string extend_string(std::string base_string, std::string sub_script, size_t number_of_elements);

	virtual glw::GLint compile_shader_and_get_compilation_result(
		const std::string& tested_snippet, typename TestCaseBase<API>::TestShaderType tested_shader_type,
		bool require_gpu_shader5 = false);

	virtual tcu::TestNode::IterateResult execute_negative_test(
		typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& shader_source);

	virtual tcu::TestNode::IterateResult execute_positive_test(const std::string& vertex_shader_source,
															   const std::string& fragment_shader_source,
															   bool delete_generated_objects, bool require_gpu_shader5);

	virtual tcu::TestNode::IterateResult execute_positive_test(const std::string& vertex_shader_source,
															   const std::string& tess_ctrl_shader_source,
															   const std::string& tess_eval_shader_source,
															   const std::string& geometry_shader_source,
															   const std::string& fragment_shader_source,
															   const std::string& compute_shader_source,
															   bool delete_generated_objects, bool require_gpu_shader5);

	virtual void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type) = 0;

	/* Protected fields */
	Context&   context_id;
	glw::GLint program_object_id;

private:
	/* Private fields */
	glw::GLint compute_shader_object_id;
	glw::GLint fragment_shader_object_id;
	glw::GLint geometry_shader_object_id;
	glw::GLint tess_ctrl_shader_object_id;
	glw::GLint tess_eval_shader_object_id;
	glw::GLint vertex_shader_object_id;
};

template <class API>
class SizedDeclarationsPrimitive : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsPrimitive(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsPrimitive",
							" Verify that declarations of variables containing between 2 and 8\n"
							" sized dimensions of each primitive type are permitted.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsPrimitive()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsStructTypes1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsStructTypes1(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsStructTypes1",
							" Declare a structure type containing both ints and floats, and verify\n"
							" that variables having between 2 and 8 sized dimensions of this type\n"
							" can be declared.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsStructTypes1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsStructTypes2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsStructTypes2(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsStructTypes2",
							" Verify that a single declaration containing multiple\n"
							" variables with different numbers of array dimensions is accepted,\n"
							" e.g.  float [2][2] x2, x3[2], x4[2][2], etc) with each variable\n"
							" having between two and eight dimensions when there are\n"
							" declarations within the structure body.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsStructTypes2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsStructTypes3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsStructTypes3(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsStructTypes3",
							" Declare a structure type containing both ints and floats, and verify\n"
							" that variables having between 2 and 8 sized dimensions of this type\n"
							" can be declared, with a structure containing an array.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsStructTypes3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsStructTypes4 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsStructTypes4(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsStructTypes4",
							" Declare a structure type containing an array, and verify\n"
							" that variables having between 2 and 8 sized dimensions of this type\n"
							" can be declared when the structure  definition is included in the \n"
							" variable definition.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsStructTypes4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsTypenameStyle1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsTypenameStyle1(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsTypenameStyle1",
							" Verify that an 8-dimensional array of floats can be declared with\n"
							" any placement of the brackets (e.g. float[2]\n"
							" x[2][2][2][2][2][2][2], float [2][2] x [2][2][2][2][2][2], etc) (9\n"
							" cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsTypenameStyle1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsTypenameStyle2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsTypenameStyle2(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsTypenameStyle2",
							" Verify that a single declaration containing multiple\n"
							" variables with different numbers of array dimensions is accepted,\n"
							" e.g.  float [2][2] x2, x3[2], x4[2][2], etc) with each variable\n"
							" having between two and eight dimensions. Repeat these tests for\n"
							" declarations within a structure body.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsTypenameStyle2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsTypenameStyle3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsTypenameStyle3(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsTypenameStyle3",
							" Verify that a single declaration, within a structure body,\n"
							" is accepted when it contains multiple variables with different\n"
							" numbers of array dimensions, \n"
							" e.g.  float [2][2] x2, x3[2], x4[2][2], etc), with each variable\n"
							" having between two and eight dimensions.\n"
							" The variables should be declared within a structure body.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsTypenameStyle3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsTypenameStyle4 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsTypenameStyle4(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsTypenameStyle4",
							" Verify that an 8-dimensional array of floats can be declared with\n"
							" any placement of the brackets (e.g. float[2]\n"
							" x[2][2][2][2][2][2][2], float [2][2] x [2][2][2][2][2][2], etc) (9\n"
							" cases) within a structure body.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsTypenameStyle4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsTypenameStyle5 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsTypenameStyle5(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsTypenameStyle5",
							" Verify that a single declaration containing multiple\n"
							" variables with different numbers of array dimensions is accepted,\n"
							" within a structure body\n"
							" e.g.  float [2][2] x2, x3[2], x4[2][2], etc) with each variable\n"
							" having between two and eight dimensions.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsTypenameStyle5()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SizedDeclarationsFunctionParams : public TestCaseBase<API>
{
public:
	/* Public methods */
	SizedDeclarationsFunctionParams(Context& context)
		: TestCaseBase<API>(context, "SizedDeclarationsFunctionParams",
							" Declare a function having eight parameters, each a float array\n"
							" with a different number of dimensions between 1 and 8, and verify\n"
							" that the compiler accepts this. Declare a variable with matching\n"
							" shape for each parameter, and verify that the function can be\n"
							" called with these variables as arguments. Interchange each pair\n"
							" of arguments and verify that the shader is correctly rejected due\n"
							" to mismatched arguments (28 total cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SizedDeclarationsFunctionParams()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class sized_declarations_invalid_sizes1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	sized_declarations_invalid_sizes1(Context& context)
		: TestCaseBase<API>(context, "sized_declarations_invalid_sizes1",
							" Correctly reject variable declarations, having 4 dimensions, for\n"
							" which any combination of dimensions are declared with zero-size\n"
							" (16 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~sized_declarations_invalid_sizes1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class sized_declarations_invalid_sizes2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	sized_declarations_invalid_sizes2(Context& context)
		: TestCaseBase<API>(context, "sized_declarations_invalid_sizes2",
							" Correctly reject variable declarations, having 4 dimensions, for\n"
							" which any combination of dimensions are declared with size -1\n"
							" (16 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~sized_declarations_invalid_sizes2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class sized_declarations_invalid_sizes3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	sized_declarations_invalid_sizes3(Context& context)
		: TestCaseBase<API>(context, "sized_declarations_invalid_sizes3",
							" Correctly reject variable declarations, having 4 dimensions, for\n"
							" which any combination of dimensions are declared with a\n"
							" non-constant (16 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~sized_declarations_invalid_sizes3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class sized_declarations_invalid_sizes4 : public TestCaseBase<API>
{
public:
	/* Public methods */
	sized_declarations_invalid_sizes4(Context& context)
		: TestCaseBase<API>(context, "sized_declarations_invalid_sizes4",
							" Correctly reject modifications of a variable declaration of\n"
							" 4 dimensions (e.g. float x[2][2][2][2]), in which\n"
							" each adjacent pair '][' is replaced by the sequence operator\n"
							" (e.g. float x[2,2][2][2]) (6 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~sized_declarations_invalid_sizes4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclConstructors1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclConstructors1(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclConstructors1",
							" Verifies that constructors for arrays of between 2 and 8 dimensions\n"
							" are accepted as isolated expressions for each non-opaque primitive\n"
							" type (7 cases per primitive type).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclConstructors1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
	std::string recursively_initialise(std::string var_type, size_t dimension_index, std::string init_string);
};

template <class API>
class ConstructorsAndUnsizedDeclConstructors2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclConstructors2(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclConstructors2",
							" Correctly reject any attempt to pass arguments to a\n"
							" 2 dimensional float array constructor which has matching scalar\n"
							" count, but different array shape or dimensionality to the array's\n"
							" indexed type e.g. float[2][2](float[4](1,2,3,4)),\n"
							" float[2][2](float[1][4](float[4](1,2,3,4))) (2 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclConstructors2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclUnsizedConstructors : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclUnsizedConstructors(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclUnsizedConstructors",
							" Verifies that any of the array dimensions, or any combination of\n"
							" dimensions, may be omitted for a 4-dimensional float array\n"
							" constructor (the sizes on its nested constructors may be\n"
							" consistently omitted or present, and need not be manipulated\n"
							" between cases) (e.g. float[][][2][](float[][][](float[][] etc))\n"
							" (16 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclUnsizedConstructors()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclConst : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclConst(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclConst",
							" Verifies that multi-dimensional arrays can be declared as const\n"
							" using nested constructors to initialize inner dimensions\n"
							" e.g. const float[2][2] x = float[2][2](float[2](1,2),float[2](3,4)).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclConst()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclInvalidConstructors1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclInvalidConstructors1(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclInvalidConstructors1",
							" Correctly reject any attempt to call array constructors for\n"
							" 2-dimensional arrays of any opaque type.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclInvalidConstructors1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclInvalidConstructors2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclInvalidConstructors2(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclInvalidConstructors2",
							" Correctly reject 3-dimensional int array constructor calls\n"
							" for which any dimension or combination of dimensions is\n"
							" given as zero (see sec(i) - 8 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclInvalidConstructors2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclInvalidConstructors3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclInvalidConstructors3(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclInvalidConstructors3",
							" Correctly reject 3-dimensional int array constructor calls\n"
							" for which any dimension or combination of dimensions is\n"
							" given as -1 (see sec(i) - 8 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclInvalidConstructors3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclInvalidConstructors4 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclInvalidConstructors4(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclInvalidConstructors4",
							" Correctly reject 3-dimensional int array constructor calls\n"
							" for which any dimension or combination of dimensions is\n"
							" given by a non-constant variable (8 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclInvalidConstructors4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclConstructorSizing1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclConstructorSizing1(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclConstructorSizing1",
							" Verifies that arrays of 4 dimensions can be declared with any\n"
							" combination of dimension sizes omitted, provided a valid\n"
							" constructor is used as an initializer (15 cases per non-opaque\n"
							" primitive type).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclConstructorSizing1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclConstructorSizing2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclConstructorSizing2(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclConstructorSizing2",
							" Verifies that a sequence of arrays from 2 to 8 dimensions can\n"
							" be declared in a single statement\n"
							" (e.g. float[] x=float[](4,5), y[]=float[][](float[](4)), z[][]...).\n"
							" The size of the dimensions should vary between the cases\n"
							" of this test (e.g. in the previous case, we have\n"
							" float x[2] and float y[1][1]).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclConstructorSizing2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclStructConstructors : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclStructConstructors(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclStructConstructors",
							" Declare a user type (struct) and verify that arrays of between 2\n"
							" and 8 dimensions can be declared without explicit sizes, and\n"
							" initialized from constructors (7 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclStructConstructors()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
	std::string recursively_initialise(std::string var_type, size_t dimension_index, std::string init_string);
};

template <class API>
class ConstructorsAndUnsizedDeclUnsizedArrays1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclUnsizedArrays1(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclUnsizedArrays1",
							" Correctly reject unsized declarations of variables between 2 and 8\n"
							" dimensions for which an initializer is not present.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclUnsizedArrays1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclUnsizedArrays2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclUnsizedArrays2(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclUnsizedArrays2",
							" Correctly reject unsized declarations where some elements are\n"
							" lacking initializers (e.g. float[] x=float[](1), y).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclUnsizedArrays2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclUnsizedArrays3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclUnsizedArrays3(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclUnsizedArrays3",
							" Correctly reject a declaration which initializes a\n"
							" multi-dimensional array from a matrix type,\n"
							" e.g. (float[][] x = mat4(0)).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclUnsizedArrays3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ConstructorsAndUnsizedDeclUnsizedArrays4 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ConstructorsAndUnsizedDeclUnsizedArrays4(Context& context)
		: TestCaseBase<API>(context, "ConstructorsAndUnsizedDeclUnsizedArrays4",
							" Declare a user type containing an unsized array\n"
							" (e.g. struct foo { float[][] x; }) and verify that the shader is\n"
							" correctly rejected.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ConstructorsAndUnsizedDeclUnsizedArrays4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsAssignment1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsAssignment1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsAssignment1",
							" Declare two variables of matching array size, having between 2 and\n"
							" 8 dimensions, and verify that the value of one can be assigned to\n"
							" the other without error (7 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsAssignment1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
	std::string recursively_initialise(std::string var_type, size_t dimension_index, std::string init_string);
};

template <class API>
class ExpressionsAssignment2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsAssignment2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsAssignment2",
							" Correctly reject assignment of variables of differing numbers of array\n"
							" dimensions (between 1 and 4) to one another (6 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsAssignment2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsAssignment3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsAssignment3(Context& context)
		: TestCaseBase<API>(context, "ExpressionsAssignment3",
							" Correctly reject assignment of variables of 4 dimensions and differing\n"
							" array size to one another, where all combinations of each dimension\n"
							" matching or not matching are tested (15 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsAssignment3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsTypeRestrictions1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsTypeRestrictions1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsTypeRestrictions1",
							" Declare two 2-dimensional arrays of a sampler type and verify that\n"
							" one cannot be assigned to the other.\n"
							" Repeat the test for each opaque type.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsTypeRestrictions1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsTypeRestrictions2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsTypeRestrictions2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsTypeRestrictions2",
							" For each opaque type, verify that structures containing \n"
							" two 2-dimensional arrays of that sampler type\n"
							" cannot be assigned to each other.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsTypeRestrictions2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsIndexingScalar1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingScalar1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingScalar1",
							" Assign to each scalar element of a 4 dimensional array\n"
							" float x[1][2][3][4] (24 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingScalar1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsIndexingScalar2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingScalar2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingScalar2",
							" Correctly reject indexing the array with any combination\n"
							" of indices given as -1 (15 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingScalar2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsIndexingScalar3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingScalar3(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingScalar3",
							" Correctly reject indexing the array with any combination\n"
							" of indices given as 4 (15 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingScalar3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsIndexingScalar4 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingScalar4(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingScalar4",
							" Correctly reject any attempt to index a 4-dimensional array with\n"
							" any combination of missing array index expressions\n"
							" (e.g. x[][0][0][]) - (15 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingScalar4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsIndexingArray1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingArray1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingArray1",
							" Assign to each dimension of an 8 dimensional, single-element array\n"
							" with an appropriate constructor (e.g. float\n"
							" x[1][1][1][1][1][1][1][1];\n"
							" x[0] = float[1][1][1][1][1][1][1](1);\n"
							" x[0][0] = etc) - (8 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingArray1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsIndexingArray2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingArray2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingArray2",
							" Declare two 8 dimensional, single-element arrays, and assign to\n"
							" each dimension of one from the matching sub_scripting of the other\n"
							" (e.g. x[0] = y[0]; x[0][0] = y[0][0]; etc.) (8 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingArray2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
	std::string recursively_initialise(std::string var_type, size_t dimension_index, std::string init_string);
};

template <class API>
class ExpressionsIndexingArray3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsIndexingArray3(Context& context)
		: TestCaseBase<API>(context, "ExpressionsIndexingArray3",
							" Correctly reject use of ivecn to index an n-dimensional array -\n"
							" e.g. float x[2][2][2][2]; x[ivec4(0)] = 1; (3 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsIndexingArray3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsDynamicIndexing1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsDynamicIndexing1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsDynamicIndexing1",
							" Verifies that any mixture of constant, uniform and dynamic expressions\n"
							" can be used as the array index expression, in any combination, for\n"
							" each dimension of a 2 dimensional array (16 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsDynamicIndexing1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsDynamicIndexing2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsDynamicIndexing2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsDynamicIndexing2",
							" Correctly reject any attempt to index 4-dimensional arrays of opaque\n"
							" types with any combination of non-constant expressions\n"
							" (e.g. x[0][y][0][y] etc for non-const y) - (15 cases per type).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsDynamicIndexing2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsEquality1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsEquality1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsEquality1",
							" Verifies that two 4-dimensional arrays of matching primitive\n"
							" type can be correctly compared for equality and inequality, when\n"
							" they differ independently in each component or combination of\n"
							" components (e.g. x = float[][](float[](1,1), float[](1,1)); y =\n"
							" float[][](float[](2,1), float[](1,1)); return x == y;) - (16\n"
							" cases per primitive type).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsEquality1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsEquality2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsEquality2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsEquality2",
							" Verifies that two 4-dimensional arrays of matching user (struct)\n"
							" types can be correctly compared for equality and inequality, when\n"
							" they differ independently in each component or combination of\n"
							" components - (16 cases per primitive type).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsEquality2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsLength1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsLength1(Context& context, const glw::GLchar* name, const glw::GLchar* description)
		: TestCaseBase<API>(context, name, description)
	{
		/* Left empty on purpose */
	}

	ExpressionsLength1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsLength1",
							" For a 4-dimensional array declared as int x[4][3][2][1], verify that\n"
							" x.length returns the integer 4, x[0].length the integer 3, and so\n"
							" forth (4 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsLength1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void execute_dispatch_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
							   const std::string& tested_declaration, const std::string& tested_snippet);

	void execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
						   const std::string& tested_declaration, const std::string& tested_snippet);

	std::string prepare_compute_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									   const std::string& tested_declaration, const std::string& tested_snippet);

	std::string prepare_fragment_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& tested_declaration, const std::string& tested_snippet);

	std::string prepare_geometry_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& tested_declaration, const std::string& tested_snippet);

	std::string prepare_tess_ctrl_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& tested_declaration, const std::string& tested_snippet);

	std::string prepare_tess_eval_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& tested_declaration, const std::string& tested_snippet);

	std::string prepare_vertex_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									  const std::string& tested_declaration, const std::string& tested_snippet);

	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsLength2 : public ExpressionsLength1<API>
{
public:
	/* Public methods */
	ExpressionsLength2(Context& context)
		: ExpressionsLength1<API>(context, "ExpressionsLength2",
								  " For a 4-dimensional array declared as int x[1][2][3][4], verify that\n"
								  " x.length returns the integer 1, x[0].length the integer 2, and so\n"
								  " forth (4 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsLength2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsLength3 : public ExpressionsLength1<API>
{
public:
	/* Public methods */
	ExpressionsLength3(Context& context)
		: ExpressionsLength1<API>(context, "ExpressionsLength3",
								  " Correctly reject any use of the length method on elements of a\n"
								  " 4-dimensional array x[1][1][1][1] for which the index\n"
								  " expression is omitted, e.g. x[].length, x[][].length etc (3 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsLength3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsInvalid1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsInvalid1(Context& context)
		: TestCaseBase<API>(context, "ExpressionsInvalid1", " Correctly reject an assignment of a 2 dimensional\n"
															" array x[2][2] to a variable y of type mat2.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsInvalid1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class ExpressionsInvalid2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	ExpressionsInvalid2(Context& context)
		: TestCaseBase<API>(context, "ExpressionsInvalid2",
							" For 8-dimensional arrays x,y, correctly reject any attempt\n"
							" to apply the relational operators other than equality and\n"
							" inequality (4 cases per non-opaque primitive type).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~ExpressionsInvalid2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionFunctionCalls1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionFunctionCalls1(Context& context, const glw::GLchar* name, const glw::GLchar* description)
		: TestCaseBase<API>(context, name, description)
	{
		/* Left empty on purpose */
	}

	InteractionFunctionCalls1(Context& context)
		: TestCaseBase<API>(context, "InteractionFunctionCalls1",
							" Declare a function returning an 8-dimensional 64-element array as\n"
							" an out parameter, which places a unique integer in each\n"
							" element.\n"
							" Verifies that the values are returned as expected when this function\n"
							" is called.\n"
							" Repeat for the following primitive types: int, float,\n"
							" ivec2, ivec3, ivec4, vec2, vec3, vec4, mat2, mat3, mat4.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionFunctionCalls1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void execute_dispatch_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
							   const std::string& function_definition, const std::string& function_use,
							   const std::string& verification);

	void execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
						   const std::string& function_definition, const std::string& function_use,
						   const std::string& verification);

	std::string prepare_compute_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									   const std::string& function_definition, const std::string& function_use,
									   const std::string& verification);

	std::string prepare_fragment_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& function_definition, const std::string& function_use,
										const std::string& verification);

	std::string prepare_geometry_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& function_definition, const std::string& function_use,
										const std::string& verification);

	std::string prepare_tess_ctrl_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& function_definition, const std::string& function_use,
										 const std::string& verification);

	std::string prepare_tess_eval_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& function_definition, const std::string& function_use,
										 const std::string& verification);

	std::string prepare_vertex_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									  const std::string& function_definition, const std::string& function_use,
									  const std::string& verification);

	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionFunctionCalls2 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionFunctionCalls2(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionFunctionCalls2",
										 " Declare a function taking an inout parameter,\n"
										 " which multiplies each element by a different prime.\n"
										 " Verifies that the results after returning are again as expected.\n"
										 " Repeat for the following primitive types: int, float,\n"
										 " ivec2, ivec3, ivec4, vec2, vec3, vec4, mat2, mat3, mat4.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionFunctionCalls2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionArgumentAliasing1 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionArgumentAliasing1(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionArgumentAliasing1",
										 " Declare a function taking two 8-dimensional, 64-element parameters\n"
										 " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
										 " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
										 " and overwriting x with a constant value, the original values of z\n"
										 " are accessible through y.\n"
										 " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionArgumentAliasing1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionArgumentAliasing2 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionArgumentAliasing2(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionArgumentAliasing2",
										 " Declare a function taking two 8-dimensional, 64-element parameters\n"
										 " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
										 " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
										 " and overwriting y with a constant value, the original values of z\n"
										 " are accessible through x.\n"
										 " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionArgumentAliasing2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionArgumentAliasing3 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionArgumentAliasing3(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionArgumentAliasing3",
										 " Declare a function taking two 8-dimensional, 64-element parameters\n"
										 " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
										 " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
										 " and overwriting y with a constant value, the original values of z\n"
										 " are accessible through x, where x is an out parameter.\n"
										 " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionArgumentAliasing3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionArgumentAliasing4 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionArgumentAliasing4(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionArgumentAliasing4",
										 " Declare a function taking two 8-dimensional, 64-element parameters\n"
										 " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
										 " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
										 " and overwriting x with a constant value, the original values of z\n"
										 " are accessible through y, where y is an out parameter.\n"
										 " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionArgumentAliasing4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionArgumentAliasing5 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionArgumentAliasing5(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionArgumentAliasing5",
										 " Declare a function taking two 8-dimensional, 64-element parameters\n"
										 " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
										 " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
										 " and overwriting y with a constant value, the original values of z\n"
										 " are accessible through x, where x is an inout parameter.\n"
										 " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionArgumentAliasing5()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionArgumentAliasing6 : public InteractionFunctionCalls1<API>
{
public:
	/* Public methods */
	InteractionArgumentAliasing6(Context& context)
		: InteractionFunctionCalls1<API>(context, "InteractionArgumentAliasing6",
										 " Declare a function taking two 8-dimensional, 64-element parameters\n"
										 " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
										 " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
										 " and overwriting x with a constant value, the original values of z\n"
										 " are accessible through y, where y is an inout parameter.\n"
										 " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionArgumentAliasing6()
	{
		/* Left empty on purpose */
	}

public:
	//AL protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionUniforms1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionUniforms1(Context& context, const glw::GLchar* name, const glw::GLchar* description)
		: TestCaseBase<API>(context, name, description)
	{
		/* Left empty on purpose */
	}

	InteractionUniforms1(Context& context)
		: TestCaseBase<API>(context, "InteractionUniforms1",
							" Declare a 4-dimensional uniform array and verify that it can be\n"
							" initialized with user data correctly using the API.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionUniforms1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	std::string prepare_compute_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									   const std::string& uniform_definition, const std::string& uniform_use);

	std::string prepare_fragment_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& uniform_definition, const std::string& uniform_use);

	std::string prepare_geometry_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& uniform_definition, const std::string& uniform_use);

	std::string prepare_tess_ctrl_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& uniform_definition, const std::string& uniform_use);

	std::string prepare_tess_eval_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& uniform_definition, const std::string& uniform_use);

	std::string prepare_vertex_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									  const std::string& uniform_definition, const std::string& uniform_use);

	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionUniforms2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionUniforms2(Context& context)
		: TestCaseBase<API>(context, "InteractionUniforms2",
							" Correctly reject 4-dimensional uniform arrays with any unsized\n"
							" dimension, with or without an initializer (30 cases).\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionUniforms2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionUniformBuffers1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionUniformBuffers1(Context& context)
		: TestCaseBase<API>(context, "InteractionUniformBuffers1",
							" Declare a uniform block containing a 6-dimensional array and verify\n"
							" that the resulting shader compiles.\n"
							" Repeat for ints and uints.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionUniformBuffers1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionUniformBuffers2 : public InteractionUniforms1<API>
{
public:
	/* Public methods */
	InteractionUniformBuffers2(Context& context)
		: InteractionUniforms1<API>(context, "InteractionUniformBuffers2",
									" Declare a 4-dimensional uniform float array x[2][2][2][2] within a\n"
									" uniform block, and verify that it can be initialized correctly with user\n"
									" data via the API.\n"
									" Repeat for ints and uints.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionUniformBuffers2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void execute_dispatch_test();

	void execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type);

	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionUniformBuffers3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionUniformBuffers3(Context& context)
		: TestCaseBase<API>(context, "InteractionUniformBuffers3",
							" Correctly reject 4-dimensional uniform arrays with a uniform block\n"
							" with any dimension unsized, with or without an initializer (30 cases).\n"
							" Repeat for ints and uints.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionUniformBuffers3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionStorageBuffers1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionStorageBuffers1(Context& context)
		: TestCaseBase<API>(context, "InteractionStorageBuffers1",
							" Declare a storage block containing a 6-dimensional array and verify\n"
							" that the resulting shader compiles.\n"
							" Repeat for ints and uints.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionStorageBuffers1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionStorageBuffers2 : public InteractionUniforms1<API>
{
public:
	/* Public methods */
	InteractionStorageBuffers2(Context& context)
		: InteractionUniforms1<API>(context, "InteractionStorageBuffers2",
									" Declare a 4-dimensional float array x[2][2][2][2] within a\n"
									" storage block, and verify that it can be initialized correctly with user\n"
									" data via the API.\n"
									" Repeat for ints and uints.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionStorageBuffers2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void execute_dispatch_test();

	void execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type);

	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionStorageBuffers3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionStorageBuffers3(Context& context)
		: TestCaseBase<API>(context, "InteractionStorageBuffers3",
							" Correctly reject 4-dimensional uniform arrays with a uniform block\n"
							" with any dimension unsized, with or without an initializer (30 cases).\n"
							" Repeat for ints and uints.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionStorageBuffers3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionInterfaceArrays1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionInterfaceArrays1(Context& context)
		: TestCaseBase<API>(context, "InteractionInterfaceArrays1", " Verifies that 2-dimensional arrays of shader\n"
																	" storage buffer objects are correctly rejected.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionInterfaceArrays1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionInterfaceArrays2 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionInterfaceArrays2(Context& context, const glw::GLchar* name, const glw::GLchar* description)
		: TestCaseBase<API>(context, name, description)
	{
		/* Left empty on purpose */
	}

	InteractionInterfaceArrays2(Context& context)
		: TestCaseBase<API>(context, "InteractionInterfaceArrays2",
							" Verifies that 2-dimensional arrays of input and output variables\n"
							" are correctly rejected.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionInterfaceArrays2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	const typename TestCaseBase<API>::TestShaderType get_output_shader_type(
		const typename TestCaseBase<API>::TestShaderType& input_shader_type);
	const std::string prepare_fragment_shader(const typename TestCaseBase<API>::TestShaderType& input_shader_type,
											  const std::string& input_source, const std::string& output_source);
	const std::string prepare_geometry_shader(const typename TestCaseBase<API>::TestShaderType& input_shader_type,
											  const std::string& input_source, const std::string& output_source);
	const std::string prepare_tess_ctrl_shader_source(
		const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
		const std::string& output_source);
	const std::string prepare_tess_eval_shader_source(
		const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
		const std::string& output_source);
	const std::string prepare_vertex_shader(const typename TestCaseBase<API>::TestShaderType& input_shader_type,
											const std::string& input_source, const std::string& output_source);
	void prepare_sources(const typename TestCaseBase<API>::TestShaderType& input_shader_type,
						 const typename TestCaseBase<API>::TestShaderType& output_shader_type,
						 const std::string* input_shader_source, const std::string* output_shader_source,
						 std::string& input_source, std::string& output_source);
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType input_shader_type);
};

template <class API>
class InteractionInterfaceArrays3 : public TestCaseBase<API>
{
public:
	/* Public methods */
	InteractionInterfaceArrays3(Context& context)
		: TestCaseBase<API>(context, "InteractionInterfaceArrays3",
							" Verifies that 2-dimensional arrays of uniform interface blocks\n"
							" are correctly rejected.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionInterfaceArrays3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class InteractionInterfaceArrays4 : public InteractionInterfaceArrays2<API>
{
public:
	/* Public methods */
	InteractionInterfaceArrays4(Context& context)
		: InteractionInterfaceArrays2<API>(context, "InteractionInterfaceArrays4",
										   " Verifies that 2-dimensional arrays of input and output interface blocks\n"
										   " are correctly rejected.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~InteractionInterfaceArrays4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType input_shader_type);
};

/** Implements test AtomicDeclaration, desription follows:
 *
 * Prepare a shader that declares "bar", a multidimensional array of atomic
 * counters. Shader should increment all entries of "bar". It is expected that
 * building of program will:
 * - pass when number of entries in "bar" is equal to a value of the
 * corresponding MAX_*_ATOMIC_COUNTERS constant,
 * - fail when number of entries in "bar" exceeds limit.
 * Test all supported shader stages separately.
 **/
template <class API>
class AtomicDeclarationTest : public TestCaseBase<API>
{
public:
	/* Public methods */
	AtomicDeclarationTest(Context& context)
		: TestCaseBase<API>(context, "AtomicDeclaration",
							" Verifies that atomic counters can be grouped in multidimensional arrays\n")
	{
		/* Left empty on purpose */
	}

	virtual ~AtomicDeclarationTest()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

/** Implements test AtomicUsage, desription follows:
 *
 * Prepare a program like in "valid" case of AtomicDeclaration test, but use
 * layout qualifier with last binding and specific offset. Bind a buffer so all
 * entries in "bar" have unique values assigned. Select offset and size of
 * "bar" as follows:
 * - offset is 0, "bar" fills entire allowed space;
 * - offset is not 0, "bar" does not exceed any limit.
 * Test pass if buffer contents are correctly modified by execution of the
 * program.
 * Test all supported shader stages separately.
 **/
template <class API>
class AtomicUsageTest : public TestCaseBase<API>
{
public:
	/* Public methods */
	AtomicUsageTest(Context& context)
		: TestCaseBase<API>(context, "AtomicUsage",
							" Verifies that atomic counters grouped in multidimensional arrays can be used\n")
	{
		/* Left empty on purpose */
	}

	virtual ~AtomicUsageTest()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);

private:
	void execute(typename TestCaseBase<API>::TestShaderType tested_shader_type, glw::GLuint binding, glw::GLuint offset,
				 glw::GLuint n_entries);
};

/** Implements "out" case of SubRoutineCalls test, description follows:
 *
 * Modify "function-calls" test in the following aspects:
 * - provide two subroutines instead of each function;
 * - "new" "out" subroutine should use different pattern of values;
 * - "new" "inout" subroutine should use division instead of multiplication;
 * - it is expected that "original" set will pass, while "new" set will fail.
 **/
template <class API>
class SubroutineFunctionCalls1 : public TestCaseBase<API>
{
public:
	/* Public methods */
	SubroutineFunctionCalls1(Context& context, const glw::GLchar* name, const glw::GLchar* description)
		: TestCaseBase<API>(context, name, description)
	{
		/* Left empty on purpose */
	}

	SubroutineFunctionCalls1(Context& context)
		: TestCaseBase<API>(context, "SubroutineFunctionCalls1",
							" Declare two subroutines returning an 8-dimensional 64-element array as\n"
							" an out parameter, filled with unique values.\n"
							" Verifies that the values are returned as expected when this function\n"
							" is called.\n"
							" Repeat for the following primitive types: int, float,\n"
							" ivec2, ivec3, ivec4, vec2, vec3, vec4, mat2, mat3, mat4.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SubroutineFunctionCalls1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void execute_dispatch_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
							   const std::string& function_definition, const std::string& function_use,
							   const std::string& verification, bool use_original, bool expect_invalid_result);

	void execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
						   const std::string& function_definition, const std::string& function_use,
						   const std::string& verification, bool use_original, bool expect_invalid_result);

	std::string prepare_compute_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									   const std::string& function_definition, const std::string& function_use,
									   const std::string& verification);

	std::string prepare_fragment_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& function_definition, const std::string& function_use,
										const std::string& verification);

	std::string prepare_geometry_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										const std::string& function_definition, const std::string& function_use,
										const std::string& verification);

	std::string prepare_tess_ctrl_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& function_definition, const std::string& function_use,
										 const std::string& verification);

	std::string prepare_tess_eval_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
										 const std::string& function_definition, const std::string& function_use,
										 const std::string& verification);

	std::string prepare_vertex_shader(typename TestCaseBase<API>::TestShaderType tested_shader_type,
									  const std::string& function_definition, const std::string& function_use,
									  const std::string& verification);

	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

/** Implements "inout" case of SubRoutineCalls test, description follows:
 *
 * Modify "function-calls" test in the following aspects:
 * - provide two subroutines instead of each function;
 * - "new" "out" subroutine should use different pattern of values;
 * - "new" "inout" subroutine should use division instead of multiplication;
 * - it is expected that "original" set will pass, while "new" set will fail.
 **/
template <class API>
class SubroutineFunctionCalls2 : public SubroutineFunctionCalls1<API>
{
public:
	/* Public methods */
	SubroutineFunctionCalls2(Context& context)
		: SubroutineFunctionCalls1<API>(context, "SubroutineFunctionCalls2",
										" Declare two subroutines taking an inout parameter,\n"
										" which modifies each element in a unique way.\n"
										" Verifies that the results after returning are as expected.\n"
										" Repeat for the following primitive types: int, float,\n"
										" ivec2, ivec3, ivec4, vec2, vec3, vec4, mat2, mat3, mat4.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SubroutineFunctionCalls2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SubroutineArgumentAliasing1 : public SubroutineFunctionCalls1<API>
{
public:
	/* Public methods */
	SubroutineArgumentAliasing1(Context& context)
		: SubroutineFunctionCalls1<API>(
			  context, "SubroutineArgumentAliasing1",
			  " Declare a function taking two 8-dimensional, 64-element parameters\n"
			  " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
			  " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
			  " and overwriting one parameter with a constant value, the original values of z\n"
			  " are accessible through the second parameter.\n"
			  " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SubroutineArgumentAliasing1()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SubroutineArgumentAliasing2 : public SubroutineFunctionCalls1<API>
{
public:
	/* Public methods */
	SubroutineArgumentAliasing2(Context& context)
		: SubroutineFunctionCalls1<API>(
			  context, "SubroutineArgumentAliasing2",
			  " Declare two subroutines taking two 8-dimensional, 64-element parameters\n"
			  " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
			  " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
			  " and overwriting one parameter with a constant value, the original values of z\n"
			  " are accessible through the second inout parameter.\n"
			  " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SubroutineArgumentAliasing2()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SubroutineArgumentAliasing3 : public SubroutineFunctionCalls1<API>
{
public:
	/* Public methods */
	SubroutineArgumentAliasing3(Context& context)
		: SubroutineFunctionCalls1<API>(
			  context, "SubroutineArgumentAliasing3",
			  " Declare two subroutines taking two 8-dimensional, 64-element parameters\n"
			  " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
			  " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
			  " and overwriting x, out, parameter with a constant value, the original values of z\n"
			  " are accessible through the y parameter.\n"
			  " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SubroutineArgumentAliasing3()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

template <class API>
class SubroutineArgumentAliasing4 : public SubroutineFunctionCalls1<API>
{
public:
	/* Public methods */
	SubroutineArgumentAliasing4(Context& context)
		: SubroutineFunctionCalls1<API>(
			  context, "SubroutineArgumentAliasing4",
			  " Declare two subroutines taking two 8-dimensional, 64-element parameters\n"
			  " (e.g. void g(int x[2][2][2][2][2][2][2][2], int\n"
			  " y[2][2][2][2][2][2][2][2]) and verify that after calling g(z,z),\n"
			  " and overwriting y, out, parameter with a constant value, the original values of z\n"
			  " are accessible through the x parameter.\n"
			  " Repeat for float and mat4 types.\n")
	{
		/* Left empty on purpose */
	}

	virtual ~SubroutineArgumentAliasing4()
	{
		/* Left empty on purpose */
	}

protected:
	/* Protected methods */
	void test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type);
};

} /* ArraysOfArrays */
} /* glcts */

#endif // _ES31CARRAYOFARRAYSTESTS_HPP
