/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Basic Compute Shader Tests.
 *//*--------------------------------------------------------------------*/

#include "es31fAtomicCounterTests.hpp"

#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluRenderContext.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "tcuTestLog.hpp"

#include "deStringUtil.hpp"
#include "deRandom.hpp"
#include "deMemory.h"

#include <vector>
#include <string>

using namespace glw;
using tcu::TestLog;

using std::vector;
using std::string;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

class AtomicCounterTest : public TestCase
{
public:
	enum Operation
	{
		OPERATION_INC = (1<<0),
		OPERATION_DEC = (1<<1),
		OPERATION_GET = (1<<2)
	};

	enum OffsetType
	{
		OFFSETTYPE_NONE = 0,
		OFFSETTYPE_BASIC,
		OFFSETTYPE_REVERSE,
		OFFSETTYPE_FIRST_AUTO,
		OFFSETTYPE_DEFAULT_AUTO,
		OFFSETTYPE_RESET_DEFAULT,
		OFFSETTYPE_INVALID,
		OFFSETTYPE_INVALID_OVERLAPPING,
		OFFSETTYPE_INVALID_DEFAULT
	};

	enum BindingType
	{
		BINDINGTYPE_BASIC = 0,
		BINDINGTYPE_INVALID,
		BINDINGTYPE_INVALID_DEFAULT
	};

	struct TestSpec
	{
		TestSpec (void)
			: atomicCounterCount	(0)
			, operations			((Operation)0)
			, callCount				(0)
			, useBranches			(false)
			, threadCount			(0)
			, offsetType			(OFFSETTYPE_NONE)
			, bindingType			(BINDINGTYPE_BASIC)
		{
		}

		int			atomicCounterCount;
		Operation	operations;
		int			callCount;
		bool		useBranches;
		int			threadCount;
		OffsetType	offsetType;
		BindingType	bindingType;
	};

						AtomicCounterTest		(Context& context, const char* name, const char* description, const TestSpec& spec);
						~AtomicCounterTest		(void);

	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);

private:
	const TestSpec		m_spec;

	bool				checkAndLogCounterValues	(TestLog& log, const vector<deUint32>& counters) const;
	bool				checkAndLogCallValues		(TestLog& log, const vector<deUint32>& increments, const vector<deUint32>& decrements, const vector<deUint32>& preGets, const vector<deUint32>& postGets, const vector<deUint32>& gets) const;
	void				splitBuffer					(const vector<deUint32>& buffer, vector<deUint32>& increments, vector<deUint32>& decrements, vector<deUint32>& preGets, vector<deUint32>& postGets, vector<deUint32>& gets) const;
	deUint32			getInitialValue				(void) const { return m_spec.callCount * m_spec.threadCount + 1; }

	static string		generateShaderSource		(const TestSpec& spec);
	static void			getCountersValues			(vector<deUint32>& counterValues, const vector<deUint32>& values, int ndx, int counterCount);
	static bool			checkRange					(TestLog& log, const vector<deUint32>& values, const vector<deUint32>& min, const vector<deUint32>& max);
	static bool			checkUniquenessAndLinearity	(TestLog& log, const vector<deUint32>& values);
	static bool			checkPath					(const vector<deUint32>& increments, const vector<deUint32>& decrements, int initialValue, const TestSpec& spec);

	int					getOperationCount			(void) const;

	AtomicCounterTest&	operator=					(const AtomicCounterTest&);
						AtomicCounterTest			(const AtomicCounterTest&);
};

int AtomicCounterTest::getOperationCount (void) const
{
	int count = 0;

	if (m_spec.operations & OPERATION_INC)
		count++;

	if (m_spec.operations & OPERATION_DEC)
		count++;

	if (m_spec.operations == OPERATION_GET)
		count++;
	else if (m_spec.operations & OPERATION_GET)
		count += 2;

	return count;
}

AtomicCounterTest::AtomicCounterTest (Context& context, const char* name, const char* description, const TestSpec& spec)
	: TestCase	(context, name, description)
	, m_spec	(spec)
{
}

AtomicCounterTest::~AtomicCounterTest (void)
{
}

void AtomicCounterTest::init (void)
{
}

void AtomicCounterTest::deinit (void)
{
}

string AtomicCounterTest::generateShaderSource (const TestSpec& spec)
{
	std::ostringstream src;

	src
		<<  "#version 310 es\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n";

	{
		bool wroteLayout = false;

		switch (spec.bindingType)
		{
			case BINDINGTYPE_INVALID_DEFAULT:
				src << "layout(binding=10000";
				wroteLayout = true;
				break;

			default:
				// Do nothing
				break;
		}

		switch (spec.offsetType)
		{
			case OFFSETTYPE_DEFAULT_AUTO:
				if (!wroteLayout)
					src << "layout(binding=1, ";
				else
					src << ", ";

				src << "offset=4";
				wroteLayout = true;
				break;

			case OFFSETTYPE_RESET_DEFAULT:
				DE_ASSERT(spec.atomicCounterCount > 2);

				if (!wroteLayout)
					src << "layout(binding=1, ";
				else
					src << ", ";

				src << "offset=" << (4 * spec.atomicCounterCount/2);
				wroteLayout = true;
				break;

			case OFFSETTYPE_INVALID_DEFAULT:
				if (!wroteLayout)
					src << "layout(binding=1, ";
				else
					src << ", ";

				src << "offset=1";
				wroteLayout = true;
				break;

			default:
				// Do nothing
				break;
		}

		if (wroteLayout)
			src << ") uniform atomic_uint;\n";
	}

	src
	<< "layout(binding = 1, std430) buffer Output {\n";

	if ((spec.operations & OPERATION_GET) != 0 && spec.operations != OPERATION_GET)
		src << "	uint preGet[" << spec.threadCount * spec.atomicCounterCount * spec.callCount << "];\n";

	if ((spec.operations & OPERATION_INC) != 0)
		src << "	uint increment[" << spec.threadCount * spec.atomicCounterCount * spec.callCount << "];\n";

	if ((spec.operations & OPERATION_DEC) != 0)
		src << "	uint decrement[" << spec.threadCount * spec.atomicCounterCount * spec.callCount << "];\n";

	if ((spec.operations & OPERATION_GET) != 0 && spec.operations != OPERATION_GET)
		src << "	uint postGet[" << spec.threadCount * spec.atomicCounterCount * spec.callCount << "];\n";

	if (spec.operations == OPERATION_GET)
		src << "	uint get[" << spec.threadCount * spec.atomicCounterCount * spec.callCount << "];\n";

	src << "} sb_in;\n\n";

	for (int counterNdx = 0; counterNdx < spec.atomicCounterCount; counterNdx++)
	{
		bool layoutStarted = false;

		if (spec.offsetType == OFFSETTYPE_RESET_DEFAULT && counterNdx == spec.atomicCounterCount/2)
			src << "layout(binding=1, offset=0) uniform atomic_uint;\n";

		switch (spec.bindingType)
		{
			case BINDINGTYPE_BASIC:
				layoutStarted = true;
				src << "layout(binding=1";
				break;

			case BINDINGTYPE_INVALID:
				layoutStarted = true;
				src << "layout(binding=10000";
				break;

			case BINDINGTYPE_INVALID_DEFAULT:
				// Nothing
				break;

			default:
				DE_ASSERT(false);
		}

		switch (spec.offsetType)
		{
			case OFFSETTYPE_NONE:
				if (layoutStarted)
					src << ") ";

				src << "uniform atomic_uint counter" << counterNdx << ";\n";

				break;

			case OFFSETTYPE_BASIC:
				if (!layoutStarted)
					src << "layout(";
				else
					src << ", ";

				src << "offset=" << (counterNdx * 4) << ") uniform atomic_uint counter" << counterNdx << ";\n";

				break;

			case OFFSETTYPE_INVALID_DEFAULT:
				if (layoutStarted)
					src << ") ";

				src << "uniform atomic_uint counter" << counterNdx << ";\n";

				break;

			case OFFSETTYPE_INVALID:
				if (!layoutStarted)
					src << "layout(";
				else
					src << ", ";

				src << "offset=" << (1 + counterNdx * 2) << ") uniform atomic_uint counter" << counterNdx << ";\n";

				break;

			case OFFSETTYPE_INVALID_OVERLAPPING:
				if (!layoutStarted)
					src << "layout(";
				else
					src << ", ";

				src << "offset=0) uniform atomic_uint counter" << counterNdx << ";\n";

				break;

			case OFFSETTYPE_REVERSE:
				if (!layoutStarted)
					src << "layout(";
				else
					src << ", ";

				src << "offset=" << (spec.atomicCounterCount - counterNdx - 1) * 4 << ") uniform atomic_uint counter" << (spec.atomicCounterCount - counterNdx - 1) << ";\n";

				break;

			case OFFSETTYPE_FIRST_AUTO:
				DE_ASSERT(spec.atomicCounterCount > 2);

				if (counterNdx + 1 == spec.atomicCounterCount)
				{
					if (!layoutStarted)
						src << "layout(";
					else
						src << ", ";

					src << "offset=0) uniform atomic_uint counter0;\n";
				}
				else if (counterNdx == 0)
				{
					if (!layoutStarted)
						src << "layout(";
					else
						src << ", ";

					src << "offset=4) uniform atomic_uint counter1;\n";
				}
				else
				{
					if (layoutStarted)
						src << ") ";

					src << "uniform atomic_uint counter" << (counterNdx + 1) << ";\n";
				}

				break;

			case OFFSETTYPE_DEFAULT_AUTO:
				if (counterNdx + 1 == spec.atomicCounterCount)
				{
					if (!layoutStarted)
						src << "layout(";
					else
						src << ", ";

					src << "offset=0) uniform atomic_uint counter0;\n";
				}
				else
				{
					if (layoutStarted)
						src << ") ";

					src << "uniform atomic_uint counter" << (counterNdx + 1) << ";\n";
				}

				break;

			case OFFSETTYPE_RESET_DEFAULT:
				if (layoutStarted)
					src << ") ";

				if (counterNdx < spec.atomicCounterCount/2)
					src << "uniform atomic_uint counter" << (counterNdx + spec.atomicCounterCount/2) << ";\n";
				else
					src << "uniform atomic_uint counter" << (counterNdx - spec.atomicCounterCount/2) << ";\n";

				break;

			default:
				DE_ASSERT(false);
		}
	}

	src
	<< "\n"
	<< "void main (void)\n"
	<< "{\n";

	if (spec.callCount > 1)
		src << "\tfor (uint i = 0u; i < " << spec.callCount << "u; i++)\n";

	src
	<< "\t{\n"
	<< "\t\tuint id = (gl_GlobalInvocationID.x";

	if (spec.callCount > 1)
		src << " * "<< spec.callCount << "u";

	if (spec.callCount > 1)
		src << " + i)";
	else
		src << ")";

	if  (spec.atomicCounterCount > 1)
		src << " * " << spec.atomicCounterCount << "u";

	src << ";\n";

	for (int counterNdx = 0; counterNdx < spec.atomicCounterCount; counterNdx++)
	{
		if ((spec.operations & OPERATION_GET) != 0 && spec.operations != OPERATION_GET)
			src << "\t\tsb_in.preGet[id + " << counterNdx << "u] = atomicCounter(counter" << counterNdx << ");\n";

		if (spec.useBranches && ((spec.operations & (OPERATION_INC|OPERATION_DEC)) == (OPERATION_INC|OPERATION_DEC)))
		{
			src
			<< "\t\tif (((gl_GlobalInvocationID.x" << (spec.callCount > 1 ? " + i" : "") << ") % 2u) == 0u)\n"
			<< "\t\t{\n"
			<< "\t\t\tsb_in.increment[id + " << counterNdx << "u] = atomicCounterIncrement(counter" << counterNdx << ");\n"
			<< "\t\t\tsb_in.decrement[id + " << counterNdx << "u] = uint(-1);\n"
			<< "\t\t}\n"
			<< "\t\telse\n"
			<< "\t\t{\n"
			<< "\t\t\tsb_in.decrement[id + " << counterNdx << "u] = atomicCounterDecrement(counter" << counterNdx << ") + 1u;\n"
			<< "\t\t\tsb_in.increment[id + " << counterNdx << "u] = uint(-1);\n"
			<< "\t\t}\n";
		}
		else
		{
			if ((spec.operations & OPERATION_INC) != 0)
			{
				if (spec.useBranches)
				{
					src
					<< "\t\tif (((gl_GlobalInvocationID.x" << (spec.callCount > 1 ? " + i" : "") << ") % 2u) == 0u)\n"
					<< "\t\t{\n"
					<< "\t\t\tsb_in.increment[id + " << counterNdx << "u] = atomicCounterIncrement(counter" << counterNdx << ");\n"
					<< "\t\t}\n"
					<< "\t\telse\n"
					<< "\t\t{\n"
					<< "\t\t\tsb_in.increment[id + " << counterNdx << "u] = uint(-1);\n"
					<< "\t\t}\n";

				}
				else
					src << "\t\tsb_in.increment[id + " << counterNdx << "u] = atomicCounterIncrement(counter" << counterNdx << ");\n";
			}

			if ((spec.operations & OPERATION_DEC) != 0)
			{
				if (spec.useBranches)
				{
					src
					<< "\t\tif (((gl_GlobalInvocationID.x" << (spec.callCount > 1 ? " + i" : "") << ") % 2u) == 0u)\n"
					<< "\t\t{\n"
					<< "\t\t\tsb_in.decrement[id + " << counterNdx << "u] = atomicCounterDecrement(counter" << counterNdx << ") + 1u;\n"
					<< "\t\t}\n"
					<< "\t\telse\n"
					<< "\t\t{\n"
					<< "\t\t\tsb_in.decrement[id + " << counterNdx << "u] = uint(-1);\n"
					<< "\t\t}\n";

				}
				else
					src << "\t\tsb_in.decrement[id + " << counterNdx << "u] = atomicCounterDecrement(counter" << counterNdx << ") + 1u;\n";
			}
		}

		if ((spec.operations & OPERATION_GET) != 0 && spec.operations != OPERATION_GET)
			src << "\t\tsb_in.postGet[id + " << counterNdx << "u] = atomicCounter(counter" << counterNdx << ");\n";

		if ((spec.operations == OPERATION_GET) != 0)
		{
			if (spec.useBranches)
			{
				src
				<< "\t\tif (((gl_GlobalInvocationID.x" << (spec.callCount > 1 ? " + i" : "") << ") % 2u) == 0u)\n"
				<< "\t\t{\n"
				<< "\t\t\tsb_in.get[id + " << counterNdx << "u] = atomicCounter(counter" << counterNdx << ");\n"
				<< "\t\t}\n"
				<< "\t\telse\n"
				<< "\t\t{\n"
				<< "\t\t\tsb_in.get[id + " << counterNdx << "u] = uint(-1);\n"
				<< "\t\t}\n";
			}
			else
				src << "\t\tsb_in.get[id + " << counterNdx << "u] = atomicCounter(counter" << counterNdx << ");\n";
		}
	}

	src
	<< "\t}\n"
	<< "}\n";

	return src.str();
}

bool AtomicCounterTest::checkAndLogCounterValues (TestLog& log, const vector<deUint32>& counters) const
{
	tcu::ScopedLogSection	counterSection	(log, "Counter info", "Show initial value, current value and expected value of each counter.");
	bool					isOk			= true;

	// Check that atomic counters have sensible results
	for (int counterNdx = 0; counterNdx < (int)counters.size(); counterNdx++)
	{
		const deUint32	value			= counters[counterNdx];
		const deUint32	initialValue	= getInitialValue();
		deUint32		expectedValue	= (deUint32)-1;

		if ((m_spec.operations & OPERATION_INC) != 0 && (m_spec.operations & OPERATION_DEC) == 0)
			expectedValue = initialValue + (m_spec.useBranches ? m_spec.threadCount*m_spec.callCount - m_spec.threadCount*m_spec.callCount/2 : m_spec.threadCount*m_spec.callCount);

		if ((m_spec.operations & OPERATION_INC) == 0 && (m_spec.operations & OPERATION_DEC) != 0)
			expectedValue = initialValue - (m_spec.useBranches ? m_spec.threadCount*m_spec.callCount - m_spec.threadCount*m_spec.callCount/2 : m_spec.threadCount*m_spec.callCount);

		if ((m_spec.operations & OPERATION_INC) != 0 && (m_spec.operations & OPERATION_DEC) != 0)
			expectedValue = initialValue + (m_spec.useBranches ? m_spec.threadCount*m_spec.callCount - m_spec.threadCount*m_spec.callCount/2 : 0) - (m_spec.useBranches ? m_spec.threadCount*m_spec.callCount/2 : 0);

		if ((m_spec.operations & OPERATION_INC) == 0 && (m_spec.operations & OPERATION_DEC) == 0)
			expectedValue = initialValue;

		log << TestLog::Message << "atomic_uint counter" << counterNdx << " initial value: " << initialValue << ", value: " << value << ", expected: " << expectedValue << (value == expectedValue ? "" : ", failed!") << TestLog::EndMessage;

		if (value != expectedValue)
			isOk = false;
	}

	return isOk;
}

void AtomicCounterTest::splitBuffer (const vector<deUint32>& buffer, vector<deUint32>& increments, vector<deUint32>& decrements, vector<deUint32>& preGets, vector<deUint32>& postGets, vector<deUint32>& gets) const
{
	const int bufferValueCount	= m_spec.callCount * m_spec.threadCount * m_spec.atomicCounterCount;

	int firstPreGet				= -1;
	int firstPostGet			= -1;
	int	firstGet				= -1;
	int firstInc				= -1;
	int firstDec				= -1;

	increments.clear();
	decrements.clear();
	preGets.clear();
	postGets.clear();
	gets.clear();

	if (m_spec.operations == OPERATION_GET)
		firstGet = 0;
	else if (m_spec.operations == OPERATION_INC)
		firstInc = 0;
	else if (m_spec.operations == OPERATION_DEC)
		firstDec = 0;
	else if (m_spec.operations == (OPERATION_GET|OPERATION_INC))
	{
		firstPreGet		= 0;
		firstInc		= bufferValueCount;
		firstPostGet	= bufferValueCount * 2;
	}
	else if (m_spec.operations == (OPERATION_GET|OPERATION_DEC))
	{
		firstPreGet		= 0;
		firstDec		= bufferValueCount;
		firstPostGet	= bufferValueCount * 2;
	}
	else if (m_spec.operations == (OPERATION_GET|OPERATION_DEC|OPERATION_INC))
	{
		firstPreGet		= 0;
		firstInc		= bufferValueCount;
		firstDec		= bufferValueCount * 2;
		firstPostGet	= bufferValueCount * 3;
	}
	else if (m_spec.operations == (OPERATION_DEC|OPERATION_INC))
	{
		firstInc		= 0;
		firstDec		= bufferValueCount;
	}
	else
		DE_ASSERT(false);

	for (int threadNdx = 0; threadNdx < m_spec.threadCount; threadNdx++)
	{
		for (int callNdx = 0; callNdx < m_spec.callCount; callNdx++)
		{
			for (int counterNdx = 0; counterNdx < m_spec.atomicCounterCount; counterNdx++)
			{
				const int id = ((threadNdx * m_spec.callCount) + callNdx) * m_spec.atomicCounterCount + counterNdx;

				if (firstInc != -1)
					increments.push_back(buffer[firstInc + id]);

				if (firstDec != -1)
					decrements.push_back(buffer[firstDec + id]);

				if (firstPreGet != -1)
					preGets.push_back(buffer[firstPreGet + id]);

				if (firstPostGet != -1)
					postGets.push_back(buffer[firstPostGet + id]);

				if (firstGet != -1)
					gets.push_back(buffer[firstGet + id]);
			}
		}
	}
}

void AtomicCounterTest::getCountersValues (vector<deUint32>& counterValues, const vector<deUint32>& values, int ndx, int counterCount)
{
	counterValues.resize(values.size()/counterCount, 0);

	DE_ASSERT(values.size() % counterCount == 0);

	for (int valueNdx = 0; valueNdx < (int)counterValues.size(); valueNdx++)
		counterValues[valueNdx] = values[valueNdx * counterCount + ndx];
}

bool AtomicCounterTest::checkRange (TestLog& log, const vector<deUint32>& values, const vector<deUint32>& min, const vector<deUint32>& max)
{
	int failedCount = 0;

	DE_ASSERT(values.size() == min.size());
	DE_ASSERT(values.size() == max.size());

	for (int valueNdx = 0; valueNdx < (int)values.size(); valueNdx++)
	{
		if (values[valueNdx] != (deUint32)-1)
		{
			if (!deInRange32(values[valueNdx], min[valueNdx], max[valueNdx]))
			{
				if (failedCount < 20)
					log << TestLog::Message << "Value " << values[valueNdx] << " not in range [" << min[valueNdx] << ", " << max[valueNdx] << "]." << TestLog::EndMessage;
				failedCount++;
			}
		}
	}

	if (failedCount > 20)
		log << TestLog::Message << "Number of values not in range: " << failedCount << ", displaying first 20 values." << TestLog::EndMessage;

	return failedCount == 0;
}

bool AtomicCounterTest::checkUniquenessAndLinearity (TestLog& log, const vector<deUint32>& values)
{
	vector<deUint32>	counts;
	int					failedCount	= 0;
	deUint32			minValue	= (deUint32)-1;
	deUint32			maxValue	= 0;

	DE_ASSERT(!values.empty());

	for (int valueNdx = 0; valueNdx < (int)values.size(); valueNdx++)
	{
		if (values[valueNdx] != (deUint32)-1)
		{
			minValue = std::min(minValue, values[valueNdx]);
			maxValue = std::max(maxValue, values[valueNdx]);
		}
	}

	counts.resize(maxValue - minValue + 1, 0);

	for (int valueNdx = 0; valueNdx < (int)values.size(); valueNdx++)
	{
		if (values[valueNdx] != (deUint32)-1)
			counts[values[valueNdx] - minValue]++;
	}

	for (int countNdx = 0; countNdx < (int)counts.size(); countNdx++)
	{
		if (counts[countNdx] != 1)
		{
			if (failedCount < 20)
				log << TestLog::Message << "Value " << (minValue + countNdx) << " is not unique. Returned " << counts[countNdx] << " times." << TestLog::EndMessage;

			failedCount++;
		}
	}

	if (failedCount > 20)
		log << TestLog::Message << "Number of values not unique: " << failedCount << ", displaying first 20 values." << TestLog::EndMessage;

	return failedCount == 0;
}

bool AtomicCounterTest::checkPath (const vector<deUint32>& increments, const vector<deUint32>& decrements, int initialValue, const TestSpec& spec)
{
	const deUint32		lastValue	= initialValue + (spec.useBranches ? spec.threadCount*spec.callCount - spec.threadCount*spec.callCount/2 : 0) - (spec.useBranches ? spec.threadCount*spec.callCount/2 : 0);
	bool				isOk		= true;

	vector<deUint32>	incrementCounts;
	vector<deUint32>	decrementCounts;

	deUint32			minValue = 0xFFFFFFFFu;
	deUint32			maxValue = 0;

	for (int valueNdx = 0; valueNdx < (int)increments.size(); valueNdx++)
	{
		if (increments[valueNdx] != (deUint32)-1)
		{
			minValue = std::min(minValue, increments[valueNdx]);
			maxValue = std::max(maxValue, increments[valueNdx]);
		}
	}

	for (int valueNdx = 0; valueNdx < (int)decrements.size(); valueNdx++)
	{
		if (decrements[valueNdx] != (deUint32)-1)
		{
			minValue = std::min(minValue, decrements[valueNdx]);
			maxValue = std::max(maxValue, decrements[valueNdx]);
		}
	}

	minValue = std::min(minValue, (deUint32)initialValue);
	maxValue = std::max(maxValue, (deUint32)initialValue);

	incrementCounts.resize(maxValue - minValue + 1, 0);
	decrementCounts.resize(maxValue - minValue + 1, 0);

	for (int valueNdx = 0; valueNdx < (int)increments.size(); valueNdx++)
	{
		if (increments[valueNdx] != (deUint32)-1)
			incrementCounts[increments[valueNdx] - minValue]++;
	}

	for (int valueNdx = 0; valueNdx < (int)decrements.size(); valueNdx++)
	{
		if (decrements[valueNdx] != (deUint32)-1)
			decrementCounts[decrements[valueNdx] - minValue]++;
	}

	int pos = initialValue - minValue;

	while (incrementCounts[pos] + decrementCounts[pos] != 0)
	{
		if (incrementCounts[pos] > 0 && pos >= (int)(lastValue - minValue))
		{
			// If can increment and incrementation would move us away from result value, increment
			incrementCounts[pos]--;
			pos++;
		}
		else if (decrementCounts[pos] > 0)
		{
			// If can, decrement
			decrementCounts[pos]--;
			pos--;
		}
		else if (incrementCounts[pos] > 0)
		{
			// If increment moves closer to result value and can't decrement, increment
			incrementCounts[pos]--;
			pos++;
		}
		else
			DE_ASSERT(false);

		if (pos < 0 || pos >= (int)incrementCounts.size())
			break;
	}

	if (minValue + pos != lastValue)
		isOk = false;

	for (int valueNdx = 0; valueNdx < (int)incrementCounts.size(); valueNdx++)
	{
		if (incrementCounts[valueNdx] != 0)
			isOk = false;
	}

	for (int valueNdx = 0; valueNdx < (int)decrementCounts.size(); valueNdx++)
	{
		if (decrementCounts[valueNdx] != 0)
			isOk = false;
	}

	return isOk;
}

bool AtomicCounterTest::checkAndLogCallValues (TestLog& log, const vector<deUint32>& increments, const vector<deUint32>& decrements, const vector<deUint32>& preGets, const vector<deUint32>& postGets, const vector<deUint32>& gets) const
{
	bool isOk = true;

	for (int counterNdx = 0; counterNdx < m_spec.atomicCounterCount; counterNdx++)
	{
		vector<deUint32> counterIncrements;
		vector<deUint32> counterDecrements;
		vector<deUint32> counterPreGets;
		vector<deUint32> counterPostGets;
		vector<deUint32> counterGets;

		getCountersValues(counterIncrements,	increments,	counterNdx, m_spec.atomicCounterCount);
		getCountersValues(counterDecrements,	decrements,	counterNdx, m_spec.atomicCounterCount);
		getCountersValues(counterPreGets,		preGets,	counterNdx, m_spec.atomicCounterCount);
		getCountersValues(counterPostGets,		postGets,	counterNdx, m_spec.atomicCounterCount);
		getCountersValues(counterGets,			gets,		counterNdx, m_spec.atomicCounterCount);

		if (m_spec.operations == OPERATION_GET)
		{
			tcu::ScopedLogSection valueCheck(log, ("counter" + de::toString(counterNdx) + " value check").c_str(), ("Check that counter" + de::toString(counterNdx) + " values haven't changed.").c_str());
			int changedValues = 0;

			for (int valueNdx = 0; valueNdx < (int)gets.size(); valueNdx++)
			{
				if ((!m_spec.useBranches || gets[valueNdx] != (deUint32)-1) && gets[valueNdx] != getInitialValue())
				{
					if (changedValues < 20)
						log << TestLog::Message << "atomicCounter(counter" << counterNdx << ") returned " << gets[valueNdx] << " expected " << getInitialValue() << TestLog::EndMessage;
					isOk = false;
					changedValues++;
				}
			}

			if (changedValues == 0)
				log << TestLog::Message << "All values returned by atomicCounter(counter" << counterNdx << ") match initial value " << getInitialValue() <<  "." << TestLog::EndMessage;
			else if (changedValues > 20)
				log << TestLog::Message << "Total number of invalid values returned by atomicCounter(counter" << counterNdx << ") " << changedValues << " displaying first 20 values." <<  TestLog::EndMessage;
		}
		else if ((m_spec.operations & (OPERATION_INC|OPERATION_DEC)) == (OPERATION_INC|OPERATION_DEC))
		{
			tcu::ScopedLogSection valueCheck(log, ("counter" + de::toString(counterNdx) + " path check").c_str(), ("Check that there is order in which counter" + de::toString(counterNdx) + " increments and decrements could have happened.").c_str());
			if (!checkPath(counterIncrements, counterDecrements, getInitialValue(), m_spec))
			{
				isOk = false;
				log << TestLog::Message << "No possible order of calls to atomicCounterIncrement(counter" << counterNdx << ") and atomicCounterDecrement(counter" << counterNdx << ") found." << TestLog::EndMessage;
			}
			else
				log << TestLog::Message << "Found possible order of calls to atomicCounterIncrement(counter" << counterNdx << ") and atomicCounterDecrement(counter" << counterNdx << ")." << TestLog::EndMessage;
		}
		else if ((m_spec.operations & OPERATION_INC) != 0)
		{
			{
				tcu::ScopedLogSection uniquenesCheck(log, ("counter" + de::toString(counterNdx) + " check uniqueness and linearity").c_str(), ("Check that counter" + de::toString(counterNdx) + " returned only unique and linear values.").c_str());

				if (!checkUniquenessAndLinearity(log, counterIncrements))
				{
					isOk = false;
					log << TestLog::Message << "atomicCounterIncrement(counter" << counterNdx << ") returned non unique values." << TestLog::EndMessage;
				}
				else
					log << TestLog::Message << "atomicCounterIncrement(counter" << counterNdx << ") returned only unique values." << TestLog::EndMessage;
			}

			if (isOk && ((m_spec.operations & OPERATION_GET) != 0))
			{
				tcu::ScopedLogSection uniquenesCheck(log, ("counter" + de::toString(counterNdx) + " check range").c_str(), ("Check that counter" + de::toString(counterNdx) + " returned only values values between previous and next atomicCounter(counter" + de::toString(counterNdx) + ").").c_str());

				if (!checkRange(log, counterIncrements, counterPreGets, counterPostGets))
				{
					isOk = false;
					log << TestLog::Message << "atomicCounterIncrement(counter" << counterNdx << ") returned value that is not between previous and next call to atomicCounter(counter" << counterNdx << ")." << TestLog::EndMessage;
				}
				else
					log << TestLog::Message << "atomicCounterIncrement(counter" << counterNdx << ") returned only values between previous and next call to atomicCounter(counter" << counterNdx << ")." << TestLog::EndMessage;
			}
		}
		else if ((m_spec.operations & OPERATION_DEC) != 0)
		{
			{
				tcu::ScopedLogSection uniquenesCheck(log, ("counter" + de::toString(counterNdx) + " check uniqueness and linearity").c_str(), ("Check that counter" + de::toString(counterNdx) + " returned only unique and linear values.").c_str());

				if (!checkUniquenessAndLinearity(log, counterDecrements))
				{
					isOk = false;
					log << TestLog::Message << "atomicCounterDecrement(counter" << counterNdx << ") returned non unique values." << TestLog::EndMessage;
				}
				else
					log << TestLog::Message << "atomicCounterDecrement(counter" << counterNdx << ") returned only unique values." << TestLog::EndMessage;
			}

			if (isOk && ((m_spec.operations & OPERATION_GET) != 0))
			{
				tcu::ScopedLogSection uniquenesCheck(log, ("counter" + de::toString(counterNdx) + " check range").c_str(), ("Check that counter" + de::toString(counterNdx) + " returned only values values between previous and next atomicCounter(counter" + de::toString(counterNdx) + ".").c_str());

				if (!checkRange(log, counterDecrements, counterPostGets, counterPreGets))
				{
					isOk = false;
					log << TestLog::Message << "atomicCounterDecrement(counter" << counterNdx << ") returned value that is not between previous and next call to atomicCounter(counter" << counterNdx << ")." << TestLog::EndMessage;
				}
				else
					log << TestLog::Message << "atomicCounterDecrement(counter" << counterNdx << ") returned only values between previous and next call to atomicCounter(counter" << counterNdx << ")." << TestLog::EndMessage;
			}
		}
	}

	return isOk;
}

TestCase::IterateResult AtomicCounterTest::iterate (void)
{
	const glw::Functions&		gl					= m_context.getRenderContext().getFunctions();
	TestLog&					log					= m_testCtx.getLog();
	const glu::Buffer			counterBuffer		(m_context.getRenderContext());
	const glu::Buffer			outputBuffer		(m_context.getRenderContext());
	const glu::ShaderProgram	program				(m_context.getRenderContext(), glu::ProgramSources() << glu::ShaderSource(glu::SHADERTYPE_COMPUTE, generateShaderSource(m_spec)));

	const deInt32				counterBufferSize	= m_spec.atomicCounterCount * 4;
	const deInt32				ssoSize				= m_spec.atomicCounterCount * m_spec.callCount * m_spec.threadCount * 4 * getOperationCount();

	log << program;

	if (m_spec.offsetType == OFFSETTYPE_INVALID || m_spec.offsetType == OFFSETTYPE_INVALID_DEFAULT || m_spec.bindingType == BINDINGTYPE_INVALID || m_spec.bindingType == BINDINGTYPE_INVALID_DEFAULT || m_spec.offsetType == OFFSETTYPE_INVALID_OVERLAPPING)
	{
		if (program.isOk())
		{
			log << TestLog::Message << "Expected program to fail, but compilation passed." << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Compile succeeded");
			return STOP;
		}
		else
		{
			log << TestLog::Message << "Compilation failed as expected." << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Compile failed");
			return STOP;
		}
	}
	else if (!program.isOk())
	{
		log << TestLog::Message << "Compile failed." << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Compile failed");
		return STOP;
	}

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram()");

	// Create output buffer
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, ssoSize, NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create output buffer");

	// Create atomic counter buffer
	{
		vector<deUint32> data(m_spec.atomicCounterCount, getInitialValue());
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *counterBuffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, counterBufferSize, &(data[0]), GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create buffer for atomic counters");
	}

	// Bind output buffer
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *outputBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup output buffer");

	// Bind atomic counter buffer
	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, *counterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup atomic counter buffer");

	// Dispath compute
	gl.dispatchCompute(m_spec.threadCount, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

	gl.finish();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFinish()");

	vector<deUint32> output(ssoSize/4, 0);
	vector<deUint32> counters(m_spec.atomicCounterCount, 0);

	// Read back output buffer
	{
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");

		void* ptr = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)(output.size() * sizeof(deUint32)), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");

		deMemcpy(&(output[0]), ptr, (int)output.size() * sizeof(deUint32));

		if (!gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER))
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer()");
			TCU_CHECK_MSG(false, "Mapped buffer corrupted");
		}

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");
	}

	// Read back counter buffer
	{
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *counterBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");

		void* ptr = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)(counters.size() * sizeof(deUint32)), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");

		deMemcpy(&(counters[0]), ptr, (int)counters.size() * sizeof(deUint32));

		if (!gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER))
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer()");
			TCU_CHECK_MSG(false, "Mapped buffer corrupted");
		}

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");
	}

	bool isOk = true;

	if (!checkAndLogCounterValues(log, counters))
		isOk = false;

	{
		vector<deUint32> increments;
		vector<deUint32> decrements;
		vector<deUint32> preGets;
		vector<deUint32> postGets;
		vector<deUint32> gets;

		splitBuffer(output, increments, decrements, preGets, postGets, gets);

		if (!checkAndLogCallValues(log, increments, decrements, preGets, postGets, gets))
			isOk = false;
	}

	if (isOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

string specToTestName (const AtomicCounterTest::TestSpec& spec)
{
	std::ostringstream stream;

	stream << spec.atomicCounterCount	<< (spec.atomicCounterCount == 1 ? "_counter" : "_counters");
	stream << "_" << spec.callCount		<< (spec.callCount == 1 ? "_call" : "_calls");
	stream << "_" << spec.threadCount	<< (spec.threadCount == 1 ? "_thread" : "_threads");

	return stream.str();
}

string specToTestDescription (const AtomicCounterTest::TestSpec& spec)
{
	std::ostringstream	stream;
	bool				firstOperation = 0;

	stream
	<< "Test ";

	if ((spec.operations & AtomicCounterTest::OPERATION_GET) != 0)
	{
		stream << "atomicCounter()";
		firstOperation = false;
	}

	if ((spec.operations & AtomicCounterTest::OPERATION_INC) != 0)
	{
		if (!firstOperation)
			stream << ", ";

		stream << " atomicCounterIncrement()";
		firstOperation = false;
	}

	if ((spec.operations & AtomicCounterTest::OPERATION_DEC) != 0)
	{
		if (!firstOperation)
			stream << ", ";

		stream << " atomicCounterDecrement()";
		firstOperation = false;
	}

	stream << " calls with ";

	if (spec.useBranches)
		stream << " branches, ";

	stream << spec.atomicCounterCount << " atomic counters, " << spec.callCount << " calls and " << spec.threadCount << " threads.";

	return stream.str();
}

string operationToName (const AtomicCounterTest::Operation& operations, bool useBranch)
{
	std::ostringstream	stream;
	bool				first = true;

	if ((operations & AtomicCounterTest::OPERATION_GET) != 0)
	{
		stream << "get";
		first = false;
	}

	if ((operations & AtomicCounterTest::OPERATION_INC) != 0)
	{
		if (!first)
			stream << "_";

		stream << "inc";
		first = false;
	}

	if ((operations & AtomicCounterTest::OPERATION_DEC) != 0)
	{
		if (!first)
			stream << "_";

		stream << "dec";
		first = false;
	}

	if (useBranch)
		stream << "_branch";

	return stream.str();
}

string operationToDescription (const AtomicCounterTest::Operation& operations, bool useBranch)
{
	std::ostringstream	stream;
	bool				firstOperation = 0;

	stream
	<< "Test ";

	if ((operations & AtomicCounterTest::OPERATION_GET) != 0)
	{
		stream << "atomicCounter()";
		firstOperation = false;
	}

	if ((operations & AtomicCounterTest::OPERATION_INC) != 0)
	{
		if (!firstOperation)
			stream << ", ";

		stream << " atomicCounterIncrement()";
		firstOperation = false;
	}

	if ((operations & AtomicCounterTest::OPERATION_DEC) != 0)
	{
		if (!firstOperation)
			stream << ", ";

		stream << " atomicCounterDecrement()";
		firstOperation = false;
	}


	if (useBranch)
		stream << " calls with branches.";
	else
		stream << ".";

	return stream.str();
}

string layoutTypesToName (const AtomicCounterTest::BindingType& bindingType, const AtomicCounterTest::OffsetType& offsetType)
{
	std::ostringstream	stream;

	switch (bindingType)
	{
		case AtomicCounterTest::BINDINGTYPE_BASIC:
			// Nothing
			break;

		case AtomicCounterTest::BINDINGTYPE_INVALID:
			stream << "invalid_binding";
			break;

		default:
			DE_ASSERT(false);
	}

	if (bindingType != AtomicCounterTest::BINDINGTYPE_BASIC && offsetType != AtomicCounterTest::OFFSETTYPE_NONE)
		stream << "_";

	switch (offsetType)
	{
		case AtomicCounterTest::OFFSETTYPE_BASIC:
			stream << "basic_offset";
			break;

		case AtomicCounterTest::OFFSETTYPE_REVERSE:
			stream << "reverse_offset";
			break;

		case AtomicCounterTest::OFFSETTYPE_INVALID:
			stream << "invalid_offset";
			break;

		case AtomicCounterTest::OFFSETTYPE_FIRST_AUTO:
			stream << "first_offset_set";
			break;

		case AtomicCounterTest::OFFSETTYPE_DEFAULT_AUTO:
			stream << "default_offset_set";
			break;

		case AtomicCounterTest::OFFSETTYPE_RESET_DEFAULT:
			stream << "reset_default_offset";
			break;

		case AtomicCounterTest::OFFSETTYPE_NONE:
			// Do nothing
			break;

		default:
			DE_ASSERT(false);
	}

	return stream.str();
}

string layoutTypesToDesc (const AtomicCounterTest::BindingType& bindingType, const AtomicCounterTest::OffsetType& offsetType)
{
	std::ostringstream	stream;

	switch (bindingType)
	{
		case AtomicCounterTest::BINDINGTYPE_BASIC:
			stream << "Test using atomic counters with explicit layout bindings and";
			break;

		case AtomicCounterTest::BINDINGTYPE_INVALID:
			stream << "Test using atomic counters with invalid explicit layout bindings and";
			break;

		case AtomicCounterTest::BINDINGTYPE_INVALID_DEFAULT:
			stream << "Test using atomic counters with invalid default layout binding and";
			break;

		default:
			DE_ASSERT(false);
	}

	switch (offsetType)
	{
		case AtomicCounterTest::OFFSETTYPE_NONE:
			stream << " no explicit offsets.";
			break;

		case AtomicCounterTest::OFFSETTYPE_BASIC:
			stream << "explicit continuos offsets.";
			break;

		case AtomicCounterTest::OFFSETTYPE_REVERSE:
			stream << "reversed explicit offsets.";
			break;

		case AtomicCounterTest::OFFSETTYPE_INVALID:
			stream << "invalid explicit offsets.";
			break;

		case AtomicCounterTest::OFFSETTYPE_FIRST_AUTO:
			stream << "only first counter with explicit offset.";
			break;

		case AtomicCounterTest::OFFSETTYPE_DEFAULT_AUTO:
			stream << "default offset.";
			break;

		case AtomicCounterTest::OFFSETTYPE_RESET_DEFAULT:
			stream << "default offset specified twice.";
			break;

		default:
			DE_ASSERT(false);
	}

	return stream.str();
}

} // Anonymous

AtomicCounterTests::AtomicCounterTests (Context& context)
	: TestCaseGroup(context, "atomic_counter", "Atomic counter tests")
{
	// Runtime use tests
	{
		const int counterCounts[] =
		{
			1, 4, 8
		};

		const int callCounts[] =
		{
			1, 5, 100
		};

		const int threadCounts[] =
		{
			1, 10, 5000
		};

		const AtomicCounterTest::Operation operations[] =
		{
			AtomicCounterTest::OPERATION_GET,
			AtomicCounterTest::OPERATION_INC,
			AtomicCounterTest::OPERATION_DEC,

			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_INC|AtomicCounterTest::OPERATION_GET),
			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_DEC|AtomicCounterTest::OPERATION_GET),

			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_INC|AtomicCounterTest::OPERATION_DEC),
			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_INC|AtomicCounterTest::OPERATION_DEC|AtomicCounterTest::OPERATION_GET)
		};

		for (int operationNdx = 0; operationNdx < DE_LENGTH_OF_ARRAY(operations); operationNdx++)
		{
			const AtomicCounterTest::Operation operation = operations[operationNdx];

			for (int branch = 0; branch < 2; branch++)
			{
				const bool useBranch = (branch == 1);

				TestCaseGroup* operationGroup = new TestCaseGroup(m_context, operationToName(operation, useBranch).c_str(), operationToDescription(operation, useBranch).c_str());

				for (int counterCountNdx = 0; counterCountNdx < DE_LENGTH_OF_ARRAY(counterCounts); counterCountNdx++)
				{
					const int counterCount = counterCounts[counterCountNdx];

					for (int callCountNdx = 0; callCountNdx < DE_LENGTH_OF_ARRAY(callCounts); callCountNdx++)
					{
						const int callCount = callCounts[callCountNdx];

						for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
						{
							const int threadCount = threadCounts[threadCountNdx];

							if (threadCount * callCount * counterCount > 10000)
								continue;

							if (useBranch && threadCount * callCount == 1)
								continue;

							AtomicCounterTest::TestSpec spec;

							spec.atomicCounterCount = counterCount;
							spec.operations			= operation;
							spec.callCount			= callCount;
							spec.useBranches		= useBranch;
							spec.threadCount		= threadCount;
							spec.bindingType		= AtomicCounterTest::BINDINGTYPE_BASIC;
							spec.offsetType			= AtomicCounterTest::OFFSETTYPE_NONE;

							operationGroup->addChild(new AtomicCounterTest(m_context, specToTestName(spec).c_str(), specToTestDescription(spec).c_str(), spec));
						}
					}
				}

				addChild(operationGroup);
			}
		}
	}

	{
		TestCaseGroup* layoutGroup = new TestCaseGroup(m_context, "layout", "Layout qualifier tests.");

		const int counterCounts[]	= { 1, 8 };
		const int callCounts[]		= { 1, 5 };
		const int threadCounts[]	= { 1, 1000 };

		const AtomicCounterTest::Operation operations[] =
		{
			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_INC|AtomicCounterTest::OPERATION_GET),
			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_DEC|AtomicCounterTest::OPERATION_GET),
			(AtomicCounterTest::Operation)(AtomicCounterTest::OPERATION_INC|AtomicCounterTest::OPERATION_DEC)
		};

		const AtomicCounterTest::OffsetType offsetTypes[] =
		{
			AtomicCounterTest::OFFSETTYPE_REVERSE,
			AtomicCounterTest::OFFSETTYPE_FIRST_AUTO,
			AtomicCounterTest::OFFSETTYPE_DEFAULT_AUTO,
			AtomicCounterTest::OFFSETTYPE_RESET_DEFAULT
		};

		for (int offsetTypeNdx = 0; offsetTypeNdx < DE_LENGTH_OF_ARRAY(offsetTypes); offsetTypeNdx++)
		{
			const AtomicCounterTest::OffsetType offsetType = offsetTypes[offsetTypeNdx];

			TestCaseGroup* layoutQualifierGroup = new TestCaseGroup(m_context, layoutTypesToName(AtomicCounterTest::BINDINGTYPE_BASIC, offsetType).c_str(), layoutTypesToDesc(AtomicCounterTest::BINDINGTYPE_BASIC, offsetType).c_str());

			for (int operationNdx = 0; operationNdx < DE_LENGTH_OF_ARRAY(operations); operationNdx++)
			{
				const AtomicCounterTest::Operation operation = operations[operationNdx];

				TestCaseGroup* operationGroup = new TestCaseGroup(m_context, operationToName(operation, false).c_str(), operationToDescription(operation, false).c_str());

				for (int counterCountNdx = 0; counterCountNdx < DE_LENGTH_OF_ARRAY(counterCounts); counterCountNdx++)
				{
					const int counterCount = counterCounts[counterCountNdx];

					if (offsetType == AtomicCounterTest::OFFSETTYPE_FIRST_AUTO && counterCount < 3)
						continue;

					if (offsetType == AtomicCounterTest::OFFSETTYPE_DEFAULT_AUTO && counterCount < 2)
						continue;

					if (offsetType == AtomicCounterTest::OFFSETTYPE_RESET_DEFAULT && counterCount < 2)
						continue;

					if (offsetType == AtomicCounterTest::OFFSETTYPE_REVERSE && counterCount < 2)
						continue;

					for (int callCountNdx = 0; callCountNdx < DE_LENGTH_OF_ARRAY(callCounts); callCountNdx++)
					{
						const int callCount = callCounts[callCountNdx];

						for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
						{
							const int threadCount = threadCounts[threadCountNdx];

							AtomicCounterTest::TestSpec spec;

							spec.atomicCounterCount = counterCount;
							spec.operations			= operation;
							spec.callCount			= callCount;
							spec.useBranches		= false;
							spec.threadCount		= threadCount;
							spec.bindingType		= AtomicCounterTest::BINDINGTYPE_BASIC;
							spec.offsetType			= offsetType;

							operationGroup->addChild(new AtomicCounterTest(m_context, specToTestName(spec).c_str(), specToTestDescription(spec).c_str(), spec));
						}
					}
				}
				layoutQualifierGroup->addChild(operationGroup);
			}
			layoutGroup->addChild(layoutQualifierGroup);
		}

		{
			TestCaseGroup* invalidGroup = new TestCaseGroup(m_context, "invalid", "Test invalid layouts");

			{
				AtomicCounterTest::TestSpec spec;

				spec.atomicCounterCount = 1;
				spec.operations			= AtomicCounterTest::OPERATION_INC;
				spec.callCount			= 1;
				spec.useBranches		= false;
				spec.threadCount		= 1;
				spec.bindingType		= AtomicCounterTest::BINDINGTYPE_INVALID;
				spec.offsetType			= AtomicCounterTest::OFFSETTYPE_NONE;

				invalidGroup->addChild(new AtomicCounterTest(m_context, "invalid_binding", "Test layout qualifiers with invalid binding.", spec));
			}

			{
				AtomicCounterTest::TestSpec spec;

				spec.atomicCounterCount = 1;
				spec.operations			= AtomicCounterTest::OPERATION_INC;
				spec.callCount			= 1;
				spec.useBranches		= false;
				spec.threadCount		= 1;
				spec.bindingType		= AtomicCounterTest::BINDINGTYPE_INVALID_DEFAULT;
				spec.offsetType			= AtomicCounterTest::OFFSETTYPE_NONE;

				invalidGroup->addChild(new AtomicCounterTest(m_context, "invalid_default_binding", "Test layout qualifiers with invalid default binding.", spec));
			}

			{
				AtomicCounterTest::TestSpec spec;

				spec.atomicCounterCount = 1;
				spec.operations			= AtomicCounterTest::OPERATION_INC;
				spec.callCount			= 1;
				spec.useBranches		= false;
				spec.threadCount		= 1;
				spec.bindingType		= AtomicCounterTest::BINDINGTYPE_BASIC;
				spec.offsetType			= AtomicCounterTest::OFFSETTYPE_INVALID;

				invalidGroup->addChild(new AtomicCounterTest(m_context, "invalid_offset_align", "Test layout qualifiers with invalid alignment offset.", spec));
			}

			{
				AtomicCounterTest::TestSpec spec;

				spec.atomicCounterCount = 2;
				spec.operations			= AtomicCounterTest::OPERATION_INC;
				spec.callCount			= 1;
				spec.useBranches		= false;
				spec.threadCount		= 1;
				spec.bindingType		= AtomicCounterTest::BINDINGTYPE_BASIC;
				spec.offsetType			= AtomicCounterTest::OFFSETTYPE_INVALID_OVERLAPPING;

				invalidGroup->addChild(new AtomicCounterTest(m_context, "invalid_offset_overlap", "Test layout qualifiers with invalid overlapping offset.", spec));
			}

			{
				AtomicCounterTest::TestSpec spec;

				spec.atomicCounterCount = 1;
				spec.operations			= AtomicCounterTest::OPERATION_INC;
				spec.callCount			= 1;
				spec.useBranches		= false;
				spec.threadCount		= 1;
				spec.bindingType		= AtomicCounterTest::BINDINGTYPE_BASIC;
				spec.offsetType			= AtomicCounterTest::OFFSETTYPE_INVALID_DEFAULT;

				invalidGroup->addChild(new AtomicCounterTest(m_context, "invalid_default_offset", "Test layout qualifiers with invalid default offset.", spec));
			}

			layoutGroup->addChild(invalidGroup);
		}

		addChild(layoutGroup);
	}
}

AtomicCounterTests::~AtomicCounterTests (void)
{
}

void AtomicCounterTests::init (void)
{
}

} // Functional
} // gles31
} // deqp
