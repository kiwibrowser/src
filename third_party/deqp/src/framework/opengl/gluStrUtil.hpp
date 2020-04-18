#ifndef _GLUSTRUTIL_HPP
#define _GLUSTRUTIL_HPP
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
 * \brief OpenGL value to string utilities.
 *//*--------------------------------------------------------------------*/

#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "tcuFormatUtil.hpp"

namespace glu
{

// Internal format utilities.
namespace detail
{

class EnumPointerFmt
{
public:
	typedef const char* (*GetEnumNameFunc) (int value);

	const deUint32* const	value;
	const deUint32			size;
	const GetEnumNameFunc	getName;

	EnumPointerFmt (const deUint32* value_, deUint32 size_, GetEnumNameFunc getName_) : value(value_), size(size_), getName(getName_) {}
};

class BooleanPointerFmt
{
public:
	const deUint8* const	value;
	const deUint32			size;

	BooleanPointerFmt (const deUint8* value_, deUint32 size_) : value(value_), size(size_) {}
};

class TextureUnitStr
{
public:
	const deUint32 texUnit;
	TextureUnitStr (deUint32 texUnit_) : texUnit(texUnit_) {}
};

class TextureParameterValueStr
{
public:
	const deUint32	param;
	const int		value;
	TextureParameterValueStr (deUint32 param_, int value_) : param(param_), value(value_) {}
};

std::ostream&		operator<<		(std::ostream& str, const TextureUnitStr& unitStr);
std::ostream&		operator<<		(std::ostream& str, const TextureParameterValueStr& valueStr);
std::ostream&		operator<<		(std::ostream& str, const BooleanPointerFmt& fmt);
std::ostream&		operator<<		(std::ostream& str, const EnumPointerFmt& fmt);

} // detail

inline detail::EnumPointerFmt getEnumPointerStr (const deUint32* value, deInt32 size, detail::EnumPointerFmt::GetEnumNameFunc getName)
{
	return detail::EnumPointerFmt(value, (deUint32)de::max(0, size), getName);
}

inline detail::BooleanPointerFmt getBooleanPointerStr (const deUint8* value, deInt32 size)
{
	return detail::BooleanPointerFmt(value, (deUint32)de::max(0, size));
}

inline detail::TextureUnitStr						getTextureUnitStr			(deUint32 unit) { return detail::TextureUnitStr(unit); }
inline detail::TextureParameterValueStr				getTextureParameterValueStr	(deUint32 param, int value) { return detail::TextureParameterValueStr(param, value); }
detail::EnumPointerFmt								getInvalidateAttachmentStr	(const deUint32* attachments, int numAttachments);

std::ostream&										operator<<					(std::ostream& str, ApiType apiType);
std::ostream&										operator<<					(std::ostream& str, ContextType contextType);

// prevent implicit conversions from bool to int.
//
// While it is well-defined that (int)true == GL_TRUE and (int)false == GL_FALSE,
// using these functions to convert non-GL-types suggests a that the calling code is
// mixing and matching GLboolean and bool types which may not be safe.
//
// \note return value is void to prevent compilation. Otherwise this would only break linking.
void	getBooleanPointerStr	(const bool* value, deInt32 size);	// delete
void	getBooleanStr			(bool);								// delete
void	getBooleanName			(bool);								// delete

#include "gluStrUtilPrototypes.inl"

} // glu

#endif // _GLUSTRUTIL_HPP
