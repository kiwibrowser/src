/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Shader derivate function tests.
 *
 * \todo [2013-06-25 pyry] Missing features:
 *  - lines and points
 *  - projected coordinates
 *  - continous non-trivial functions (sin, exp)
 *  - non-continous functions (step)
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderDerivateTests.hpp"
#include "vktShaderRender.hpp"
#include "vkImageUtil.hpp"

#include "gluTextureUtil.hpp"

#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuRGBA.hpp"
#include "tcuFloat.hpp"
#include "tcuInterval.hpp"

#include "deUniquePtr.hpp"
#include "glwEnums.hpp"

#include <sstream>
#include <string>

namespace vkt
{
namespace sr
{
namespace
{

using namespace vk;

using std::vector;
using std::string;
using std::map;
using tcu::TestLog;
using std::ostringstream;

enum
{
	VIEWPORT_WIDTH			= 99,
	VIEWPORT_HEIGHT			= 133,
	MAX_FAILED_MESSAGES		= 10
};

enum DerivateFunc
{
	DERIVATE_DFDX			= 0,
	DERIVATE_DFDXFINE,
	DERIVATE_DFDXCOARSE,

	DERIVATE_DFDY,
	DERIVATE_DFDYFINE,
	DERIVATE_DFDYCOARSE,

	DERIVATE_FWIDTH,
	DERIVATE_FWIDTHFINE,
	DERIVATE_FWIDTHCOARSE,

	DERIVATE_LAST
};

enum SurfaceType
{
	SURFACETYPE_UNORM_FBO	= 0,
	SURFACETYPE_FLOAT_FBO,	// \note Uses RGBA32UI fbo actually, since FP rendertargets are not in core spec.

	SURFACETYPE_LAST
};

// Utilities

static const char* getDerivateFuncName (DerivateFunc func)
{
	switch (func)
	{
		case DERIVATE_DFDX:				return "dFdx";
		case DERIVATE_DFDXFINE:			return "dFdxFine";
		case DERIVATE_DFDXCOARSE:		return "dFdxCoarse";
		case DERIVATE_DFDY:				return "dFdy";
		case DERIVATE_DFDYFINE:			return "dFdyFine";
		case DERIVATE_DFDYCOARSE:		return "dFdyCoarse";
		case DERIVATE_FWIDTH:			return "fwidth";
		case DERIVATE_FWIDTHFINE:		return "fwidthFine";
		case DERIVATE_FWIDTHCOARSE:		return "fwidthCoarse";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static const char* getDerivateFuncCaseName (DerivateFunc func)
{
	switch (func)
	{
		case DERIVATE_DFDX:				return "dfdx";
		case DERIVATE_DFDXFINE:			return "dfdxfine";
		case DERIVATE_DFDXCOARSE:		return "dfdxcoarse";
		case DERIVATE_DFDY:				return "dfdy";
		case DERIVATE_DFDYFINE:			return "dfdyfine";
		case DERIVATE_DFDYCOARSE:		return "dfdycoarse";
		case DERIVATE_FWIDTH:			return "fwidth";
		case DERIVATE_FWIDTHFINE:		return "fwidthfine";
		case DERIVATE_FWIDTHCOARSE:		return "fwidthcoarse";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static inline bool isDfdxFunc (DerivateFunc func)
{
	return func == DERIVATE_DFDX || func == DERIVATE_DFDXFINE || func == DERIVATE_DFDXCOARSE;
}

static inline bool isDfdyFunc (DerivateFunc func)
{
	return func == DERIVATE_DFDY || func == DERIVATE_DFDYFINE || func == DERIVATE_DFDYCOARSE;
}

static inline bool isFwidthFunc (DerivateFunc func)
{
	return func == DERIVATE_FWIDTH || func == DERIVATE_FWIDTHFINE || func == DERIVATE_FWIDTHCOARSE;
}

static inline tcu::BVec4 getDerivateMask (glu::DataType type)
{
	switch (type)
	{
		case glu::TYPE_FLOAT:		return tcu::BVec4(true, false, false, false);
		case glu::TYPE_FLOAT_VEC2:	return tcu::BVec4(true, true, false, false);
		case glu::TYPE_FLOAT_VEC3:	return tcu::BVec4(true, true, true, false);
		case glu::TYPE_FLOAT_VEC4:	return tcu::BVec4(true, true, true, true);
		default:
			DE_ASSERT(false);
			return tcu::BVec4(true);
	}
}

static inline tcu::Vec4 readDerivate (const tcu::ConstPixelBufferAccess& surface, const tcu::Vec4& derivScale, const tcu::Vec4& derivBias, int x, int y)
{
	return (surface.getPixel(x, y) - derivBias) / derivScale;
}

static inline tcu::UVec4 getCompExpBits (const tcu::Vec4& v)
{
	return tcu::UVec4(tcu::Float32(v[0]).exponentBits(),
					  tcu::Float32(v[1]).exponentBits(),
					  tcu::Float32(v[2]).exponentBits(),
					  tcu::Float32(v[3]).exponentBits());
}

float computeFloatingPointError (const float value, const int numAccurateBits)
{
	const int		numGarbageBits	= 23-numAccurateBits;
	const deUint32	mask			= (1u<<numGarbageBits)-1u;
	const int		exp				= tcu::Float32(value).exponent();

	return tcu::Float32::construct(+1, exp, (1u<<23) | mask).asFloat() - tcu::Float32::construct(+1, exp, 1u<<23).asFloat();
}

static int getNumMantissaBits (const glu::Precision precision)
{
	switch (precision)
	{
		case glu::PRECISION_HIGHP:		return 23;
		case glu::PRECISION_MEDIUMP:	return 10;
		case glu::PRECISION_LOWP:		return 6;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

static int getMinExponent (const glu::Precision precision)
{
	switch (precision)
	{
		case glu::PRECISION_HIGHP:		return -126;
		case glu::PRECISION_MEDIUMP:	return -14;
		case glu::PRECISION_LOWP:		return -8;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

static float getSingleULPForExponent (int exp, int numMantissaBits)
{
	if (numMantissaBits > 0)
	{
		DE_ASSERT(numMantissaBits <= 23);

		const int ulpBitNdx = 23-numMantissaBits;
		return tcu::Float32::construct(+1, exp, (1<<23) | (1 << ulpBitNdx)).asFloat() - tcu::Float32::construct(+1, exp, (1<<23)).asFloat();
	}
	else
	{
		DE_ASSERT(numMantissaBits == 0);
		return tcu::Float32::construct(+1, exp, (1<<23)).asFloat();
	}
}

static float getSingleULPForValue (float value, int numMantissaBits)
{
	const int exp = tcu::Float32(value).exponent();
	return getSingleULPForExponent(exp, numMantissaBits);
}

static float convertFloatFlushToZeroRtn (float value, int minExponent, int numAccurateBits)
{
	if (value == 0.0f)
	{
		return 0.0f;
	}
	else
	{
		const tcu::Float32	inputFloat			= tcu::Float32(value);
		const int			numTruncatedBits	= 23-numAccurateBits;
		const deUint32		truncMask			= (1u<<numTruncatedBits)-1u;

		if (value > 0.0f)
		{
			if (value > 0.0f && tcu::Float32(value).exponent() < minExponent)
			{
				// flush to zero if possible
				return 0.0f;
			}
			else
			{
				// just mask away non-representable bits
				return tcu::Float32::construct(+1, inputFloat.exponent(), inputFloat.mantissa() & ~truncMask).asFloat();
			}
		}
		else
		{
			if (inputFloat.mantissa() & truncMask)
			{
				// decrement one ulp if truncated bits are non-zero (i.e. if value is not representable)
				return tcu::Float32::construct(-1, inputFloat.exponent(), inputFloat.mantissa() & ~truncMask).asFloat() - getSingleULPForExponent(inputFloat.exponent(), numAccurateBits);
			}
			else
			{
				// value is representable, no need to do anything
				return value;
			}
		}
	}
}

static float convertFloatFlushToZeroRtp (float value, int minExponent, int numAccurateBits)
{
	return -convertFloatFlushToZeroRtn(-value, minExponent, numAccurateBits);
}

static float addErrorUlp (float value, float numUlps, int numMantissaBits)
{
	return value + numUlps * getSingleULPForValue(value, numMantissaBits);
}

enum
{
	INTERPOLATION_LOST_BITS = 3, // number mantissa of bits allowed to be lost in varying interpolation
};

static inline tcu::Vec4 getDerivateThreshold (const glu::Precision precision, const tcu::Vec4& valueMin, const tcu::Vec4& valueMax, const tcu::Vec4& expectedDerivate)
{
	const int			baseBits		= getNumMantissaBits(precision);
	const tcu::UVec4	derivExp		= getCompExpBits(expectedDerivate);
	const tcu::UVec4	maxValueExp		= max(getCompExpBits(valueMin), getCompExpBits(valueMax));
	const tcu::UVec4	numBitsLost		= maxValueExp - min(maxValueExp, derivExp);
	const tcu::IVec4	numAccurateBits	= max(baseBits - numBitsLost.asInt() - (int)INTERPOLATION_LOST_BITS, tcu::IVec4(0));

	return tcu::Vec4(computeFloatingPointError(expectedDerivate[0], numAccurateBits[0]),
					 computeFloatingPointError(expectedDerivate[1], numAccurateBits[1]),
					 computeFloatingPointError(expectedDerivate[2], numAccurateBits[2]),
					 computeFloatingPointError(expectedDerivate[3], numAccurateBits[3]));
}

struct LogVecComps
{
	const tcu::Vec4&	v;
	int					numComps;

	LogVecComps (const tcu::Vec4& v_, int numComps_)
		: v			(v_)
		, numComps	(numComps_)
	{
	}
};

std::ostream& operator<< (std::ostream& str, const LogVecComps& v)
{
	DE_ASSERT(de::inRange(v.numComps, 1, 4));
	if (v.numComps == 1)		return str << v.v[0];
	else if (v.numComps == 2)	return str << v.v.toWidth<2>();
	else if (v.numComps == 3)	return str << v.v.toWidth<3>();
	else						return str << v.v;
}

enum VerificationLogging
{
	LOG_ALL = 0,
	LOG_NOTHING
};

static bool verifyConstantDerivate (tcu::TestLog&						log,
									const tcu::ConstPixelBufferAccess&	result,
									const tcu::PixelBufferAccess&		errorMask,
									glu::DataType						dataType,
									const tcu::Vec4&					reference,
									const tcu::Vec4&					threshold,
									const tcu::Vec4&					scale,
									const tcu::Vec4&					bias,
									VerificationLogging					logPolicy = LOG_ALL)
{
	const int			numComps		= glu::getDataTypeFloatScalars(dataType);
	const tcu::BVec4	mask			= tcu::logicalNot(getDerivateMask(dataType));
	int					numFailedPixels	= 0;

	if (logPolicy == LOG_ALL)
		log << TestLog::Message << "Expecting " << LogVecComps(reference, numComps) << " with threshold " << LogVecComps(threshold, numComps) << TestLog::EndMessage;

	for (int y = 0; y < result.getHeight(); y++)
	{
		for (int x = 0; x < result.getWidth(); x++)
		{
			const tcu::Vec4		resDerivate		= readDerivate(result, scale, bias, x, y);
			const bool			isOk			= tcu::allEqual(tcu::logicalOr(tcu::lessThanEqual(tcu::abs(reference - resDerivate), threshold), mask), tcu::BVec4(true));

			if (!isOk)
			{
				if (numFailedPixels < MAX_FAILED_MESSAGES && logPolicy == LOG_ALL)
					log << TestLog::Message << "FAIL: got " << LogVecComps(resDerivate, numComps)
											<< ", diff = " << LogVecComps(tcu::abs(reference - resDerivate), numComps)
											<< ", at x = " << x << ", y = " << y
						<< TestLog::EndMessage;
				numFailedPixels += 1;
				errorMask.setPixel(tcu::RGBA::red().toVec(), x, y);
			}
		}
	}

	if (numFailedPixels >= MAX_FAILED_MESSAGES && logPolicy == LOG_ALL)
		log << TestLog::Message << "..." << TestLog::EndMessage;

	if (numFailedPixels > 0 && logPolicy == LOG_ALL)
		log << TestLog::Message << "FAIL: found " << numFailedPixels << " failed pixels" << TestLog::EndMessage;

	return numFailedPixels == 0;
}

struct Linear2DFunctionEvaluator
{
	tcu::Matrix<float, 4, 3> matrix;

	//      .-----.
	//      | s_x |
	//  M x | s_y |
	//      | 1.0 |
	//      '-----'
	tcu::Vec4 evaluateAt (float screenX, float screenY) const;
};

tcu::Vec4 Linear2DFunctionEvaluator::evaluateAt (float screenX, float screenY) const
{
	const tcu::Vec3 position(screenX, screenY, 1.0f);
	return matrix * position;
}

static bool reverifyConstantDerivateWithFlushRelaxations (tcu::TestLog&							log,
														  const tcu::ConstPixelBufferAccess&	result,
														  const tcu::PixelBufferAccess&			errorMask,
														  glu::DataType							dataType,
														  glu::Precision						precision,
														  const tcu::Vec4&						derivScale,
														  const tcu::Vec4&						derivBias,
														  const tcu::Vec4&						surfaceThreshold,
														  DerivateFunc							derivateFunc,
														  const Linear2DFunctionEvaluator&		function)
{
	DE_ASSERT(result.getWidth() == errorMask.getWidth());
	DE_ASSERT(result.getHeight() == errorMask.getHeight());
	DE_ASSERT(isDfdxFunc(derivateFunc) || isDfdyFunc(derivateFunc));

	const tcu::IVec4	red						(255, 0, 0, 255);
	const tcu::IVec4	green					(0, 255, 0, 255);
	const float			divisionErrorUlps		= 2.5f;

	const int			numComponents			= glu::getDataTypeFloatScalars(dataType);
	const int			numBits					= getNumMantissaBits(precision);
	const int			minExponent				= getMinExponent(precision);

	const int			numVaryingSampleBits	= numBits - INTERPOLATION_LOST_BITS;
	int					numFailedPixels			= 0;

	tcu::clear(errorMask, green);

	// search for failed pixels
	for (int y = 0; y < result.getHeight(); ++y)
	for (int x = 0; x < result.getWidth(); ++x)
	{
		//                 flushToZero?(f2z?(functionValueCurrent) - f2z?(functionValueBefore))
		// flushToZero? ( ------------------------------------------------------------------------ +- 2.5 ULP )
		//                                                  dx

		const tcu::Vec4	resultDerivative		= readDerivate(result, derivScale, derivBias, x, y);

		// sample at the front of the back pixel and the back of the front pixel to cover the whole area of
		// legal sample positions. In general case this is NOT OK, but we know that the target funtion is
		// (mostly*) linear which allows us to take the sample points at arbitrary points. This gets us the
		// maximum difference possible in exponents which are used in error bound calculations.
		// * non-linearity may happen around zero or with very high function values due to subnorms not
		//   behaving well.
		const tcu::Vec4	functionValueForward	= (isDfdxFunc(derivateFunc))
													? (function.evaluateAt((float)x + 2.0f, (float)y + 0.5f))
													: (function.evaluateAt((float)x + 0.5f, (float)y + 2.0f));
		const tcu::Vec4	functionValueBackward	= (isDfdyFunc(derivateFunc))
													? (function.evaluateAt((float)x - 1.0f, (float)y + 0.5f))
													: (function.evaluateAt((float)x + 0.5f, (float)y - 1.0f));

		bool	anyComponentFailed				= false;

		// check components separately
		for (int c = 0; c < numComponents; ++c)
		{
			// Simulate interpolation. Add allowed interpolation error and round to target precision. Allow one half ULP (i.e. correct rounding)
			const tcu::Interval	forwardComponent		(convertFloatFlushToZeroRtn(addErrorUlp((float)functionValueForward[c],  -0.5f, numVaryingSampleBits), minExponent, numBits),
														 convertFloatFlushToZeroRtp(addErrorUlp((float)functionValueForward[c],  +0.5f, numVaryingSampleBits), minExponent, numBits));
			const tcu::Interval	backwardComponent		(convertFloatFlushToZeroRtn(addErrorUlp((float)functionValueBackward[c], -0.5f, numVaryingSampleBits), minExponent, numBits),
														 convertFloatFlushToZeroRtp(addErrorUlp((float)functionValueBackward[c], +0.5f, numVaryingSampleBits), minExponent, numBits));
			const int			maxValueExp				= de::max(de::max(tcu::Float32(forwardComponent.lo()).exponent(),   tcu::Float32(forwardComponent.hi()).exponent()),
																  de::max(tcu::Float32(backwardComponent.lo()).exponent(),  tcu::Float32(backwardComponent.hi()).exponent()));

			// subtraction in numerator will likely cause a cancellation of the most
			// significant bits. Apply error bounds.

			const tcu::Interval	numerator				(forwardComponent - backwardComponent);
			const int			numeratorLoExp			= tcu::Float32(numerator.lo()).exponent();
			const int			numeratorHiExp			= tcu::Float32(numerator.hi()).exponent();
			const int			numeratorLoBitsLost		= de::max(0, maxValueExp - numeratorLoExp); //!< must clamp to zero since if forward and backward components have different
			const int			numeratorHiBitsLost		= de::max(0, maxValueExp - numeratorHiExp); //!< sign, numerator might have larger exponent than its operands.
			const int			numeratorLoBits			= de::max(0, numBits - numeratorLoBitsLost);
			const int			numeratorHiBits			= de::max(0, numBits - numeratorHiBitsLost);

			const tcu::Interval	numeratorRange			(convertFloatFlushToZeroRtn((float)numerator.lo(), minExponent, numeratorLoBits),
														 convertFloatFlushToZeroRtp((float)numerator.hi(), minExponent, numeratorHiBits));

			const tcu::Interval	divisionRange			= numeratorRange / 3.0f; // legal sample area is anywhere within this and neighboring pixels (i.e. size = 3)
			const tcu::Interval	divisionResultRange		(convertFloatFlushToZeroRtn(addErrorUlp((float)divisionRange.lo(), -divisionErrorUlps, numBits), minExponent, numBits),
														 convertFloatFlushToZeroRtp(addErrorUlp((float)divisionRange.hi(), +divisionErrorUlps, numBits), minExponent, numBits));
			const tcu::Interval	finalResultRange		(divisionResultRange.lo() - surfaceThreshold[c], divisionResultRange.hi() + surfaceThreshold[c]);

			if (resultDerivative[c] >= finalResultRange.lo() && resultDerivative[c] <= finalResultRange.hi())
			{
				// value ok
			}
			else
			{
				if (numFailedPixels < MAX_FAILED_MESSAGES)
					log << tcu::TestLog::Message
						<< "Error in pixel at " << x << ", " << y << " with component " << c << " (channel " << ("rgba"[c]) << ")\n"
						<< "\tGot pixel value " << result.getPixelInt(x, y) << "\n"
						<< "\t\tdFd" << ((isDfdxFunc(derivateFunc)) ? ('x') : ('y')) << " ~= " << resultDerivative[c] << "\n"
						<< "\t\tdifference to a valid range: "
							<< ((resultDerivative[c] < finalResultRange.lo()) ? ("-") : ("+"))
							<< ((resultDerivative[c] < finalResultRange.lo()) ? (finalResultRange.lo() - resultDerivative[c]) : (resultDerivative[c] - finalResultRange.hi()))
							<< "\n"
						<< "\tDerivative value range:\n"
						<< "\t\tMin: " << finalResultRange.lo() << "\n"
						<< "\t\tMax: " << finalResultRange.hi() << "\n"
						<< tcu::TestLog::EndMessage;

				++numFailedPixels;
				anyComponentFailed = true;
			}
		}

		if (anyComponentFailed)
			errorMask.setPixel(red, x, y);
	}

	if (numFailedPixels >= MAX_FAILED_MESSAGES)
		log << TestLog::Message << "..." << TestLog::EndMessage;

	if (numFailedPixels > 0)
		log << TestLog::Message << "FAIL: found " << numFailedPixels << " failed pixels" << TestLog::EndMessage;

	return numFailedPixels == 0;
}

// TestCase utils

struct DerivateCaseDefinition
{
	DerivateCaseDefinition (void)
	{
		func				= DERIVATE_LAST;
		dataType			= glu::TYPE_LAST;
		precision			= glu::PRECISION_LAST;
		coordDataType		= glu::TYPE_LAST;
		coordPrecision		= glu::PRECISION_LAST;
		surfaceType			= SURFACETYPE_UNORM_FBO;
		numSamples			= 0;
	}

	DerivateFunc			func;
	glu::DataType			dataType;
	glu::Precision			precision;

	glu::DataType			coordDataType;
	glu::Precision			coordPrecision;

	SurfaceType				surfaceType;
	int						numSamples;
};

struct DerivateCaseValues
{
	tcu::Vec4	coordMin;
	tcu::Vec4	coordMax;
	tcu::Vec4	derivScale;
	tcu::Vec4	derivBias;
};

struct TextureCaseValues
{
	tcu::Vec4	texValueMin;
	tcu::Vec4	texValueMax;
};

class DerivateUniformSetup : public UniformSetup
{
public:
						DerivateUniformSetup		(bool useSampler);
	virtual				~DerivateUniformSetup		(void);

	virtual void		setup						(ShaderRenderCaseInstance& instance, const tcu::Vec4&) const;

private:
	const bool			m_useSampler;
};

DerivateUniformSetup::DerivateUniformSetup (bool useSampler)
	: m_useSampler(useSampler)
{
}

DerivateUniformSetup::~DerivateUniformSetup (void)
{
}

// TriangleDerivateCaseInstance

class TriangleDerivateCaseInstance : public ShaderRenderCaseInstance
{
public:
									TriangleDerivateCaseInstance	(Context&						context,
																	 const UniformSetup&			uniformSetup,
																	 const DerivateCaseDefinition&	definitions,
																	 const DerivateCaseValues&		values);
	virtual							~TriangleDerivateCaseInstance	(void);
	virtual tcu::TestStatus			iterate							(void);
	DerivateCaseDefinition			getDerivateCaseDefinition		(void) { return m_definitions; }
	DerivateCaseValues				getDerivateCaseValues			(void) { return m_values; }

protected:
	virtual bool					verify							(const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask) = 0;
	tcu::Vec4						getSurfaceThreshold				(void) const;
	virtual void					setupDefaultInputs				(void);

	const DerivateCaseDefinition&	m_definitions;
	const DerivateCaseValues&		m_values;
};

static VkSampleCountFlagBits getVkSampleCount (int numSamples)
{
	switch (numSamples)
	{
		case 0:		return VK_SAMPLE_COUNT_1_BIT;
		case 2:		return VK_SAMPLE_COUNT_2_BIT;
		case 4:		return VK_SAMPLE_COUNT_4_BIT;
		default:
			DE_ASSERT(false);
			return (VkSampleCountFlagBits)0;
	}
}

TriangleDerivateCaseInstance::TriangleDerivateCaseInstance (Context&						context,
															const UniformSetup&				uniformSetup,
															const DerivateCaseDefinition&	definitions,
															const DerivateCaseValues&		values)
	: ShaderRenderCaseInstance	(context, true, DE_NULL, uniformSetup, DE_NULL)
	, m_definitions				(definitions)
	, m_values					(values)
{
	m_renderSize	= tcu::UVec2(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	m_colorFormat	= vk::mapTextureFormat(glu::mapGLInternalFormat(m_definitions.surfaceType == SURFACETYPE_FLOAT_FBO ? GL_RGBA32UI : GL_RGBA8));

	setSampleCount(getVkSampleCount(definitions.numSamples));
}

TriangleDerivateCaseInstance::~TriangleDerivateCaseInstance (void)
{
}

tcu::Vec4 TriangleDerivateCaseInstance::getSurfaceThreshold (void) const
{
	switch (m_definitions.surfaceType)
	{
		case SURFACETYPE_UNORM_FBO:				return tcu::IVec4(1).asFloat() / 255.0f;
		case SURFACETYPE_FLOAT_FBO:				return tcu::Vec4(0.0f);
		default:
			DE_ASSERT(false);
			return tcu::Vec4(0.0f);
	}
}

void TriangleDerivateCaseInstance::setupDefaultInputs (void)
{
	const int		numVertices			= 4;
	const float		positions[]			=
	{
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		1.0f,  1.0f, 0.0f, 1.0f
	};
	const float		coords[]			=
	{
		m_values.coordMin.x(), m_values.coordMin.y(), m_values.coordMin.z(),								m_values.coordMax.w(),
		m_values.coordMin.x(), m_values.coordMax.y(), (m_values.coordMin.z()+m_values.coordMax.z())*0.5f,	(m_values.coordMin.w()+m_values.coordMax.w())*0.5f,
		m_values.coordMax.x(), m_values.coordMin.y(), (m_values.coordMin.z()+m_values.coordMax.z())*0.5f,	(m_values.coordMin.w()+m_values.coordMax.w())*0.5f,
		m_values.coordMax.x(), m_values.coordMax.y(), m_values.coordMax.z(),								m_values.coordMin.w()
	};

	addAttribute(0u, vk::VK_FORMAT_R32G32B32A32_SFLOAT, 4 * (deUint32)sizeof(float), numVertices, positions);
	if (m_definitions.coordDataType != glu::TYPE_LAST)
		addAttribute(1u, vk::VK_FORMAT_R32G32B32A32_SFLOAT, 4 * (deUint32)sizeof(float), numVertices, coords);
}

tcu::TestStatus TriangleDerivateCaseInstance::iterate (void)
{
	tcu::TestLog&				log				= m_context.getTestContext().getLog();
	const deUint32				numVertices		= 4;
	const deUint32				numTriangles	= 2;
	const deUint16				indices[]		= { 0, 2, 1, 2, 3, 1 };
	tcu::TextureLevel			resultImage;

	setup();

	render(numVertices, numTriangles, indices);

	{
		const tcu::TextureLevel&		renderedImage	= getResultImage();

		if (m_definitions.surfaceType == SURFACETYPE_FLOAT_FBO)
		{
			const tcu::TextureFormat	dataFormat		(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT);

			resultImage.setStorage(dataFormat, renderedImage.getWidth(), renderedImage.getHeight());
			tcu::copy(resultImage.getAccess(), tcu::ConstPixelBufferAccess(dataFormat, renderedImage.getSize(), renderedImage.getAccess().getDataPtr()));
		}
		else
		{
			resultImage = renderedImage;
		}
	}

	// Verify
	{
		tcu::Surface errorMask(resultImage.getWidth(), resultImage.getHeight());
		tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());

		const bool isOk = verify(resultImage.getAccess(), errorMask.getAccess());

		log << TestLog::ImageSet("Result", "Result images")
			<< TestLog::Image("Rendered", "Rendered image", resultImage);

		if (!isOk)
			log << TestLog::Image("ErrorMask", "Error mask", errorMask);

		log << TestLog::EndImageSet;

		if (isOk)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Image comparison failed");
	}
}

void DerivateUniformSetup::setup (ShaderRenderCaseInstance& instance, const tcu::Vec4&) const
{
	DerivateCaseDefinition	definitions		= dynamic_cast<TriangleDerivateCaseInstance&>(instance).getDerivateCaseDefinition();
	DerivateCaseValues		values			= dynamic_cast<TriangleDerivateCaseInstance&>(instance).getDerivateCaseValues();

	DE_ASSERT(glu::isDataTypeFloatOrVec(definitions.dataType));

	instance.addUniform(0u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, glu::getDataTypeScalarSize(definitions.dataType) * sizeof(float), values.derivScale.getPtr());
	instance.addUniform(1u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, glu::getDataTypeScalarSize(definitions.dataType) * sizeof(float), values.derivBias.getPtr());

	if (m_useSampler)
		instance.useSampler(2u, 0u); // To the uniform binding location 2 bind the texture 0
}

// TriangleDerivateCase

class TriangleDerivateCase : public ShaderRenderCase
{
public:
									TriangleDerivateCase	(tcu::TestContext&		testCtx,
															 const std::string&		name,
															 const std::string&		description,
															 const UniformSetup*	uniformSetup);
	virtual							~TriangleDerivateCase	(void);

protected:
	mutable DerivateCaseDefinition	m_definitions;
	mutable DerivateCaseValues		m_values;
};

TriangleDerivateCase::TriangleDerivateCase (tcu::TestContext&		testCtx,
											const std::string&		name,
											const std::string&		description,
											const UniformSetup*		uniformSetup)
	: ShaderRenderCase		(testCtx, name, description, false, (ShaderEvaluator*)DE_NULL, uniformSetup, DE_NULL)
	, m_definitions			()
{
}

TriangleDerivateCase::~TriangleDerivateCase (void)
{
}

static std::string genVertexSource (glu::DataType coordType, glu::Precision precision)
{
	DE_ASSERT(coordType == glu::TYPE_LAST || glu::isDataTypeFloatOrVec(coordType));

	const std::string vertexTmpl =
		"#version 450\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		+ string(coordType != glu::TYPE_LAST ? "layout(location = 1) in ${PRECISION} ${DATATYPE} a_coord;\n"
											   "layout(location = 0) out ${PRECISION} ${DATATYPE} v_coord;\n" : "") +
		"out gl_PerVertex {\n"
		"	vec4 gl_Position;\n"
		"};\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = a_position;\n"
		+ string(coordType != glu::TYPE_LAST ? "	v_coord = a_coord;\n" : "") +
		"}\n";

	map<string, string> vertexParams;

	if (coordType != glu::TYPE_LAST)
	{
		vertexParams["PRECISION"]	= glu::getPrecisionName(precision);
		vertexParams["DATATYPE"]	= glu::getDataTypeName(coordType);
	}

	return tcu::StringTemplate(vertexTmpl).specialize(vertexParams);
}

// ConstantDerivateCaseInstance

class ConstantDerivateCaseInstance : public TriangleDerivateCaseInstance
{
public:
								ConstantDerivateCaseInstance	(Context&						context,
																 const UniformSetup&			uniformSetup,
																 const DerivateCaseDefinition&	definitions,
																 const DerivateCaseValues&		values);
	virtual						~ConstantDerivateCaseInstance	(void);

	virtual bool				verify							(const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask);
};

ConstantDerivateCaseInstance::ConstantDerivateCaseInstance (Context&							context,
															const UniformSetup&					uniformSetup,
															const DerivateCaseDefinition&		definitions,
															const DerivateCaseValues&			values)
	: TriangleDerivateCaseInstance	(context, uniformSetup, definitions, values)
{
}

ConstantDerivateCaseInstance::~ConstantDerivateCaseInstance (void)
{
}

bool ConstantDerivateCaseInstance::verify (const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask)
{
	const tcu::Vec4 reference	(0.0f); // Derivate of constant argument should always be 0
	const tcu::Vec4	threshold	= getSurfaceThreshold() / abs(m_values.derivScale);

	return verifyConstantDerivate(m_context.getTestContext().getLog(), result, errorMask, m_definitions.dataType,
								  reference, threshold, m_values.derivScale, m_values.derivBias);
}

// ConstantDerivateCase

class ConstantDerivateCase : public TriangleDerivateCase
{
public:
							ConstantDerivateCase		(tcu::TestContext&		testCtx,
														 const std::string&		name,
														 const std::string&		description,
														 DerivateFunc			func,
														 glu::DataType			type);
	virtual					~ConstantDerivateCase		(void);

	virtual	void			initPrograms				(vk::SourceCollections& programCollection) const;
	virtual TestInstance*	createInstance				(Context& context) const;
};

ConstantDerivateCase::ConstantDerivateCase (tcu::TestContext&		testCtx,
											const std::string&		name,
											const std::string&		description,
											DerivateFunc			func,
											glu::DataType			type)
	: TriangleDerivateCase	(testCtx, name, description, new DerivateUniformSetup(false))
{
	m_definitions.func				= func;
	m_definitions.dataType			= type;
	m_definitions.precision			= glu::PRECISION_HIGHP;
}

ConstantDerivateCase::~ConstantDerivateCase (void)
{
}

TestInstance* ConstantDerivateCase::createInstance (Context& context) const
{
	DE_ASSERT(m_uniformSetup != DE_NULL);
	return new ConstantDerivateCaseInstance(context, *m_uniformSetup, m_definitions, m_values);
}

void ConstantDerivateCase::initPrograms (vk::SourceCollections& programCollection) const
{
	const char* fragmentTmpl =
		"#version 450\n"
		"layout(location = 0) out mediump vec4 o_color;\n"
		"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
		"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; }; \n"
		"void main (void)\n"
		"{\n"
		"	${PRECISION} ${DATATYPE} res = ${FUNC}(${VALUE}) * u_scale + u_bias;\n"
		"	o_color = ${CAST_TO_OUTPUT};\n"
		"}\n";

	map<string, string> fragmentParams;
	fragmentParams["PRECISION"]			= glu::getPrecisionName(m_definitions.precision);
	fragmentParams["DATATYPE"]			= glu::getDataTypeName(m_definitions.dataType);
	fragmentParams["FUNC"]				= getDerivateFuncName(m_definitions.func);
	fragmentParams["VALUE"]				= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "vec4(1.0, 7.2, -1e5, 0.0)" :
										  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? "vec3(1e2, 8.0, 0.01)" :
										  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? "vec2(-0.0, 2.7)" :
										  /* TYPE_FLOAT */								   "7.7";
	fragmentParams["CAST_TO_OUTPUT"]	= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "res" :
										  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? "vec4(res, 1.0)" :
										  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? "vec4(res, 0.0, 1.0)" :
										  /* TYPE_FLOAT */								   "vec4(res, 0.0, 0.0, 1.0)";

	std::string fragmentSrc = tcu::StringTemplate(fragmentTmpl).specialize(fragmentParams);
	programCollection.glslSources.add("vert") << glu::VertexSource(genVertexSource(m_definitions.coordDataType, m_definitions.coordPrecision));
	programCollection.glslSources.add("frag") << glu::FragmentSource(fragmentSrc);

	m_values.derivScale		= tcu::Vec4(1e3f, 1e3f, 1e3f, 1e3f);
	m_values.derivBias		= tcu::Vec4(0.5f, 0.5f, 0.5f, 0.5f);
}

// Linear cases

class LinearDerivateUniformSetup : public DerivateUniformSetup
{
public:
					LinearDerivateUniformSetup		(bool useSampler, BaseUniformType usedDefaultUniform);
	virtual			~LinearDerivateUniformSetup		(void);

	virtual void	setup							(ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) const;

private:
	const BaseUniformType	m_usedDefaultUniform;
};

LinearDerivateUniformSetup::LinearDerivateUniformSetup (bool useSampler, BaseUniformType usedDefaultUniform)
	: DerivateUniformSetup	(useSampler)
	, m_usedDefaultUniform	(usedDefaultUniform)
{
}

LinearDerivateUniformSetup::~LinearDerivateUniformSetup (void)
{
}

void LinearDerivateUniformSetup::setup (ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) const
{
	DerivateUniformSetup::setup(instance, constCoords);

	if (m_usedDefaultUniform != U_LAST)
		switch (m_usedDefaultUniform)
		{
			case UB_TRUE:
			case UI_ONE:
			case UI_TWO:
				instance.useUniform(2u, m_usedDefaultUniform);
				break;
			default:
				DE_ASSERT(false);
				break;
		}
}

class LinearDerivateCaseInstance : public TriangleDerivateCaseInstance
{
public:
								LinearDerivateCaseInstance	(Context&						context,
															 const UniformSetup&			uniformSetup,
															 const DerivateCaseDefinition&	definitions,
															 const DerivateCaseValues&		values);
	virtual						~LinearDerivateCaseInstance	(void);

	virtual bool				verify						(const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask);
};

LinearDerivateCaseInstance::LinearDerivateCaseInstance (Context&						context,
														const UniformSetup&				uniformSetup,
														const DerivateCaseDefinition&	definitions,
														const DerivateCaseValues&		values)
	: TriangleDerivateCaseInstance	(context, uniformSetup, definitions, values)
{
}

LinearDerivateCaseInstance::~LinearDerivateCaseInstance (void)
{
}

bool LinearDerivateCaseInstance::verify (const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask)
{
	const tcu::Vec4		xScale				= tcu::Vec4(1.0f, 0.0f, 0.5f, -0.5f);
	const tcu::Vec4		yScale				= tcu::Vec4(0.0f, 1.0f, 0.5f, -0.5f);
	const tcu::Vec4		surfaceThreshold	= getSurfaceThreshold() / abs(m_values.derivScale);

	if (isDfdxFunc(m_definitions.func) || isDfdyFunc(m_definitions.func))
	{
		const bool			isX			= isDfdxFunc(m_definitions.func);
		const float			div			= isX ? float(result.getWidth()) : float(result.getHeight());
		const tcu::Vec4		scale		= isX ? xScale : yScale;
		tcu::Vec4			reference	= ((m_values.coordMax - m_values.coordMin) / div);
		const tcu::Vec4		opThreshold	= getDerivateThreshold(m_definitions.precision, m_values.coordMin, m_values.coordMax, reference);
		const tcu::Vec4		threshold	= max(surfaceThreshold, opThreshold);
		const int			numComps	= glu::getDataTypeFloatScalars(m_definitions.dataType);

		/* adjust the reference value for the correct dfdx or dfdy sample adjacency */
		reference = reference * scale;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Verifying result image.\n"
			<< "\tValid derivative is " << LogVecComps(reference, numComps) << " with threshold " << LogVecComps(threshold, numComps)
			<< tcu::TestLog::EndMessage;

		// short circuit if result is strictly within the normal value error bounds.
		// This improves performance significantly.
		if (verifyConstantDerivate(m_context.getTestContext().getLog(), result, errorMask, m_definitions.dataType,
								   reference, threshold, m_values.derivScale, m_values.derivBias,
								   LOG_NOTHING))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "No incorrect derivatives found, result valid."
				<< tcu::TestLog::EndMessage;

			return true;
		}

		// some pixels exceed error bounds calculated for normal values. Verify that these
		// potentially invalid pixels are in fact valid due to (for example) subnorm flushing.

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Initial verification failed, verifying image by calculating accurate error bounds for each result pixel.\n"
			<< "\tVerifying each result derivative is within its range of legal result values."
			<< tcu::TestLog::EndMessage;

		{
			const tcu::UVec2			viewportSize	(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
			const float					w				= float(viewportSize.x());
			const float					h				= float(viewportSize.y());
			const tcu::Vec4				valueRamp		= (m_values.coordMax - m_values.coordMin);
			Linear2DFunctionEvaluator	function;

			function.matrix.setRow(0, tcu::Vec3(valueRamp.x() / w, 0.0f, m_values.coordMin.x()));
			function.matrix.setRow(1, tcu::Vec3(0.0f, valueRamp.y() / h, m_values.coordMin.y()));
			function.matrix.setRow(2, tcu::Vec3(valueRamp.z() / w, valueRamp.z() / h, m_values.coordMin.z() + m_values.coordMin.z()) / 2.0f);
			function.matrix.setRow(3, tcu::Vec3(-valueRamp.w() / w, -valueRamp.w() / h, m_values.coordMax.w() + m_values.coordMax.w()) / 2.0f);

			return reverifyConstantDerivateWithFlushRelaxations(m_context.getTestContext().getLog(), result, errorMask,
																m_definitions.dataType, m_definitions.precision, m_values.derivScale,
																m_values.derivBias, surfaceThreshold, m_definitions.func,
																function);
		}
	}
	else
	{
		DE_ASSERT(isFwidthFunc(m_definitions.func));
		const float			w			= float(result.getWidth());
		const float			h			= float(result.getHeight());

		const tcu::Vec4		dx			= ((m_values.coordMax - m_values.coordMin) / w) * xScale;
		const tcu::Vec4		dy			= ((m_values.coordMax - m_values.coordMin) / h) * yScale;
		const tcu::Vec4		reference	= tcu::abs(dx) + tcu::abs(dy);
		const tcu::Vec4		dxThreshold	= getDerivateThreshold(m_definitions.precision, m_values.coordMin*xScale, m_values.coordMax*xScale, dx);
		const tcu::Vec4		dyThreshold	= getDerivateThreshold(m_definitions.precision, m_values.coordMin*yScale, m_values.coordMax*yScale, dy);
		const tcu::Vec4		threshold	= max(surfaceThreshold, max(dxThreshold, dyThreshold));

		return verifyConstantDerivate(m_context.getTestContext().getLog(), result, errorMask, m_definitions.dataType,
									  reference, threshold, m_values.derivScale, m_values.derivBias);
	}
}

// LinearDerivateCase

class LinearDerivateCase : public TriangleDerivateCase
{
public:
							LinearDerivateCase			(tcu::TestContext&		testCtx,
														 const std::string&		name,
														 const std::string&		description,
														 DerivateFunc			func,
														 glu::DataType			type,
														 glu::Precision			precision,
														 SurfaceType			surfaceType,
														 int					numSamples,
														 const std::string&		fragmentSrcTmpl,
														 BaseUniformType		usedDefaultUniform);
	virtual					~LinearDerivateCase			(void);

	virtual	void			initPrograms				(vk::SourceCollections& programCollection) const;
	virtual TestInstance*	createInstance				(Context& context) const;

private:
	const std::string		m_fragmentTmpl;
};

LinearDerivateCase::LinearDerivateCase (tcu::TestContext&		testCtx,
										const std::string&		name,
										const std::string&		description,
										DerivateFunc			func,
										glu::DataType			type,
										glu::Precision			precision,
										SurfaceType				surfaceType,
										int						numSamples,
										const std::string&		fragmentSrcTmpl,
										BaseUniformType			usedDefaultUniform)
	: TriangleDerivateCase	(testCtx, name, description, new LinearDerivateUniformSetup(false, usedDefaultUniform))
	, m_fragmentTmpl		(fragmentSrcTmpl)
{
	m_definitions.func				= func;
	m_definitions.dataType			= type;
	m_definitions.precision			= precision;
	m_definitions.coordDataType		= m_definitions.dataType;
	m_definitions.coordPrecision	= m_definitions.precision;
	m_definitions.surfaceType		= surfaceType;
	m_definitions.numSamples		= numSamples;
}

LinearDerivateCase::~LinearDerivateCase (void)
{
}

TestInstance* LinearDerivateCase::createInstance (Context& context) const
{
	DE_ASSERT(m_uniformSetup != DE_NULL);
	return new LinearDerivateCaseInstance(context, *m_uniformSetup, m_definitions, m_values);
}

void LinearDerivateCase::initPrograms (vk::SourceCollections& programCollection) const
{
	const tcu::UVec2	viewportSize	(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	const float			w				= float(viewportSize.x());
	const float			h				= float(viewportSize.y());
	const bool			packToInt		= m_definitions.surfaceType == SURFACETYPE_FLOAT_FBO;
	map<string, string>	fragmentParams;

	fragmentParams["OUTPUT_TYPE"]		= glu::getDataTypeName(packToInt ? glu::TYPE_UINT_VEC4 : glu::TYPE_FLOAT_VEC4);
	fragmentParams["OUTPUT_PREC"]		= glu::getPrecisionName(packToInt ? glu::PRECISION_HIGHP : m_definitions.precision);
	fragmentParams["PRECISION"]			= glu::getPrecisionName(m_definitions.precision);
	fragmentParams["DATATYPE"]			= glu::getDataTypeName(m_definitions.dataType);
	fragmentParams["FUNC"]				= getDerivateFuncName(m_definitions.func);

	if (packToInt)
	{
		fragmentParams["CAST_TO_OUTPUT"]	= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "floatBitsToUint(res)" :
											  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? "floatBitsToUint(vec4(res, 1.0))" :
											  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? "floatBitsToUint(vec4(res, 0.0, 1.0))" :
											  /* TYPE_FLOAT */								   "floatBitsToUint(vec4(res, 0.0, 0.0, 1.0))";
	}
	else
	{
		fragmentParams["CAST_TO_OUTPUT"]	= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "res" :
											  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? "vec4(res, 1.0)" :
											  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? "vec4(res, 0.0, 1.0)" :
											  /* TYPE_FLOAT */								   "vec4(res, 0.0, 0.0, 1.0)";
	}

	std::string fragmentSrc = tcu::StringTemplate(m_fragmentTmpl).specialize(fragmentParams);
	programCollection.glslSources.add("vert") << glu::VertexSource(genVertexSource(m_definitions.coordDataType, m_definitions.coordPrecision));
	programCollection.glslSources.add("frag") << glu::FragmentSource(fragmentSrc);

	switch (m_definitions.precision)
	{
		case glu::PRECISION_HIGHP:
			m_values.coordMin = tcu::Vec4(-97.f, 0.2f, 71.f, 74.f);
			m_values.coordMax = tcu::Vec4(-13.2f, -77.f, 44.f, 76.f);
			break;

		case glu::PRECISION_MEDIUMP:
			m_values.coordMin = tcu::Vec4(-37.0f, 47.f, -7.f, 0.0f);
			m_values.coordMax = tcu::Vec4(-1.0f, 12.f, 7.f, 19.f);
			break;

		case glu::PRECISION_LOWP:
			m_values.coordMin = tcu::Vec4(0.0f, -1.0f, 0.0f, 1.0f);
			m_values.coordMax = tcu::Vec4(1.0f, 1.0f, -1.0f, -1.0f);
			break;

		default:
			DE_ASSERT(false);
	}

	if (m_definitions.surfaceType == SURFACETYPE_FLOAT_FBO)
	{
		// No scale or bias used for accuracy.
		m_values.derivScale	= tcu::Vec4(1.0f);
		m_values.derivBias		= tcu::Vec4(0.0f);
	}
	else
	{
		// Compute scale - bias that normalizes to 0..1 range.
		const tcu::Vec4 dx = (m_values.coordMax - m_values.coordMin) / tcu::Vec4(w, w, w*0.5f, -w*0.5f);
		const tcu::Vec4 dy = (m_values.coordMax - m_values.coordMin) / tcu::Vec4(h, h, h*0.5f, -h*0.5f);

		if (isDfdxFunc(m_definitions.func))
			m_values.derivScale = 0.5f / dx;
		else if (isDfdyFunc(m_definitions.func))
			m_values.derivScale = 0.5f / dy;
		else if (isFwidthFunc(m_definitions.func))
			m_values.derivScale = 0.5f / (tcu::abs(dx) + tcu::abs(dy));
		else
			DE_ASSERT(false);

		m_values.derivBias = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

// TextureDerivateCaseInstance

class TextureDerivateCaseInstance : public TriangleDerivateCaseInstance
{
public:
								TextureDerivateCaseInstance		(Context&							context,
																 const UniformSetup&				uniformSetup,
																 const DerivateCaseDefinition&		definitions,
																 const DerivateCaseValues&			values,
																 const TextureCaseValues&			textureValues);
	virtual						~TextureDerivateCaseInstance	(void);

	virtual bool				verify							(const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask);

private:
	const TextureCaseValues&	m_textureValues;
};

TextureDerivateCaseInstance::TextureDerivateCaseInstance (Context&							context,
														  const UniformSetup&				uniformSetup,
														  const DerivateCaseDefinition&		definitions,
														  const DerivateCaseValues&			values,
														  const TextureCaseValues&			textureValues)
	: TriangleDerivateCaseInstance	(context, uniformSetup, definitions, values)
	, m_textureValues				(textureValues)
{
	de::MovePtr<tcu::Texture2D>		texture;

	// Lowp and mediump cases use RGBA16F format, while highp uses RGBA32F.
	{
		const tcu::UVec2			viewportSize	(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		const tcu::TextureFormat	format			= glu::mapGLInternalFormat(m_definitions.precision == glu::PRECISION_HIGHP ? GL_RGBA32F : GL_RGBA16F);

		texture = de::MovePtr<tcu::Texture2D>(new tcu::Texture2D(format, viewportSize.x(), viewportSize.y()));
		texture->allocLevel(0);
	}

	// Fill with gradients.
	{
		const tcu::PixelBufferAccess level0 = texture->getLevel(0);
		for (int y = 0; y < level0.getHeight(); y++)
		{
			for (int x = 0; x < level0.getWidth(); x++)
			{
				const float		xf		= (float(x)+0.5f) / float(level0.getWidth());
				const float		yf		= (float(y)+0.5f) / float(level0.getHeight());
				const tcu::Vec4	s		= tcu::Vec4(xf, yf, (xf+yf)/2.0f, 1.0f - (xf+yf)/2.0f);

				level0.setPixel(m_textureValues.texValueMin + (m_textureValues.texValueMax - m_textureValues.texValueMin)*s, x, y);
			}
		}
	}

	de::SharedPtr<TextureBinding>	testTexture		(new TextureBinding(texture.release(),
																		tcu::Sampler(tcu::Sampler::CLAMP_TO_EDGE,
																					 tcu::Sampler::CLAMP_TO_EDGE,
																					 tcu::Sampler::CLAMP_TO_EDGE,
																					 tcu::Sampler::NEAREST,
																					 tcu::Sampler::NEAREST)));
	m_textures.push_back(testTexture);
}

TextureDerivateCaseInstance::~TextureDerivateCaseInstance (void)
{
}

bool TextureDerivateCaseInstance::verify (const tcu::ConstPixelBufferAccess& result, const tcu::PixelBufferAccess& errorMask)
{
	// \note Edges are ignored in comparison
	if (result.getWidth() < 2 || result.getHeight() < 2)
		throw tcu::NotSupportedError("Too small viewport");

	tcu::ConstPixelBufferAccess	compareArea			= tcu::getSubregion(result, 1, 1, result.getWidth()-2, result.getHeight()-2);
	tcu::PixelBufferAccess		maskArea			= tcu::getSubregion(errorMask, 1, 1, errorMask.getWidth()-2, errorMask.getHeight()-2);
	const tcu::Vec4				xScale				= tcu::Vec4(1.0f, 0.0f, 0.5f, -0.5f);
	const tcu::Vec4				yScale				= tcu::Vec4(0.0f, 1.0f, 0.5f, -0.5f);
	const float					w					= float(result.getWidth());
	const float					h					= float(result.getHeight());

	const tcu::Vec4				surfaceThreshold	= getSurfaceThreshold() / abs(m_values.derivScale);

	if (isDfdxFunc(m_definitions.func) || isDfdyFunc(m_definitions.func))
	{
		const bool			isX			= isDfdxFunc(m_definitions.func);
		const float			div			= isX ? w : h;
		const tcu::Vec4		scale		= isX ? xScale : yScale;
		tcu::Vec4			reference	= ((m_textureValues.texValueMax - m_textureValues.texValueMin) / div);
		const tcu::Vec4		opThreshold	= getDerivateThreshold(m_definitions.precision, m_textureValues.texValueMin, m_textureValues.texValueMax, reference);
		const tcu::Vec4		threshold	= max(surfaceThreshold, opThreshold);
		const int			numComps	= glu::getDataTypeFloatScalars(m_definitions.dataType);

		/* adjust the reference value for the correct dfdx or dfdy sample adjacency */
		reference = reference * scale;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Verifying result image.\n"
			<< "\tValid derivative is " << LogVecComps(reference, numComps) << " with threshold " << LogVecComps(threshold, numComps)
			<< tcu::TestLog::EndMessage;

		// short circuit if result is strictly within the normal value error bounds.
		// This improves performance significantly.
		if (verifyConstantDerivate(m_context.getTestContext().getLog(), compareArea, maskArea, m_definitions.dataType,
								   reference, threshold, m_values.derivScale, m_values.derivBias,
								   LOG_NOTHING))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "No incorrect derivatives found, result valid."
				<< tcu::TestLog::EndMessage;

			return true;
		}

		// some pixels exceed error bounds calculated for normal values. Verify that these
		// potentially invalid pixels are in fact valid due to (for example) subnorm flushing.

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Initial verification failed, verifying image by calculating accurate error bounds for each result pixel.\n"
			<< "\tVerifying each result derivative is within its range of legal result values."
			<< tcu::TestLog::EndMessage;

		{
			const tcu::Vec4				valueRamp		= (m_textureValues.texValueMax - m_textureValues.texValueMin);
			Linear2DFunctionEvaluator	function;

			function.matrix.setRow(0, tcu::Vec3(valueRamp.x() / w, 0.0f, m_textureValues.texValueMin.x()));
			function.matrix.setRow(1, tcu::Vec3(0.0f, valueRamp.y() / h, m_textureValues.texValueMin.y()));
			function.matrix.setRow(2, tcu::Vec3(valueRamp.z() / w, valueRamp.z() / h, m_textureValues.texValueMin.z() + m_textureValues.texValueMin.z()) / 2.0f);
			function.matrix.setRow(3, tcu::Vec3(-valueRamp.w() / w, -valueRamp.w() / h, m_textureValues.texValueMax.w() + m_textureValues.texValueMax.w()) / 2.0f);

			return reverifyConstantDerivateWithFlushRelaxations(m_context.getTestContext().getLog(), compareArea, maskArea,
																m_definitions.dataType, m_definitions.precision, m_values.derivScale,
																m_values.derivBias, surfaceThreshold, m_definitions.func,
																function);
		}
	}
	else
	{
		DE_ASSERT(isFwidthFunc(m_definitions.func));
		const tcu::Vec4	dx			= ((m_textureValues.texValueMax - m_textureValues.texValueMin) / w) * xScale;
		const tcu::Vec4	dy			= ((m_textureValues.texValueMax - m_textureValues.texValueMin) / h) * yScale;
		const tcu::Vec4	reference	= tcu::abs(dx) + tcu::abs(dy);
		const tcu::Vec4	dxThreshold	= getDerivateThreshold(m_definitions.precision, m_textureValues.texValueMin*xScale, m_textureValues.texValueMax*xScale, dx);
		const tcu::Vec4	dyThreshold	= getDerivateThreshold(m_definitions.precision, m_textureValues.texValueMin*yScale, m_textureValues.texValueMax*yScale, dy);
		const tcu::Vec4	threshold	= max(surfaceThreshold, max(dxThreshold, dyThreshold));

		return verifyConstantDerivate(m_context.getTestContext().getLog(), compareArea, maskArea, m_definitions.dataType,
									  reference, threshold, m_values.derivScale, m_values.derivBias);
	}
}

// TextureDerivateCase

class TextureDerivateCase : public TriangleDerivateCase
{
public:
							TextureDerivateCase			(tcu::TestContext&		testCtx,
														 const std::string&		name,
														 const std::string&		description,
														 DerivateFunc			func,
														 glu::DataType			type,
														 glu::Precision			precision,
														 SurfaceType			surfaceType,
														 int					numSamples);
	virtual					~TextureDerivateCase		(void);

	virtual	void			initPrograms				(vk::SourceCollections& programCollection) const;
	virtual TestInstance*	createInstance				(Context& context) const;

private:
	mutable TextureCaseValues	m_textureValues;
};

TextureDerivateCase::TextureDerivateCase (tcu::TestContext&		testCtx,
										  const std::string&	name,
										  const std::string&	description,
										  DerivateFunc			func,
										  glu::DataType			type,
										  glu::Precision		precision,
										  SurfaceType			surfaceType,
										  int					numSamples)
	: TriangleDerivateCase	(testCtx, name, description, new DerivateUniformSetup(true))
{
	m_definitions.dataType			= type;
	m_definitions.func				= func;
	m_definitions.precision			= precision;
	m_definitions.coordDataType		= glu::TYPE_FLOAT_VEC2;
	m_definitions.coordPrecision	= glu::PRECISION_HIGHP;
	m_definitions.surfaceType		= surfaceType;
	m_definitions.numSamples		= numSamples;
}

TextureDerivateCase::~TextureDerivateCase (void)
{
}

TestInstance* TextureDerivateCase::createInstance (Context& context) const
{
	DE_ASSERT(m_uniformSetup != DE_NULL);
	return new TextureDerivateCaseInstance(context, *m_uniformSetup, m_definitions, m_values, m_textureValues);
}

void TextureDerivateCase::initPrograms (vk::SourceCollections& programCollection) const
{
	// Generate shader
	{
		const char* fragmentTmpl =
			"#version 450\n"
			"layout(location = 0) in highp vec2 v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"layout(binding = 2) uniform ${PRECISION} sampler2D u_sampler;\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} vec4 tex = texture(u_sampler, v_coord);\n"
			"	${PRECISION} ${DATATYPE} res = ${FUNC}(tex${SWIZZLE}) * u_scale + u_bias;\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n";

		const bool			packToInt		= m_definitions.surfaceType == SURFACETYPE_FLOAT_FBO;
		map<string, string> fragmentParams;

		fragmentParams["OUTPUT_TYPE"]		= glu::getDataTypeName(packToInt ? glu::TYPE_UINT_VEC4 : glu::TYPE_FLOAT_VEC4);
		fragmentParams["OUTPUT_PREC"]		= glu::getPrecisionName(packToInt ? glu::PRECISION_HIGHP : m_definitions.precision);
		fragmentParams["PRECISION"]			= glu::getPrecisionName(m_definitions.precision);
		fragmentParams["DATATYPE"]			= glu::getDataTypeName(m_definitions.dataType);
		fragmentParams["FUNC"]				= getDerivateFuncName(m_definitions.func);
		fragmentParams["SWIZZLE"]			= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "" :
											  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? ".xyz" :
											  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? ".xy" :
											  /* TYPE_FLOAT */								   ".x";

		if (packToInt)
		{
			fragmentParams["CAST_TO_OUTPUT"]	= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "floatBitsToUint(res)" :
												  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? "floatBitsToUint(vec4(res, 1.0))" :
												  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? "floatBitsToUint(vec4(res, 0.0, 1.0))" :
												  /* TYPE_FLOAT */								   "floatBitsToUint(vec4(res, 0.0, 0.0, 1.0))";
		}
		else
		{
			fragmentParams["CAST_TO_OUTPUT"]	= m_definitions.dataType == glu::TYPE_FLOAT_VEC4 ? "res" :
												  m_definitions.dataType == glu::TYPE_FLOAT_VEC3 ? "vec4(res, 1.0)" :
												  m_definitions.dataType == glu::TYPE_FLOAT_VEC2 ? "vec4(res, 0.0, 1.0)" :
												  /* TYPE_FLOAT */								   "vec4(res, 0.0, 0.0, 1.0)";
		}

		std::string fragmentSrc = tcu::StringTemplate(fragmentTmpl).specialize(fragmentParams);
		programCollection.glslSources.add("vert") << glu::VertexSource(genVertexSource(m_definitions.coordDataType, m_definitions.coordPrecision));
		programCollection.glslSources.add("frag") << glu::FragmentSource(fragmentSrc);
	}

	// Texture size matches viewport and nearest sampling is used. Thus texture sampling
	// is equal to just interpolating the texture value range.

	// Determine value range for texture.

	switch (m_definitions.precision)
	{
		case glu::PRECISION_HIGHP:
			m_textureValues.texValueMin = tcu::Vec4(-97.f, 0.2f, 71.f, 74.f);
			m_textureValues.texValueMax = tcu::Vec4(-13.2f, -77.f, 44.f, 76.f);
			break;

		case glu::PRECISION_MEDIUMP:
			m_textureValues.texValueMin = tcu::Vec4(-37.0f, 47.f, -7.f, 0.0f);
			m_textureValues.texValueMax = tcu::Vec4(-1.0f, 12.f, 7.f, 19.f);
			break;

		case glu::PRECISION_LOWP:
			m_textureValues.texValueMin = tcu::Vec4(0.0f, -1.0f, 0.0f, 1.0f);
			m_textureValues.texValueMax = tcu::Vec4(1.0f, 1.0f, -1.0f, -1.0f);
			break;

		default:
			DE_ASSERT(false);
	}

	// Texture coordinates
	m_values.coordMin = tcu::Vec4(0.0f);
	m_values.coordMax = tcu::Vec4(1.0f);

	if (m_definitions.surfaceType == SURFACETYPE_FLOAT_FBO)
	{
		// No scale or bias used for accuracy.
		m_values.derivScale		= tcu::Vec4(1.0f);
		m_values.derivBias		= tcu::Vec4(0.0f);
	}
	else
	{
		// Compute scale - bias that normalizes to 0..1 range.
		const tcu::UVec2	viewportSize	(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		const float			w				= float(viewportSize.x());
		const float			h				= float(viewportSize.y());
		const tcu::Vec4		dx				= (m_textureValues.texValueMax - m_textureValues.texValueMin) / tcu::Vec4(w, w, w*0.5f, -w*0.5f);
		const tcu::Vec4		dy				= (m_textureValues.texValueMax - m_textureValues.texValueMin) / tcu::Vec4(h, h, h*0.5f, -h*0.5f);

		if (isDfdxFunc(m_definitions.func))
			m_values.derivScale = 0.5f / dx;
		else if (isDfdyFunc(m_definitions.func))
			m_values.derivScale = 0.5f / dy;
		else if (isFwidthFunc(m_definitions.func))
			m_values.derivScale = 0.5f / (tcu::abs(dx) + tcu::abs(dy));
		else
			DE_ASSERT(false);

		m_values.derivBias = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

// ShaderDerivateTests

class ShaderDerivateTests : public tcu::TestCaseGroup
{
public:
							ShaderDerivateTests		(tcu::TestContext& testCtx);
	virtual					~ShaderDerivateTests	(void);

	virtual void			init					(void);

private:
							ShaderDerivateTests		(const ShaderDerivateTests&);		// not allowed!
	ShaderDerivateTests&	operator=				(const ShaderDerivateTests&);		// not allowed!
};

ShaderDerivateTests::ShaderDerivateTests (tcu::TestContext& testCtx)
	: TestCaseGroup(testCtx, "derivate", "Derivate Function Tests")
{
}

ShaderDerivateTests::~ShaderDerivateTests (void)
{
}

struct FunctionSpec
{
	std::string		name;
	DerivateFunc	function;
	glu::DataType	dataType;
	glu::Precision	precision;

	FunctionSpec (const std::string& name_, DerivateFunc function_, glu::DataType dataType_, glu::Precision precision_)
		: name		(name_)
		, function	(function_)
		, dataType	(dataType_)
		, precision	(precision_)
	{
	}
};

void ShaderDerivateTests::init (void)
{
	static const struct
	{
		const char*			name;
		const char*			description;
		const char*			source;
		BaseUniformType		usedDefaultUniform;
	} s_linearDerivateCases[] =
	{
		{
			"linear",
			"Basic derivate of linearly interpolated argument",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res = ${FUNC}(v_coord) * u_scale + u_bias;\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			U_LAST
		},
		{
			"in_function",
			"Derivate of linear function argument",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"\n"
			"${PRECISION} ${DATATYPE} computeRes (${PRECISION} ${DATATYPE} value)\n"
			"{\n"
			"	return ${FUNC}(v_coord) * u_scale + u_bias;\n"
			"}\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res = computeRes(v_coord);\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			U_LAST
		},
		{
			"static_if",
			"Derivate of linearly interpolated value in static if",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res;\n"
			"	if (false)\n"
			"		res = ${FUNC}(-v_coord) * u_scale + u_bias;\n"
			"	else\n"
			"		res = ${FUNC}(v_coord) * u_scale + u_bias;\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			U_LAST
		},
		{
			"static_loop",
			"Derivate of linearly interpolated value in static loop",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res = ${DATATYPE}(0.0);\n"
			"	for (int i = 0; i < 2; i++)\n"
			"		res += ${FUNC}(v_coord * float(i));\n"
			"	res = res * u_scale + u_bias;\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			U_LAST
		},
		{
			"static_switch",
			"Derivate of linearly interpolated value in static switch",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res;\n"
			"	switch (1)\n"
			"	{\n"
			"		case 0:	res = ${FUNC}(-v_coord) * u_scale + u_bias;	break;\n"
			"		case 1:	res = ${FUNC}(v_coord) * u_scale + u_bias;	break;\n"
			"	}\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			U_LAST
		},
		{
			"uniform_if",
			"Derivate of linearly interpolated value in uniform if",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"layout(binding = 2, std140) uniform Ui_true { bool ub_true; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res;\n"
			"	if (ub_true)"
			"		res = ${FUNC}(v_coord) * u_scale + u_bias;\n"
			"	else\n"
			"		res = ${FUNC}(-v_coord) * u_scale + u_bias;\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			UB_TRUE
		},
		{
			"uniform_loop",
			"Derivate of linearly interpolated value in uniform loop",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"layout(binding = 2, std140) uniform Ui_two { int ui_two; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res = ${DATATYPE}(0.0);\n"
			"	for (int i = 0; i < ui_two; i++)\n"
			"		res += ${FUNC}(v_coord * float(i));\n"
			"	res = res * u_scale + u_bias;\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			UI_TWO
		},
		{
			"uniform_switch",
			"Derivate of linearly interpolated value in uniform switch",

			"#version 450\n"
			"layout(location = 0) in ${PRECISION} ${DATATYPE} v_coord;\n"
			"layout(location = 0) out ${OUTPUT_PREC} ${OUTPUT_TYPE} o_color;\n"
			"layout(binding = 0, std140) uniform Scale { ${PRECISION} ${DATATYPE} u_scale; };\n"
			"layout(binding = 1, std140) uniform Bias { ${PRECISION} ${DATATYPE} u_bias; };\n"
			"layout(binding = 2, std140) uniform Ui_one { int ui_one; };\n"
			"void main (void)\n"
			"{\n"
			"	${PRECISION} ${DATATYPE} res;\n"
			"	switch (ui_one)\n"
			"	{\n"
			"		case 0:	res = ${FUNC}(-v_coord) * u_scale + u_bias;	break;\n"
			"		case 1:	res = ${FUNC}(v_coord) * u_scale + u_bias;	break;\n"
			"	}\n"
			"	o_color = ${CAST_TO_OUTPUT};\n"
			"}\n",

			UI_ONE
		},
	};

	static const struct
	{
		const char*		name;
		SurfaceType		surfaceType;
		int				numSamples;
	} s_fboConfigs[] =
	{
		{ "fbo",			SURFACETYPE_UNORM_FBO,		0 },
		{ "fbo_msaa2",		SURFACETYPE_UNORM_FBO,		2 },
		{ "fbo_msaa4",		SURFACETYPE_UNORM_FBO,		4 },
		{ "fbo_float",		SURFACETYPE_FLOAT_FBO,		0 },
	};

	static const struct
	{
		const char*		name;
		SurfaceType		surfaceType;
		int				numSamples;
	} s_textureConfigs[] =
	{
		{ "basic",			SURFACETYPE_UNORM_FBO,		0 },
		{ "msaa4",			SURFACETYPE_UNORM_FBO,		4 },
		{ "float",			SURFACETYPE_FLOAT_FBO,		0 },
	};

	// .dfdx[fine|coarse], .dfdy[fine|coarse], .fwidth[fine|coarse]
	for (int funcNdx = 0; funcNdx < DERIVATE_LAST; funcNdx++)
	{
		const DerivateFunc					function		= DerivateFunc(funcNdx);
		de::MovePtr<tcu::TestCaseGroup>		functionGroup	(new tcu::TestCaseGroup(m_testCtx, getDerivateFuncCaseName(function), getDerivateFuncName(function)));

		// .constant - no precision variants, checks that derivate of constant arguments is 0
		{
			de::MovePtr<tcu::TestCaseGroup>	constantGroup	(new tcu::TestCaseGroup(m_testCtx, "constant", "Derivate of constant argument"));

			for (int vecSize = 1; vecSize <= 4; vecSize++)
			{
				const glu::DataType			dataType		= vecSize > 1 ? glu::getDataTypeFloatVec(vecSize) : glu::TYPE_FLOAT;
				constantGroup->addChild(new ConstantDerivateCase(m_testCtx, glu::getDataTypeName(dataType), "", function, dataType));
			}

			functionGroup->addChild(constantGroup.release());
		}

		// Cases based on LinearDerivateCase
		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(s_linearDerivateCases); caseNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	linearCaseGroup	(new tcu::TestCaseGroup(m_testCtx, s_linearDerivateCases[caseNdx].name, s_linearDerivateCases[caseNdx].description));
			const char*						source			= s_linearDerivateCases[caseNdx].source;

			for (int vecSize = 1; vecSize <= 4; vecSize++)
			{
				for (int precNdx = 0; precNdx < glu::PRECISION_LAST; precNdx++)
				{
					const glu::DataType		dataType		= vecSize > 1 ? glu::getDataTypeFloatVec(vecSize) : glu::TYPE_FLOAT;
					const glu::Precision	precision		= glu::Precision(precNdx);
					const SurfaceType		surfaceType		= SURFACETYPE_UNORM_FBO;
					const int				numSamples		= 0;
					std::ostringstream		caseName;

					if (caseNdx != 0 && precision == glu::PRECISION_LOWP)
						continue; // Skip as lowp doesn't actually produce any bits when rendered to default FB.

					caseName << glu::getDataTypeName(dataType) << "_" << glu::getPrecisionName(precision);

					linearCaseGroup->addChild(new LinearDerivateCase(m_testCtx, caseName.str(), "", function, dataType, precision, surfaceType, numSamples, source, s_linearDerivateCases[caseNdx].usedDefaultUniform));
				}
			}

			functionGroup->addChild(linearCaseGroup.release());
		}

		// Fbo cases
		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(s_fboConfigs); caseNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	fboGroup		(new tcu::TestCaseGroup(m_testCtx, s_fboConfigs[caseNdx].name, "Derivate usage when rendering into FBO"));
			const char*						source			= s_linearDerivateCases[0].source; // use source from .linear group
			const SurfaceType				surfaceType		= s_fboConfigs[caseNdx].surfaceType;
			const int						numSamples		= s_fboConfigs[caseNdx].numSamples;

			for (int vecSize = 1; vecSize <= 4; vecSize++)
			{
				for (int precNdx = 0; precNdx < glu::PRECISION_LAST; precNdx++)
				{
					const glu::DataType		dataType		= vecSize > 1 ? glu::getDataTypeFloatVec(vecSize) : glu::TYPE_FLOAT;
					const glu::Precision	precision		= glu::Precision(precNdx);
					std::ostringstream		caseName;

					if (surfaceType != SURFACETYPE_FLOAT_FBO && precision == glu::PRECISION_LOWP)
						continue; // Skip as lowp doesn't actually produce any bits when rendered to U8 RT.

					caseName << glu::getDataTypeName(dataType) << "_" << glu::getPrecisionName(precision);

					fboGroup->addChild(new LinearDerivateCase(m_testCtx, caseName.str(), "", function, dataType, precision, surfaceType, numSamples, source, U_LAST));
				}
			}

			functionGroup->addChild(fboGroup.release());
		}

		// .texture
		{
			de::MovePtr<tcu::TestCaseGroup>		textureGroup	(new tcu::TestCaseGroup(m_testCtx, "texture", "Derivate of texture lookup result"));

			for (int texCaseNdx = 0; texCaseNdx < DE_LENGTH_OF_ARRAY(s_textureConfigs); texCaseNdx++)
			{
				de::MovePtr<tcu::TestCaseGroup>	caseGroup		(new tcu::TestCaseGroup(m_testCtx, s_textureConfigs[texCaseNdx].name, ""));
				const SurfaceType				surfaceType		= s_textureConfigs[texCaseNdx].surfaceType;
				const int						numSamples		= s_textureConfigs[texCaseNdx].numSamples;

				for (int vecSize = 1; vecSize <= 4; vecSize++)
				{
					for (int precNdx = 0; precNdx < glu::PRECISION_LAST; precNdx++)
					{
						const glu::DataType		dataType		= vecSize > 1 ? glu::getDataTypeFloatVec(vecSize) : glu::TYPE_FLOAT;
						const glu::Precision	precision		= glu::Precision(precNdx);
						std::ostringstream		caseName;

						if (surfaceType != SURFACETYPE_FLOAT_FBO && precision == glu::PRECISION_LOWP)
							continue; // Skip as lowp doesn't actually produce any bits when rendered to U8 RT.

						caseName << glu::getDataTypeName(dataType) << "_" << glu::getPrecisionName(precision);

						caseGroup->addChild(new TextureDerivateCase(m_testCtx, caseName.str(), "", function, dataType, precision, surfaceType, numSamples));
					}
				}

				textureGroup->addChild(caseGroup.release());
			}

			functionGroup->addChild(textureGroup.release());
		}

		addChild(functionGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createDerivateTests (tcu::TestContext& testCtx)
{
	return new ShaderDerivateTests(testCtx);
}

} // sr
} // vkt
