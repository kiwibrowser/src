#ifndef _SGLRREFERENCEUTILS_HPP
#define _SGLRREFERENCEUTILS_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES Utilities
 * ------------------------------------------------
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
 * \brief Reference context utils
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "rrVertexAttrib.hpp"
#include "rrPrimitiveTypes.hpp"
#include "rrShaders.hpp"
#include "rrRenderState.hpp"
#include "gluRenderContext.hpp"

namespace sglr
{
namespace rr_util
{

rr::VertexAttribType			mapGLPureIntegerVertexAttributeType		(deUint32 type);
rr::VertexAttribType			mapGLFloatVertexAttributeType			(deUint32 type, bool normalizedInteger, int size, glu::ContextType ctxType);
int								mapGLSize								(int size);
rr::PrimitiveType				mapGLPrimitiveType						(deUint32 type);
rr::IndexType					mapGLIndexType							(deUint32 type);
rr::GeometryShaderOutputType	mapGLGeometryShaderOutputType			(deUint32 primitive);
rr::GeometryShaderInputType		mapGLGeometryShaderInputType			(deUint32 primitive);
rr::TestFunc					mapGLTestFunc							(deUint32 func);
rr::StencilOp					mapGLStencilOp							(deUint32 op);
rr::BlendEquation				mapGLBlendEquation						(deUint32 equation);
rr::BlendEquationAdvanced		mapGLBlendEquationAdvanced				(deUint32 equation);
rr::BlendFunc					mapGLBlendFunc							(deUint32 func);

} // rr_util
} // sglr

#endif // _SGLRREFERENCEUTILS_HPP
