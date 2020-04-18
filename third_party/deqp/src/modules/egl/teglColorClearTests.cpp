/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief Color clear tests.
 *//*--------------------------------------------------------------------*/

#include "teglColorClearTests.hpp"
#include "teglColorClearCase.hpp"
#include "eglwEnums.hpp"

using std::string;
using std::vector;

namespace deqp
{
namespace egl
{

using namespace eglw;

ColorClearTests::ColorClearTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "color_clears", "Color clears with different client APIs")
{
}

ColorClearTests::~ColorClearTests (void)
{
}

struct ColorClearGroupSpec
{
	const char*			name;
	const char*			desc;
	EGLint				apiBits;
	eglu::ConfigFilter	baseFilter;
	int					numContextsPerApi;
};

template <class ClearClass>
static void createColorClearGroups (EglTestContext& eglTestCtx, tcu::TestCaseGroup* group, const ColorClearGroupSpec* first, const ColorClearGroupSpec* last)
{
	for (const ColorClearGroupSpec* groupIter = first; groupIter != last; groupIter++)
	{
		tcu::TestCaseGroup* configGroup = new tcu::TestCaseGroup(eglTestCtx.getTestContext(), groupIter->name, groupIter->desc);
		group->addChild(configGroup);

		vector<RenderFilterList>	filterLists;
		eglu::FilterList			baseFilters;
		baseFilters << groupIter->baseFilter;
		getDefaultRenderFilterLists(filterLists, baseFilters);

		for (vector<RenderFilterList>::const_iterator listIter = filterLists.begin(); listIter != filterLists.end(); listIter++)
			configGroup->addChild(new ClearClass(eglTestCtx, listIter->getName(), "", groupIter->apiBits, listIter->getSurfaceTypeMask(), *listIter, groupIter->numContextsPerApi));
	}
}

template <deUint32 Bits>
static bool renderable (const eglu::CandidateConfig& c)
{
	return (c.renderableType() & Bits) == Bits;
}

void ColorClearTests::init (void)
{
#define CASE(NAME, DESC, BITS, NUMCFG) { NAME, DESC, BITS, renderable<BITS>, NUMCFG }

	static const ColorClearGroupSpec singleContextCases[] =
	{
		CASE("gles1",			"Color clears using GLES1",											EGL_OPENGL_ES_BIT,										1),
		CASE("gles2",			"Color clears using GLES2",											EGL_OPENGL_ES2_BIT,										1),
		CASE("gles3",			"Color clears using GLES3",											EGL_OPENGL_ES3_BIT,										1),
		CASE("vg",				"Color clears using OpenVG",										EGL_OPENVG_BIT,											1)
	};

	static const ColorClearGroupSpec multiContextCases[] =
	{
		CASE("gles1",				"Color clears using multiple GLES1 contexts to shared surface",		EGL_OPENGL_ES_BIT,											3),
		CASE("gles2",				"Color clears using multiple GLES2 contexts to shared surface",		EGL_OPENGL_ES2_BIT,											3),
		CASE("gles3",				"Color clears using multiple GLES3 contexts to shared surface",		EGL_OPENGL_ES3_BIT,											3),
		CASE("vg",					"Color clears using multiple OpenVG contexts to shared surface",	EGL_OPENVG_BIT,												3),
		CASE("gles1_gles2",			"Color clears using multiple APIs to shared surface",				EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT,						1),
		CASE("gles1_gles2_gles3",	"Color clears using multiple APIs to shared surface",				EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT,	1),
		CASE("gles1_vg",			"Color clears using multiple APIs to shared surface",				EGL_OPENGL_ES_BIT|EGL_OPENVG_BIT,							1),
		CASE("gles2_vg",			"Color clears using multiple APIs to shared surface",				EGL_OPENGL_ES2_BIT|EGL_OPENVG_BIT,							1),
		CASE("gles3_vg",			"Color clears using multiple APIs to shared surface",				EGL_OPENGL_ES3_BIT|EGL_OPENVG_BIT,							1),
		CASE("gles1_gles2_vg",		"Color clears using multiple APIs to shared surface",				EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT|EGL_OPENVG_BIT,		1)
	};

#undef CASE

	tcu::TestCaseGroup* singleContextGroup = new tcu::TestCaseGroup(m_testCtx, "single_context", "Single-context color clears");
	addChild(singleContextGroup);
	createColorClearGroups<SingleThreadColorClearCase>(m_eglTestCtx, singleContextGroup, &singleContextCases[0], &singleContextCases[DE_LENGTH_OF_ARRAY(singleContextCases)]);

	tcu::TestCaseGroup* multiContextGroup = new tcu::TestCaseGroup(m_testCtx, "multi_context", "Multi-context color clears with shared surface");
	addChild(multiContextGroup);
	createColorClearGroups<SingleThreadColorClearCase>(m_eglTestCtx, multiContextGroup, &multiContextCases[0], &multiContextCases[DE_LENGTH_OF_ARRAY(multiContextCases)]);

	tcu::TestCaseGroup* multiThreadGroup = new tcu::TestCaseGroup(m_testCtx, "multi_thread", "Multi-thread color clears with shared surface");
	addChild(multiThreadGroup);
	createColorClearGroups<MultiThreadColorClearCase>(m_eglTestCtx, multiThreadGroup, &multiContextCases[0], &multiContextCases[DE_LENGTH_OF_ARRAY(multiContextCases)]);
}

} // egl
} // deqp
