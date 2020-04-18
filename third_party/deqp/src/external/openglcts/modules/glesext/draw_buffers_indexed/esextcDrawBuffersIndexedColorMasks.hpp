#ifndef _ESEXTCDRAWBUFFERSINDEXEDCOLORMASKS_HPP
#define _ESEXTCDRAWBUFFERSINDEXEDCOLORMASKS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file  esextcDrawBuffersIndexedColorMasks.hpp
 * \brief Draw Buffers Indexed tests 4. Color masks
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedBase.hpp"
#include "tcuSurface.hpp"

namespace glcts
{

/** 4. Color masks
 **/
class DrawBuffersIndexedColorMasks : public DrawBuffersIndexedBase
{
public:
	/** Public methods
	 **/
	DrawBuffersIndexedColorMasks(Context& context, const ExtParameters& extParams, const char* name,
								 const char* description);
	virtual ~DrawBuffersIndexedColorMasks()
	{
	}

private:
	/** Private methods
	 **/
	virtual IterateResult iterate();

	void		 prepareFramebuffer(void);
	void		 releaseFramebuffer(void);
	unsigned int NumComponents(glw::GLenum format);
	glw::GLenum ReadableType(glw::GLenum format);
	tcu::RGBA GetEpsilon();
	bool VerifyImg(const tcu::TextureLevel& textureLevel, tcu::RGBA expectedColor, tcu::RGBA epsilon);

	glw::GLuint m_fbo;
};

} // namespace glcts

#endif // _ESEXTCDRAWBUFFERSINDEXEDCOLORMASKS_HPP
