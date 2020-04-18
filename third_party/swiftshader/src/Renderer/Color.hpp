// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef sw_Color_hpp
#define sw_Color_hpp

#include "Common/Types.hpp"
#include "Common/Math.hpp"

namespace sw
{
	template<class T>
	struct Color
	{
		Color();
	
		Color(const Color<byte> &c);
		Color(const Color<short> &c);
		Color(const Color<float> &c);
		
		Color(int c);
		Color(unsigned short c);
		Color(unsigned long c);
		Color(unsigned int c);
		
		Color(T r, T g, T b, T a = 1);

		operator unsigned int() const;

		T &operator[](int i);
		const T &operator[](int i) const;

		Color<T> operator+() const;
		Color<T> operator-() const;

		Color<T>& operator=(const Color<T>& c);

		Color<T> &operator+=(const Color<T> &c);
		Color<T> &operator*=(float l);

		static Color<T> gradient(const Color<T> &c1, const Color<T>  &c2, float d);
		static Color<T> shade(const Color<T> &c1, const Color<T>  &c2, float d);

		template<class S>
		friend Color<S> operator+(const Color<S> &c1, const Color<S> &c2);
		template<class S>
		friend Color<S> operator-(const Color<S> &c1, const Color<S> &c2);

		template<class S>
		friend Color<S> operator*(float l, const Color<S> &c);
		template<class S>
		friend Color<S> operator*(const Color<S> &c1, const Color<S> &c2);
		template<class S>
		friend Color<S> operator/(const Color<S> &c, float l);

		T r;
		T g;
		T b;
		T a;
	};
}

#include "Common/Math.hpp"

namespace sw
{
	template<class T>
	inline Color<T>::Color()
	{
	}

	template<>
	inline Color<byte>::Color(const Color<byte> &c)
	{
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
	}

	template<>
	inline Color<byte>::Color(const Color<short> &c)
	{
		r = clamp(c.r >> 4, 0, 255);
		g = clamp(c.g >> 4, 0, 255);
		b = clamp(c.b >> 4, 0, 255);
		a = clamp(c.a >> 4, 0, 255);
	}

	template<>
	inline Color<byte>::Color(const Color<float> &c)
	{
		r = ifloor(clamp(c.r * 256.0f, 0.0f, 255.0f));
		g = ifloor(clamp(c.g * 256.0f, 0.0f, 255.0f));
		b = ifloor(clamp(c.b * 256.0f, 0.0f, 255.0f));
		a = ifloor(clamp(c.a * 256.0f, 0.0f, 255.0f));
	}

	template<>
	inline Color<short>::Color(const Color<short> &c)
	{
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
	}

	template<>
	inline Color<short>::Color(const Color<byte> &c)
	{
		r = c.r << 4;
		g = c.g << 4;
		b = c.b << 4;
		a = c.a << 4;
	}

	template<>
	inline Color<float>::Color(const Color<float> &c)
	{
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
	}

	template<>
	inline Color<short>::Color(const Color<float> &c)
	{
		r = iround(clamp(c.r * 4095.0f, -4096.0f, 4095.0f));
		g = iround(clamp(c.g * 4095.0f, -4096.0f, 4095.0f));
		b = iround(clamp(c.b * 4095.0f, -4096.0f, 4095.0f));
		a = iround(clamp(c.a * 4095.0f, -4096.0f, 4095.0f));
	}

	template<>
	inline Color<float>::Color(const Color<byte> &c)
	{
		r = c.r / 255.0f;
		g = c.g / 255.0f;
		b = c.b / 255.0f;
		a = c.a / 255.0f;
	}

	template<>
	inline Color<float>::Color(const Color<short> &c)
	{
		r = c.r / 4095.0f;
		g = c.g / 4095.0f;
		b = c.b / 4095.0f;
		a = c.a / 4095.0f;
	}

	template<>
	inline Color<float>::Color(unsigned short c)
	{
		r = (float)(c & 0xF800) / (float)0xF800;
		g = (float)(c & 0x07E0) / (float)0x07E0;
		b = (float)(c & 0x001F) / (float)0x001F;
		a = 1;
	}

	template<>
	inline Color<short>::Color(unsigned short c)
	{
		// 4.12 fixed-point format
		r = ((c & 0xF800) >> 4) + ((c & 0xF800) >> 9) + ((c & 0xF800) >> 14);
		g = ((c & 0x07E0) << 1) + ((c & 0x07E0) >> 5);
		b = ((c & 0x001F) << 7) + ((c & 0x001F) << 2) + ((c & 0x001F) >> 3);
		a = 0x1000;
	}

	template<>
	inline Color<byte>::Color(unsigned short c)
	{
		r = (byte)(((c & 0xF800) >> 8) + ((c & 0xE000) >> 13));
		g = (byte)(((c & 0x07E0) >> 3) + ((c & 0x0600) >> 9));
		b = (byte)(((c & 0x001F) << 3) + ((c & 0x001C) >> 2));
		a = 0xFF;
	}

	template<>
	inline Color<float>::Color(int c)
	{
		const float d = 1.0f / 255.0f;

		r = (float)((c & 0x00FF0000) >> 16) * d;
		g = (float)((c & 0x0000FF00) >> 8) * d;
		b = (float)((c & 0x000000FF) >> 0) * d;
		a = (float)((c & 0xFF000000) >> 24) * d;
	}

	template<>
	inline Color<short>::Color(int c)
	{
		// 4.12 fixed-point format
		r = (short)((c & 0x00FF0000) >> 12);
		g = (short)((c & 0x0000FF00) >> 4);
		b = (short)((c & 0x000000FF) << 4);
		a = (short)((c & 0xFF000000) >> 20);
	}

	template<>
	inline Color<byte>::Color(int c)
	{
		r = (byte)((c & 0x00FF0000) >> 16);
		g = (byte)((c & 0x0000FF00) >> 8);
		b = (byte)((c & 0x000000FF) >> 0);
		a = (byte)((c & 0xFF000000) >> 24);
	}

	template<>
	inline Color<float>::Color(unsigned int c)
	{
		const float d = 1.0f / 255.0f;

		r = (float)((c & 0x00FF0000) >> 16) * d;
		g = (float)((c & 0x0000FF00) >> 8) * d;
		b = (float)((c & 0x000000FF) >> 0) * d;
		a = (float)((c & 0xFF000000) >> 24) * d;
	}

	template<>
	inline Color<short>::Color(unsigned int c)
	{
		// 4.12 fixed-point format
		r = (short)((c & 0x00FF0000) >> 12);
		g = (short)((c & 0x0000FF00) >> 4);
		b = (short)((c & 0x000000FF) << 4);
		a = (short)((c & 0xFF000000) >> 20);
	}

	template<>
	inline Color<byte>::Color(unsigned int c)
	{
		r = (byte)((c & 0x00FF0000) >> 16);
		g = (byte)((c & 0x0000FF00) >> 8);
		b = (byte)((c & 0x000000FF) >> 0);
		a = (byte)((c & 0xFF000000) >> 24);
	}

	template<>
	inline Color<float>::Color(unsigned long c)
	{
		const float d = 1.0f / 255.0f;

		r = (float)((c & 0x00FF0000) >> 16) * d;
		g = (float)((c & 0x0000FF00) >> 8) * d;
		b = (float)((c & 0x000000FF) >> 0) * d;
		a = (float)((c & 0xFF000000) >> 24) * d;
	}

	template<>
	inline Color<short>::Color(unsigned long c)
	{
		// 4.12 fixed-point format
		r = (short)((c & 0x00FF0000) >> 12);
		g = (short)((c & 0x0000FF00) >> 4);
		b = (short)((c & 0x000000FF) << 4);
		a = (short)((c & 0xFF000000) >> 20);
	}

	template<>
	inline Color<byte>::Color(unsigned long c)
	{
		r = (byte)((c & 0x00FF0000) >> 16);
		g = (byte)((c & 0x0000FF00) >> 8);
		b = (byte)((c & 0x000000FF) >> 0);
		a = (byte)((c & 0xFF000000) >> 24);
	}

	template<class T>
	inline Color<T>::Color(T r_, T g_, T b_, T a_)
	{
		r = r_;
		g = g_;
		b = b_;
		a = a_;
	}

	template<>
	inline Color<float>::operator unsigned int() const
	{
		return ((unsigned int)min(b * 255.0f, 255.0f) << 0) |
		       ((unsigned int)min(g * 255.0f, 255.0f) << 8) |
		       ((unsigned int)min(r * 255.0f, 255.0f) << 16) |
		       ((unsigned int)min(a * 255.0f, 255.0f) << 24);
	}

	template<>
	inline Color<short>::operator unsigned int() const
	{
		return ((unsigned int)min(b >> 4, 255) << 0) |
		       ((unsigned int)min(g >> 4, 255) << 8) |
		       ((unsigned int)min(r >> 4, 255) << 16) |
		       ((unsigned int)min(a >> 4, 255) << 24);
	}

	template<>
	inline Color<byte>::operator unsigned int() const
	{
		return (b << 0) +
		       (g << 8) +
		       (r << 16) +
			   (a << 24);
	}

	template<class T>
	inline T &Color<T>::operator[](int i)
	{
		return (&r)[i];
	}

	template<class T>
	inline const T &Color<T>::operator[](int i) const
	{
		return (&r)[i];
	}

	template<class T>
	inline Color<T> Color<T>::operator+() const
	{
		return *this;
	}

	template<class T>
	inline Color<T> Color<T>::operator-() const
	{
		return Color(-r, -g, -b, -a);
	}

	template<class T>
	inline Color<T> &Color<T>::operator=(const Color& c)
	{
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;

		return *this;
	}

	template<class T>
	inline Color<T> &Color<T>::operator+=(const Color &c)
	{
		r += c.r;
		g += c.g;
		b += c.b;
		a += c.a;

		return *this;
	}

	template<class T>
	inline Color<T> &Color<T>::operator*=(float l)
	{
		*this = l * *this;

		return *this;
	}

	template<class T>
	inline Color<T> operator+(const Color<T> &c1, const Color<T> &c2)
	{
		return Color<T>(c1.r + c2.r,
		                c1.g + c2.g,
		                c1.b + c2.b,
		                c1.a + c2.a);	
	}

	template<class T>
	inline Color<T> operator-(const Color<T> &c1, const Color<T> &c2)
	{
		return Color<T>(c1.r - c2.r,
		                c1.g - c2.g,
		                c1.b - c2.b,
		                c1.a - c2.a);	
	}

	template<class T>
	inline Color<T> operator*(float l, const Color<T> &c)
	{
		T r = (T)(l * c.r);
		T g = (T)(l * c.g);
		T b = (T)(l * c.b);
		T a = (T)(l * c.a);

		return Color<T>(r, g, b, a);
	}

	template<class T>
	inline Color<T> operator*(const Color<T> &c1, const Color<T> &c2)
	{
		T r = c1.r * c2.r;
		T g = c1.g * c2.g;
		T b = c1.b * c2.b;
		T a = c1.a * c2.a;

		return Color<T>(r, g, b, a);
	}

	template<>
	inline Color<short> operator*(const Color<short> &c1, const Color<short> &c2)
	{
		short r = c1.r * c2.r >> 12;
		short g = c1.g * c2.g >> 12;
		short b = c1.b * c2.b >> 12;
		short a = c1.a * c2.a >> 12;

		return Color<short>(r, g, b, a);
	}

	template<>
	inline Color<byte> operator*(const Color<byte> &c1, const Color<byte> &c2)
	{
		byte r = c1.r * c2.r >> 8;
		byte g = c1.g * c2.g >> 8;
		byte b = c1.b * c2.b >> 8;
		byte a = c1.a * c2.a >> 8;

		return Color<byte>(r, g, b, a);
	}

	template<class T>
	inline Color<T> operator/(const Color<T> &c, float l)
	{
		l = 1.0f / l; 

		T r = (T)(l * c.r);
		T g = (T)(l * c.g);
		T b = (T)(l * c.b);
		T a = (T)(l * c.a);

		return Color<T>(r, g, b, a);
	}

	template<class T>
	inline Color<T> Color<T>::gradient(const Color<T> &c1, const Color<T> &c2, float d)
	{
		d = 1.0f / d; 

		T r = (c2.r - c1.r) * d;
		T g = (c2.g - c1.g) * d;
		T b = (c2.b - c1.b) * d;
		T a = (c2.a - c1.a) * d;

		return Color<T>(r, g, b, a);
	}

	template<class T>
	inline Color<T> Color<T>::shade(const Color<T> &c1, const Color<T>  &c2, float d)
	{
		T r = c1.r + (T)(d * (c2.r - c1.r));
		T g = c1.g + (T)(d * (c2.g - c1.g));
		T b = c1.b + (T)(d * (c2.b - c1.b));
		T a = c1.a + (T)(d * (c2.a - c1.a));

		return Color<T>(r, g, b, a);
	}
}

#endif   // sw_Color_hpp
