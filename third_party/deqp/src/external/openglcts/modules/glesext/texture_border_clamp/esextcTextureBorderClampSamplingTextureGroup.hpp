#ifndef _ESEXTCTEXTUREBORDERCLAMPSAMPLINGTEXTUREGROUP_HPP
#define _ESEXTCTEXTUREBORDERCLAMPSAMPLINGTEXTUREGROUP_HPP
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

/*!
 * \file esextcTextureBorderClampSamplingTextureGroup.hpp
 * \brief Test Group for Texture Border Clamp Sampling Texture Tests (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "glcTestCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
/* Base Test Group for texture_border_clamp tests */
class TextureBorderClampSamplingTextureGroup : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TextureBorderClampSamplingTextureGroup(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TextureBorderClampSamplingTextureGroup(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	TextureBorderClampSamplingTextureGroup(const TextureBorderClampSamplingTextureGroup& other);
	TextureBorderClampSamplingTextureGroup& operator=(const TextureBorderClampSamplingTextureGroup& other);
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPSAMPLINGTEXTUREGROUP_HPP
