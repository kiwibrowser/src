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

#include "Reactor.hpp"

#include "gtest/gtest.h"

using namespace sw;

int reference(int *p, int y)
{
	int x = p[-1];
	int z = 4;

	for(int i = 0; i < 10; i++)
	{
		z += (2 << i) - (i / 3);
	}

	int sum = x + y + z;

	return sum;
}

TEST(SubzeroReactorTest, Sample)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Int>, Int)> function;
		{
			Pointer<Int> p = function.Arg<0>();
			Int x = p[-1];
			Int y = function.Arg<1>();
			Int z = 4;

			For(Int i = 0, i < 10, i++)
			{
				z += (2 << i) - (i / 3);
			}

			Float4 v;
			v.z = As<Float>(z);
			z = As<Int>(Float(Float4(v.xzxx).y));

			Int sum = x + y + z;

			Return(sum);
		}

		routine = function(L"one");

		if(routine)
		{
			int (*callable)(int*, int) = (int(*)(int*,int))routine->getEntry();
			int one[2] = {1, 0};
			int result = callable(&one[1], 2);
			EXPECT_EQ(result, reference(&one[1], 2));
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, Uninitialized)
{
	Routine *routine = nullptr;

	{
		Function<Int()> function;
		{
			Int a;
			Int z = 4;
			Int q;
			Int c;
			Int p;
			Bool b;

			q += q;

			If(b)
			{
				c = p;
			}

			Return(a + z + q + c);
		}

		routine = function(L"one");

		if(routine)
		{
			int (*callable)() = (int(*)())routine->getEntry();
			int result = callable();
			EXPECT_EQ(result, result);   // Anything is fine, just don't crash
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, SubVectorLoadStore)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>, Pointer<Byte>)> function;
		{
			Pointer<Byte> in = function.Arg<0>();
			Pointer<Byte> out = function.Arg<1>();

			*Pointer<Int4>(out + 16 * 0)   = *Pointer<Int4>(in + 16 * 0);
			*Pointer<Short4>(out + 16 * 1) = *Pointer<Short4>(in + 16 * 1);
			*Pointer<Byte8>(out + 16 * 2)  = *Pointer<Byte8>(in + 16 * 2);
			*Pointer<Byte4>(out + 16 * 3)  = *Pointer<Byte4>(in + 16 * 3);
			*Pointer<Short2>(out + 16 * 4) = *Pointer<Short2>(in + 16 * 4);

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			int8_t in[16 * 5] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
			                     17, 18, 19, 20, 21, 22, 23, 24,  0,  0,  0,  0,  0,  0,  0,  0,
			                     25, 26, 27, 28, 29, 30, 31, 32,  0,  0,  0,  0,  0,  0,  0,  0,
			                     33, 34, 35, 36,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
			                     37, 38, 39, 40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

			int8_t out[16 * 5] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

			int (*callable)(void*, void*) = (int(*)(void*,void*))routine->getEntry();
			callable(in, out);

			for(int row = 0; row < 5; row++)
			{
				for(int col = 0; col < 16; col++)
				{
					int i = row * 16 + col;

					if(in[i] ==  0)
					{
						EXPECT_EQ(out[i], -1) << "Row " << row << " column " << col <<  " not left untouched.";
					}
					else
					{
						EXPECT_EQ(out[i], in[i]) << "Row " << row << " column " << col << " not equal to input.";
					}
				}
			}
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, VectorConstant)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Int4>(out + 16 * 0) = Int4(0x04030201, 0x08070605, 0x0C0B0A09, 0x100F0E0D);
			*Pointer<Short4>(out + 16 * 1) = Short4(0x1211, 0x1413, 0x1615, 0x1817);
			*Pointer<Byte8>(out + 16 * 2) = Byte8(0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20);
			*Pointer<Int2>(out + 16 * 3) = Int2(0x24232221, 0x28272625);

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			int8_t out[16 * 4] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

			int8_t exp[16 * 4] = {1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
			                      17, 18, 19, 20, 21, 22, 23, 24, -1, -1, -1, -1, -1, -1, -1, -1,
			                      25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1, -1, -1, -1,
			                      33, 34, 35, 36, 37, 38, 39, 40, -1, -1, -1, -1, -1, -1, -1, -1};

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(out);

			for(int row = 0; row < 4; row++)
			{
				for(int col = 0; col < 16; col++)
				{
					int i = row * 16 + col;

					EXPECT_EQ(out[i], exp[i]);
				}
			}
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, Concatenate)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Int4>(out + 16 * 0)   = Int4(Int2(0x04030201, 0x08070605), Int2(0x0C0B0A09, 0x100F0E0D));
			*Pointer<Short8>(out + 16 * 1) = Short8(Short4(0x0201, 0x0403, 0x0605, 0x0807), Short4(0x0A09, 0x0C0B, 0x0E0D, 0x100F));

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			int8_t ref[16 * 5] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
			                      1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16};

			int8_t out[16 * 5] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			                      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

			int (*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(out);

			for(int row = 0; row < 2; row++)
			{
				for(int col = 0; col < 16; col++)
				{
					int i = row * 16 + col;

					EXPECT_EQ(out[i], ref[i]) << "Row " << row << " column " << col << " not equal to reference.";
				}
			}
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, Swizzle)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			for(int i = 0; i < 256; i++)
			{
				*Pointer<Float4>(out + 16 * i) = Swizzle(Float4(1.0f, 2.0f, 3.0f, 4.0f), i);
			}

			for(int i = 0; i < 256; i++)
			{
				*Pointer<Float4>(out + 16 * (256 + i)) = ShuffleLowHigh(Float4(1.0f, 2.0f, 3.0f, 4.0f), Float4(5.0f, 6.0f, 7.0f, 8.0f), i);
			}

			*Pointer<Float4>(out + 16 * (512 + 0)) = UnpackLow(Float4(1.0f, 2.0f, 3.0f, 4.0f), Float4(5.0f, 6.0f, 7.0f, 8.0f));
			*Pointer<Float4>(out + 16 * (512 + 1)) = UnpackHigh(Float4(1.0f, 2.0f, 3.0f, 4.0f), Float4(5.0f, 6.0f, 7.0f, 8.0f));
			*Pointer<Int2>(out + 16 * (512 + 2)) = UnpackLow(Short4(1, 2, 3, 4), Short4(5, 6, 7, 8));
			*Pointer<Int2>(out + 16 * (512 + 3)) = UnpackHigh(Short4(1, 2, 3, 4), Short4(5, 6, 7, 8));
			*Pointer<Short4>(out + 16 * (512 + 4)) = UnpackLow(Byte8(1, 2, 3, 4, 5, 6, 7, 8), Byte8(9, 10, 11, 12, 13, 14, 15, 16));
			*Pointer<Short4>(out + 16 * (512 + 5)) = UnpackHigh(Byte8(1, 2, 3, 4, 5, 6, 7, 8), Byte8(9, 10, 11, 12, 13, 14, 15, 16));

			for(int i = 0; i < 256; i++)
			{
				*Pointer<Short4>(out + 16 * (512 + 6) + (8 * i)) =
                                    Swizzle(Short4(1, 2, 3, 4), i);
			}

			for(int i = 0; i < 256; i++)
			{
				*Pointer<Int4>(out + 16 * (512 + 6 + i) + (8 * 256)) =
                                    Swizzle(Int4(1, 2, 3, 4), i);
			}

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			struct
			{
				float f[256 + 256 + 2][4];
				int i[388][4];
			} out;

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			for(int i = 0; i < 256; i++)
			{
				EXPECT_EQ(out.f[i][0], float((i >> 0) & 0x03) + 1.0f);
				EXPECT_EQ(out.f[i][1], float((i >> 2) & 0x03) + 1.0f);
				EXPECT_EQ(out.f[i][2], float((i >> 4) & 0x03) + 1.0f);
				EXPECT_EQ(out.f[i][3], float((i >> 6) & 0x03) + 1.0f);
			}

			for(int i = 0; i < 256; i++)
			{
				EXPECT_EQ(out.f[256 + i][0], float((i >> 0) & 0x03) + 1.0f);
				EXPECT_EQ(out.f[256 + i][1], float((i >> 2) & 0x03) + 1.0f);
				EXPECT_EQ(out.f[256 + i][2], float((i >> 4) & 0x03) + 5.0f);
				EXPECT_EQ(out.f[256 + i][3], float((i >> 6) & 0x03) + 5.0f);
			}

			EXPECT_EQ(out.f[512 + 0][0], 1.0f);
			EXPECT_EQ(out.f[512 + 0][1], 5.0f);
			EXPECT_EQ(out.f[512 + 0][2], 2.0f);
			EXPECT_EQ(out.f[512 + 0][3], 6.0f);

			EXPECT_EQ(out.f[512 + 1][0], 3.0f);
			EXPECT_EQ(out.f[512 + 1][1], 7.0f);
			EXPECT_EQ(out.f[512 + 1][2], 4.0f);
			EXPECT_EQ(out.f[512 + 1][3], 8.0f);

			EXPECT_EQ(out.i[0][0], 0x00050001);
			EXPECT_EQ(out.i[0][1], 0x00060002);
			EXPECT_EQ(out.i[0][2], 0x00000000);
			EXPECT_EQ(out.i[0][3], 0x00000000);

			EXPECT_EQ(out.i[1][0], 0x00070003);
			EXPECT_EQ(out.i[1][1], 0x00080004);
			EXPECT_EQ(out.i[1][2], 0x00000000);
			EXPECT_EQ(out.i[1][3], 0x00000000);

			EXPECT_EQ(out.i[2][0], 0x0A020901);
			EXPECT_EQ(out.i[2][1], 0x0C040B03);
			EXPECT_EQ(out.i[2][2], 0x00000000);
			EXPECT_EQ(out.i[2][3], 0x00000000);

			EXPECT_EQ(out.i[3][0], 0x0E060D05);
			EXPECT_EQ(out.i[3][1], 0x10080F07);
			EXPECT_EQ(out.i[3][2], 0x00000000);
			EXPECT_EQ(out.i[3][3], 0x00000000);

			for(int i = 0; i < 256; i++)
			{
				EXPECT_EQ(out.i[4 + i/2][0 + (i%2) * 2] & 0xFFFF,
                                          ((i >> 0) & 0x03) + 1);
				EXPECT_EQ(out.i[4 + i/2][0 + (i%2) * 2] >> 16,
                                          ((i >> 2) & 0x03) + 1);
				EXPECT_EQ(out.i[4 + i/2][1 + (i%2) * 2] & 0xFFFF,
                                          ((i >> 4) & 0x03) + 1);
				EXPECT_EQ(out.i[4 + i/2][1 + (i%2) * 2] >> 16,
                                          ((i >> 6) & 0x03) + 1);
			}

			for(int i = 0; i < 256; i++)
			{
				EXPECT_EQ(out.i[132 + i][0], ((i >> 0) & 0x03) + 1);
				EXPECT_EQ(out.i[132 + i][1], ((i >> 2) & 0x03) + 1);
				EXPECT_EQ(out.i[132 + i][2], ((i >> 4) & 0x03) + 1);
				EXPECT_EQ(out.i[132 + i][3], ((i >> 6) & 0x03) + 1);
			}
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, Branching)
{
	Routine *routine = nullptr;

	{
		Function<Int(Void)> function;
		{
			Int x = 0;

			For(Int i = 0, i < 8, i++)
			{
				If(i < 2)
				{
					x += 1;
				}
				Else If(i < 4)
				{
					x += 10;
				}
				Else If(i < 6)
				{
					x += 100;
				}
				Else
				{
					x += 1000;
				}

				For(Int i = 0, i < 5, i++)
					x += 10000;
			}

			For(Int i = 0, i < 10, i++)
				for(int i = 0; i < 10; i++)
					For(Int i = 0, i < 10, i++)
					{
						x += 1000000;
					}

			For(Int i = 0, i < 2, i++)
				If(x == 1000402222)
				{
					If(x != 1000402222)
						x += 1000000000;
				}
				Else
					x = -5;

			Return(x);
		}

		routine = function(L"one");

		if(routine)
		{
			int(*callable)() = (int(*)())routine->getEntry();
			int result = callable();

			EXPECT_EQ(result, 1000402222);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, MinMax)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Float4>(out + 16 * 0) = Min(Float4(1.0f, 0.0f, -0.0f, +0.0f), Float4(0.0f, 1.0f, +0.0f, -0.0f));
			*Pointer<Float4>(out + 16 * 1) = Max(Float4(1.0f, 0.0f, -0.0f, +0.0f), Float4(0.0f, 1.0f, +0.0f, -0.0f));

			*Pointer<Int4>(out + 16 * 2) = Min(Int4(1, 0, -1, -0), Int4(0, 1, 0, +0));
			*Pointer<Int4>(out + 16 * 3) = Max(Int4(1, 0, -1, -0), Int4(0, 1, 0, +0));
			*Pointer<UInt4>(out + 16 * 4) = Min(UInt4(1, 0, -1, -0), UInt4(0, 1, 0, +0));
			*Pointer<UInt4>(out + 16 * 5) = Max(UInt4(1, 0, -1, -0), UInt4(0, 1, 0, +0));

			*Pointer<Short4>(out + 16 * 6) = Min(Short4(1, 0, -1, -0), Short4(0, 1, 0, +0));
			*Pointer<Short4>(out + 16 * 7) = Max(Short4(1, 0, -1, -0), Short4(0, 1, 0, +0));
			*Pointer<UShort4>(out + 16 * 8) = Min(UShort4(1, 0, -1, -0), UShort4(0, 1, 0, +0));
			*Pointer<UShort4>(out + 16 * 9) = Max(UShort4(1, 0, -1, -0), UShort4(0, 1, 0, +0));

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[10][4];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0x00000000u);
			EXPECT_EQ(out[0][1], 0x00000000u);
			EXPECT_EQ(out[0][2], 0x00000000u);
			EXPECT_EQ(out[0][3], 0x80000000u);

			EXPECT_EQ(out[1][0], 0x3F800000u);
			EXPECT_EQ(out[1][1], 0x3F800000u);
			EXPECT_EQ(out[1][2], 0x00000000u);
			EXPECT_EQ(out[1][3], 0x80000000u);

			EXPECT_EQ(out[2][0], 0x00000000u);
			EXPECT_EQ(out[2][1], 0x00000000u);
			EXPECT_EQ(out[2][2], 0xFFFFFFFFu);
			EXPECT_EQ(out[2][3], 0x00000000u);

			EXPECT_EQ(out[3][0], 0x00000001u);
			EXPECT_EQ(out[3][1], 0x00000001u);
			EXPECT_EQ(out[3][2], 0x00000000u);
			EXPECT_EQ(out[3][3], 0x00000000u);

			EXPECT_EQ(out[4][0], 0x00000000u);
			EXPECT_EQ(out[4][1], 0x00000000u);
			EXPECT_EQ(out[4][2], 0x00000000u);
			EXPECT_EQ(out[4][3], 0x00000000u);

			EXPECT_EQ(out[5][0], 0x00000001u);
			EXPECT_EQ(out[5][1], 0x00000001u);
			EXPECT_EQ(out[5][2], 0xFFFFFFFFu);
			EXPECT_EQ(out[5][3], 0x00000000u);

			EXPECT_EQ(out[6][0], 0x00000000u);
			EXPECT_EQ(out[6][1], 0x0000FFFFu);
			EXPECT_EQ(out[6][2], 0x00000000u);
			EXPECT_EQ(out[6][3], 0x00000000u);

			EXPECT_EQ(out[7][0], 0x00010001u);
			EXPECT_EQ(out[7][1], 0x00000000u);
			EXPECT_EQ(out[7][2], 0x00000000u);
			EXPECT_EQ(out[7][3], 0x00000000u);

			EXPECT_EQ(out[8][0], 0x00000000u);
			EXPECT_EQ(out[8][1], 0x00000000u);
			EXPECT_EQ(out[8][2], 0x00000000u);
			EXPECT_EQ(out[8][3], 0x00000000u);

			EXPECT_EQ(out[9][0], 0x00010001u);
			EXPECT_EQ(out[9][1], 0x0000FFFFu);
			EXPECT_EQ(out[9][2], 0x00000000u);
			EXPECT_EQ(out[9][3], 0x00000000u);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, NotNeg)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Int>(out + 16 * 0) = ~Int(0x55555555);
			*Pointer<Short>(out + 16 * 1) = ~Short(0x5555);
			*Pointer<Int4>(out + 16 * 2) = ~Int4(0x55555555, 0xAAAAAAAA, 0x00000000, 0xFFFFFFFF);
			*Pointer<Short4>(out + 16 * 3) = ~Short4(0x5555, 0xAAAA, 0x0000, 0xFFFF);

			*Pointer<Int>(out + 16 * 4) = -Int(0x55555555);
			*Pointer<Short>(out + 16 * 5) = -Short(0x5555);
			*Pointer<Int4>(out + 16 * 6) = -Int4(0x55555555, 0xAAAAAAAA, 0x00000000, 0xFFFFFFFF);
			*Pointer<Short4>(out + 16 * 7) = -Short4(0x5555, 0xAAAA, 0x0000, 0xFFFF);

			*Pointer<Float4>(out + 16 * 8) = -Float4(1.0f, -1.0f, 0.0f, -0.0f);

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[10][4];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0xAAAAAAAAu);
			EXPECT_EQ(out[0][1], 0x00000000u);
			EXPECT_EQ(out[0][2], 0x00000000u);
			EXPECT_EQ(out[0][3], 0x00000000u);

			EXPECT_EQ(out[1][0], 0x0000AAAAu);
			EXPECT_EQ(out[1][1], 0x00000000u);
			EXPECT_EQ(out[1][2], 0x00000000u);
			EXPECT_EQ(out[1][3], 0x00000000u);

			EXPECT_EQ(out[2][0], 0xAAAAAAAAu);
			EXPECT_EQ(out[2][1], 0x55555555u);
			EXPECT_EQ(out[2][2], 0xFFFFFFFFu);
			EXPECT_EQ(out[2][3], 0x00000000u);

			EXPECT_EQ(out[3][0], 0x5555AAAAu);
			EXPECT_EQ(out[3][1], 0x0000FFFFu);
			EXPECT_EQ(out[3][2], 0x00000000u);
			EXPECT_EQ(out[3][3], 0x00000000u);

			EXPECT_EQ(out[4][0], 0xAAAAAAABu);
			EXPECT_EQ(out[4][1], 0x00000000u);
			EXPECT_EQ(out[4][2], 0x00000000u);
			EXPECT_EQ(out[4][3], 0x00000000u);

			EXPECT_EQ(out[5][0], 0x0000AAABu);
			EXPECT_EQ(out[5][1], 0x00000000u);
			EXPECT_EQ(out[5][2], 0x00000000u);
			EXPECT_EQ(out[5][3], 0x00000000u);

			EXPECT_EQ(out[6][0], 0xAAAAAAABu);
			EXPECT_EQ(out[6][1], 0x55555556u);
			EXPECT_EQ(out[6][2], 0x00000000u);
			EXPECT_EQ(out[6][3], 0x00000001u);

			EXPECT_EQ(out[7][0], 0x5556AAABu);
			EXPECT_EQ(out[7][1], 0x00010000u);
			EXPECT_EQ(out[7][2], 0x00000000u);
			EXPECT_EQ(out[7][3], 0x00000000u);

			EXPECT_EQ(out[8][0], 0xBF800000u);
			EXPECT_EQ(out[8][1], 0x3F800000u);
			EXPECT_EQ(out[8][2], 0x80000000u);
			EXPECT_EQ(out[8][3], 0x00000000u);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, VectorCompare)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Int4>(out + 16 * 0) = CmpEQ(Float4(1.0f, 1.0f, -0.0f, +0.0f), Float4(0.0f, 1.0f, +0.0f, -0.0f));
			*Pointer<Int4>(out + 16 * 1) = CmpEQ(Int4(1, 0, -1, -0), Int4(0, 1, 0, +0));
			*Pointer<Byte8>(out + 16 * 2) = CmpEQ(SByte8(1, 2, 3, 4, 5, 6, 7, 8), SByte8(7, 6, 5, 4, 3, 2, 1, 0));

			*Pointer<Int4>(out + 16 * 3) = CmpNLT(Float4(1.0f, 1.0f, -0.0f, +0.0f), Float4(0.0f, 1.0f, +0.0f, -0.0f));
			*Pointer<Int4>(out + 16 * 4) = CmpNLT(Int4(1, 0, -1, -0), Int4(0, 1, 0, +0));
			*Pointer<Byte8>(out + 16 * 5) = CmpGT(SByte8(1, 2, 3, 4, 5, 6, 7, 8), SByte8(7, 6, 5, 4, 3, 2, 1, 0));

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[6][4];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0x00000000u);
			EXPECT_EQ(out[0][1], 0xFFFFFFFFu);
			EXPECT_EQ(out[0][2], 0xFFFFFFFFu);
			EXPECT_EQ(out[0][3], 0xFFFFFFFFu);

			EXPECT_EQ(out[1][0], 0x00000000u);
			EXPECT_EQ(out[1][1], 0x00000000u);
			EXPECT_EQ(out[1][2], 0x00000000u);
			EXPECT_EQ(out[1][3], 0xFFFFFFFFu);

			EXPECT_EQ(out[2][0], 0xFF000000u);
			EXPECT_EQ(out[2][1], 0x00000000u);

			EXPECT_EQ(out[3][0], 0xFFFFFFFFu);
			EXPECT_EQ(out[3][1], 0xFFFFFFFFu);
			EXPECT_EQ(out[3][2], 0xFFFFFFFFu);
			EXPECT_EQ(out[3][3], 0xFFFFFFFFu);

			EXPECT_EQ(out[4][0], 0xFFFFFFFFu);
			EXPECT_EQ(out[4][1], 0x00000000u);
			EXPECT_EQ(out[4][2], 0x00000000u);
			EXPECT_EQ(out[4][3], 0xFFFFFFFFu);

			EXPECT_EQ(out[5][0], 0x00000000u);
			EXPECT_EQ(out[5][1], 0xFFFFFFFFu);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, SaturatedAddAndSubtract)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Byte8>(out + 8 * 0) =
				AddSat(Byte8(1, 2, 3, 4, 5, 6, 7, 8),
				       Byte8(7, 6, 5, 4, 3, 2, 1, 0));
			*Pointer<Byte8>(out + 8 * 1) =
				AddSat(Byte8(0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE),
				       Byte8(7, 6, 5, 4, 3, 2, 1, 0));
			*Pointer<Byte8>(out + 8 * 2) =
				SubSat(Byte8(1, 2, 3, 4, 5, 6, 7, 8),
				       Byte8(7, 6, 5, 4, 3, 2, 1, 0));

			*Pointer<SByte8>(out + 8 * 3) =
				AddSat(SByte8(1, 2, 3, 4, 5, 6, 7, 8),
				       SByte8(7, 6, 5, 4, 3, 2, 1, 0));
			*Pointer<SByte8>(out + 8 * 4) =
				AddSat(SByte8(0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E),
				       SByte8(7, 6, 5, 4, 3, 2, 1, 0));
			*Pointer<SByte8>(out + 8 * 5) =
				AddSat(SByte8(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88),
				       SByte8(-7, -6, -5, -4, -3, -2, -1, -0));
			*Pointer<SByte8>(out + 8 * 6) =
				SubSat(SByte8(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88),
				       SByte8(7, 6, 5, 4, 3, 2, 1, 0));

			*Pointer<Short4>(out + 8 * 7) =
				AddSat(Short4(1, 2, 3, 4), Short4(3, 2, 1, 0));
			*Pointer<Short4>(out + 8 * 8) =
				AddSat(Short4(0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE),
				       Short4(3, 2, 1, 0));
			*Pointer<Short4>(out + 8 * 9) =
				AddSat(Short4(0x8001, 0x8002, 0x8003, 0x8004),
				       Short4(-3, -2, -1, -0));
			*Pointer<Short4>(out + 8 * 10) =
				SubSat(Short4(0x8001, 0x8002, 0x8003, 0x8004),
				       Short4(3, 2, 1, 0));

			*Pointer<UShort4>(out + 8 * 11) =
				AddSat(UShort4(1, 2, 3, 4), UShort4(3, 2, 1, 0));
			*Pointer<UShort4>(out + 8 * 12) =
				AddSat(UShort4(0xFFFE, 0xFFFE, 0xFFFE, 0xFFFE),
				       UShort4(3, 2, 1, 0));
			*Pointer<UShort4>(out + 8 * 13) =
				SubSat(UShort4(1, 2, 3, 4), UShort4(3, 2, 1, 0));

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[14][2];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0x08080808u);
			EXPECT_EQ(out[0][1], 0x08080808u);

			EXPECT_EQ(out[1][0], 0xFFFFFFFFu);
			EXPECT_EQ(out[1][1], 0xFEFFFFFFu);

			EXPECT_EQ(out[2][0], 0x00000000u);
			EXPECT_EQ(out[2][1], 0x08060402u);

			EXPECT_EQ(out[3][0], 0x08080808u);
			EXPECT_EQ(out[3][1], 0x08080808u);

			EXPECT_EQ(out[4][0], 0x7F7F7F7Fu);
			EXPECT_EQ(out[4][1], 0x7E7F7F7Fu);

			EXPECT_EQ(out[5][0], 0x80808080u);
			EXPECT_EQ(out[5][1], 0x88868482u);

			EXPECT_EQ(out[6][0], 0x80808080u);
			EXPECT_EQ(out[6][1], 0x88868482u);

			EXPECT_EQ(out[7][0], 0x00040004u);
			EXPECT_EQ(out[7][1], 0x00040004u);

			EXPECT_EQ(out[8][0], 0x7FFF7FFFu);
			EXPECT_EQ(out[8][1], 0x7FFE7FFFu);

			EXPECT_EQ(out[9][0], 0x80008000u);
			EXPECT_EQ(out[9][1], 0x80048002u);

			EXPECT_EQ(out[10][0], 0x80008000u);
			EXPECT_EQ(out[10][1], 0x80048002u);

			EXPECT_EQ(out[11][0], 0x00040004u);
			EXPECT_EQ(out[11][1], 0x00040004u);

			EXPECT_EQ(out[12][0], 0xFFFFFFFFu);
			EXPECT_EQ(out[12][1], 0xFFFEFFFFu);

			EXPECT_EQ(out[13][0], 0x00000000u);
			EXPECT_EQ(out[13][1], 0x00040002u);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, Unpack)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>,Pointer<Byte>)> function;
		{
			Pointer<Byte> in = function.Arg<0>();
			Pointer<Byte> out = function.Arg<1>();

			Byte4 test_byte_a = *Pointer<Byte4>(in + 4 * 0);
			Byte4 test_byte_b = *Pointer<Byte4>(in + 4 * 1);

			*Pointer<Short4>(out + 8 * 0) =
				Unpack(test_byte_a, test_byte_b);

			*Pointer<Short4>(out + 8 * 1) = Unpack(test_byte_a);

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int in[1][2];
			unsigned int out[2][2];

			memset(&out, 0, sizeof(out));

			in[0][0] = 0xABCDEF12u;
			in[0][1] = 0x34567890u;

			int(*callable)(void*,void*) = (int(*)(void*,void*))routine->getEntry();
			callable(&in, &out);

			EXPECT_EQ(out[0][0], 0x78EF9012u);
			EXPECT_EQ(out[0][1], 0x34AB56CDu);

			EXPECT_EQ(out[1][0], 0xEFEF1212u);
			EXPECT_EQ(out[1][1], 0xABABCDCDu);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, Pack)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<SByte8>(out + 8 * 0) =
				PackSigned(Short4(-1, -2, 1, 2),
					   Short4(3, 4, -3, -4));

			*Pointer<Byte8>(out + 8 * 1) =
				PackUnsigned(Short4(-1, -2, 1, 2),
					     Short4(3, 4, -3, -4));

			*Pointer<Short8>(out + 8 * 2) =
				PackSigned(Int4(-1, -2, 1, 2),
					   Int4(3, 4, -3, -4));

			*Pointer<UShort8>(out + 8 * 4) =
				PackUnsigned(Int4(-1, -2, 1, 2),
					     Int4(3, 4, -3, -4));

			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[6][2];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0x0201FEFFu);
			EXPECT_EQ(out[0][1], 0xFCFD0403u);

			EXPECT_EQ(out[1][0], 0x02010000u);
			EXPECT_EQ(out[1][1], 0x00000403u);

			EXPECT_EQ(out[2][0], 0xFFFEFFFFu);
			EXPECT_EQ(out[2][1], 0x00020001u);

			EXPECT_EQ(out[3][0], 0x00040003u);
			EXPECT_EQ(out[3][1], 0xFFFCFFFDu);

			EXPECT_EQ(out[4][0], 0x00000000u);
			EXPECT_EQ(out[4][1], 0x00020001u);

			EXPECT_EQ(out[5][0], 0x00040003u);
			EXPECT_EQ(out[5][1], 0x00000000u);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, MulHigh)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Short4>(out + 8 * 0) =
				MulHigh(Short4(0x1aa, 0x2dd, 0x3ee, 0xF422),
					Short4(0x1bb, 0x2cc, 0x3ff, 0xF411));
			*Pointer<UShort4>(out + 8 * 1) =
				MulHigh(UShort4(0x1aa, 0x2dd, 0x3ee, 0xF422),
					UShort4(0x1bb, 0x2cc, 0x3ff, 0xF411));

			// (U)Short8 variants are mentioned but unimplemented
			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[2][2];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0x00080002u);
			EXPECT_EQ(out[0][1], 0x008D000fu);

			EXPECT_EQ(out[1][0], 0x00080002u);
			EXPECT_EQ(out[1][1], 0xe8C0000Fu);
		}
	}

	delete routine;
}

TEST(SubzeroReactorTest, MulAdd)
{
	Routine *routine = nullptr;

	{
		Function<Int(Pointer<Byte>)> function;
		{
			Pointer<Byte> out = function.Arg<0>();

			*Pointer<Int2>(out + 8 * 0) =
				MulAdd(Short4(0x1aa, 0x2dd, 0x3ee, 0xF422),
				       Short4(0x1bb, 0x2cc, 0x3ff, 0xF411));

			// (U)Short8 variant is mentioned but unimplemented
			Return(0);
		}

		routine = function(L"one");

		if(routine)
		{
			unsigned int out[1][2];

			memset(&out, 0, sizeof(out));

			int(*callable)(void*) = (int(*)(void*))routine->getEntry();
			callable(&out);

			EXPECT_EQ(out[0][0], 0x000AE34Au);
			EXPECT_EQ(out[0][1], 0x009D5254u);
		}
	}

	delete routine;
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
