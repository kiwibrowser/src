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

#ifndef sw_Surface_hpp
#define sw_Surface_hpp

#include "Color.hpp"
#include "Main/Config.hpp"
#include "Common/Resource.hpp"

namespace sw
{
	class Resource;

	template <typename T> struct RectT
	{
		RectT() {}
		RectT(T x0i, T y0i, T x1i, T y1i) : x0(x0i), y0(y0i), x1(x1i), y1(y1i) {}

		void clip(T minX, T minY, T maxX, T maxY)
		{
			x0 = clamp(x0, minX, maxX);
			y0 = clamp(y0, minY, maxY);
			x1 = clamp(x1, minX, maxX);
			y1 = clamp(y1, minY, maxY);
		}

		T width() const  { return x1 - x0; }
		T height() const { return y1 - y0; }

		T x0;   // Inclusive
		T y0;   // Inclusive
		T x1;   // Exclusive
		T y1;   // Exclusive
	};

	typedef RectT<int> Rect;
	typedef RectT<float> RectF;

	template<typename T> struct SliceRectT : public RectT<T>
	{
		SliceRectT() : slice(0) {}
		SliceRectT(const RectT<T>& rect) : RectT<T>(rect), slice(0) {}
		SliceRectT(const RectT<T>& rect, int s) : RectT<T>(rect), slice(s) {}
		SliceRectT(T x0, T y0, T x1, T y1, int s) : RectT<T>(x0, y0, x1, y1), slice(s) {}
		int slice;
	};

	typedef SliceRectT<int> SliceRect;
	typedef SliceRectT<float> SliceRectF;

	enum Format : unsigned char
	{
		FORMAT_NULL,

		FORMAT_A8,
		FORMAT_R8I,
		FORMAT_R8UI,
		FORMAT_R8_SNORM,
		FORMAT_R8,
		FORMAT_R16I,
		FORMAT_R16UI,
		FORMAT_R32I,
		FORMAT_R32UI,
		FORMAT_R3G3B2,
		FORMAT_A8R3G3B2,
		FORMAT_X4R4G4B4,
		FORMAT_A4R4G4B4,
		FORMAT_R4G4B4A4,
		FORMAT_R5G6B5,
		FORMAT_R8G8B8,
		FORMAT_B8G8R8,
		FORMAT_X8R8G8B8,
		FORMAT_A8R8G8B8,
		FORMAT_X8B8G8R8I,
		FORMAT_X8B8G8R8UI,
		FORMAT_X8B8G8R8_SNORM,
		FORMAT_X8B8G8R8,
		FORMAT_A8B8G8R8I,
		FORMAT_A8B8G8R8UI,
		FORMAT_A8B8G8R8_SNORM,
		FORMAT_A8B8G8R8,
		FORMAT_SRGB8_X8,
		FORMAT_SRGB8_A8,
		FORMAT_X1R5G5B5,
		FORMAT_A1R5G5B5,
		FORMAT_R5G5B5A1,
		FORMAT_G8R8I,
		FORMAT_G8R8UI,
		FORMAT_G8R8_SNORM,
		FORMAT_G8R8,
		FORMAT_G16R16,
		FORMAT_G16R16I,
		FORMAT_G16R16UI,
		FORMAT_G32R32I,
		FORMAT_G32R32UI,
		FORMAT_A2R10G10B10,
		FORMAT_A2B10G10R10,
		FORMAT_A2B10G10R10UI,
		FORMAT_A16B16G16R16,
		FORMAT_X16B16G16R16I,
		FORMAT_X16B16G16R16UI,
		FORMAT_A16B16G16R16I,
		FORMAT_A16B16G16R16UI,
		FORMAT_X32B32G32R32I,
		FORMAT_X32B32G32R32UI,
		FORMAT_A32B32G32R32I,
		FORMAT_A32B32G32R32UI,
		// Paletted formats
		FORMAT_P8,
		FORMAT_A8P8,
		// Compressed formats
		FORMAT_DXT1,
		FORMAT_DXT3,
		FORMAT_DXT5,
		FORMAT_ATI1,
		FORMAT_ATI2,
		FORMAT_ETC1,
		FORMAT_R11_EAC,
		FORMAT_SIGNED_R11_EAC,
		FORMAT_RG11_EAC,
		FORMAT_SIGNED_RG11_EAC,
		FORMAT_RGB8_ETC2,
		FORMAT_SRGB8_ETC2,
		FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
		FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
		FORMAT_RGBA8_ETC2_EAC,
		FORMAT_SRGB8_ALPHA8_ETC2_EAC,
		FORMAT_RGBA_ASTC_4x4_KHR,
		FORMAT_RGBA_ASTC_5x4_KHR,
		FORMAT_RGBA_ASTC_5x5_KHR,
		FORMAT_RGBA_ASTC_6x5_KHR,
		FORMAT_RGBA_ASTC_6x6_KHR,
		FORMAT_RGBA_ASTC_8x5_KHR,
		FORMAT_RGBA_ASTC_8x6_KHR,
		FORMAT_RGBA_ASTC_8x8_KHR,
		FORMAT_RGBA_ASTC_10x5_KHR,
		FORMAT_RGBA_ASTC_10x6_KHR,
		FORMAT_RGBA_ASTC_10x8_KHR,
		FORMAT_RGBA_ASTC_10x10_KHR,
		FORMAT_RGBA_ASTC_12x10_KHR,
		FORMAT_RGBA_ASTC_12x12_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR,
		FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR,
		// Floating-point formats
		FORMAT_A16F,
		FORMAT_R16F,
		FORMAT_G16R16F,
		FORMAT_B16G16R16F,
		FORMAT_X16B16G16R16F,
		FORMAT_A16B16G16R16F,
		FORMAT_X16B16G16R16F_UNSIGNED,
		FORMAT_A32F,
		FORMAT_R32F,
		FORMAT_G32R32F,
		FORMAT_B32G32R32F,
		FORMAT_X32B32G32R32F,
		FORMAT_A32B32G32R32F,
		FORMAT_X32B32G32R32F_UNSIGNED,
		// Bump map formats
		FORMAT_V8U8,
		FORMAT_L6V5U5,
		FORMAT_Q8W8V8U8,
		FORMAT_X8L8V8U8,
		FORMAT_A2W10V10U10,
		FORMAT_V16U16,
		FORMAT_A16W16V16U16,
		FORMAT_Q16W16V16U16,
		// Luminance formats
		FORMAT_L8,
		FORMAT_A4L4,
		FORMAT_L16,
		FORMAT_A8L8,
		FORMAT_L16F,
		FORMAT_A16L16F,
		FORMAT_L32F,
		FORMAT_A32L32F,
		// Depth/stencil formats
		FORMAT_D16,
		FORMAT_D32,
		FORMAT_D24X8,
		FORMAT_D24S8,
		FORMAT_D24FS8,
		FORMAT_D32F,                 // Quad layout
		FORMAT_D32FS8,               // Quad layout
		FORMAT_D32F_COMPLEMENTARY,   // Quad layout, 1 - z
		FORMAT_D32FS8_COMPLEMENTARY, // Quad layout, 1 - z
		FORMAT_D32F_LOCKABLE,        // Linear layout
		FORMAT_D32FS8_TEXTURE,       // Linear layout, no PCF
		FORMAT_D32F_SHADOW,          // Linear layout, PCF
		FORMAT_D32FS8_SHADOW,        // Linear layout, PCF
		FORMAT_DF24S8,
		FORMAT_DF16S8,
		FORMAT_INTZ,
		FORMAT_S8,
		// Quad layout framebuffer
		FORMAT_X8G8R8B8Q,
		FORMAT_A8G8R8B8Q,
		// YUV formats
		FORMAT_YV12_BT601,
		FORMAT_YV12_BT709,
		FORMAT_YV12_JFIF,    // Full-swing BT.601

		FORMAT_LAST = FORMAT_YV12_JFIF
	};

	enum Lock
	{
		LOCK_UNLOCKED,
		LOCK_READONLY,
		LOCK_WRITEONLY,
		LOCK_READWRITE,
		LOCK_DISCARD,
		LOCK_UPDATE   // Write access which doesn't dirty the buffer, because it's being updated with the sibling's data.
	};

	class [[clang::lto_visibility_public]] Surface
	{
	private:
		struct Buffer
		{
			friend Surface;

		private:
			void write(int x, int y, int z, const Color<float> &color);
			void write(int x, int y, const Color<float> &color);
			void write(void *element, const Color<float> &color);
			Color<float> read(int x, int y, int z) const;
			Color<float> read(int x, int y) const;
			Color<float> read(void *element) const;
			Color<float> sample(float x, float y, float z) const;
			Color<float> sample(float x, float y, int layer) const;

			void *lockRect(int x, int y, int z, Lock lock);
			void unlockRect();

			void *buffer;
			int width;
			int height;
			int depth;
			short border;
			short samples;

			int bytes;
			int pitchB;
			int pitchP;
			int sliceB;
			int sliceP;

			Format format;
			AtomicInt lock;

			bool dirty;   // Sibling internal/external buffer doesn't match.
		};

	protected:
		Surface(int width, int height, int depth, Format format, void *pixels, int pitch, int slice);
		Surface(Resource *texture, int width, int height, int depth, int border, int samples, Format format, bool lockable, bool renderTarget, int pitchP = 0);

	public:
		static Surface *create(int width, int height, int depth, Format format, void *pixels, int pitch, int slice);
		static Surface *create(Resource *texture, int width, int height, int depth, int border, int samples, Format format, bool lockable, bool renderTarget, int pitchP = 0);

		virtual ~Surface() = 0;

		inline void *lock(int x, int y, int z, Lock lock, Accessor client, bool internal = false);
		inline void unlock(bool internal = false);
		inline int getWidth() const;
		inline int getHeight() const;
		inline int getDepth() const;
		inline int getBorder() const;
		inline Format getFormat(bool internal = false) const;
		inline int getPitchB(bool internal = false) const;
		inline int getPitchP(bool internal = false) const;
		inline int getSliceB(bool internal = false) const;
		inline int getSliceP(bool internal = false) const;

		void *lockExternal(int x, int y, int z, Lock lock, Accessor client);
		void unlockExternal();
		inline Format getExternalFormat() const;
		inline int getExternalPitchB() const;
		inline int getExternalPitchP() const;
		inline int getExternalSliceB() const;
		inline int getExternalSliceP() const;

		virtual void *lockInternal(int x, int y, int z, Lock lock, Accessor client) = 0;
		virtual void unlockInternal() = 0;
		inline Format getInternalFormat() const;
		inline int getInternalPitchB() const;
		inline int getInternalPitchP() const;
		inline int getInternalSliceB() const;
		inline int getInternalSliceP() const;

		void *lockStencil(int x, int y, int front, Accessor client);
		void unlockStencil();
		inline Format getStencilFormat() const;
		inline int getStencilPitchB() const;
		inline int getStencilSliceB() const;

		void sync();                      // Wait for lock(s) to be released.
		inline bool isUnlocked() const;   // Only reliable after sync().

		inline int getSamples() const;
		inline int getMultiSampleCount() const;
		inline int getSuperSampleCount() const;

		bool isEntire(const Rect& rect) const;
		Rect getRect() const;
		void clearDepth(float depth, int x0, int y0, int width, int height);
		void clearStencil(unsigned char stencil, unsigned char mask, int x0, int y0, int width, int height);
		void fill(const Color<float> &color, int x0, int y0, int width, int height);

		Color<float> readExternal(int x, int y, int z) const;
		Color<float> readExternal(int x, int y) const;
		Color<float> sampleExternal(float x, float y, float z) const;
		Color<float> sampleExternal(float x, float y) const;
		void writeExternal(int x, int y, int z, const Color<float> &color);
		void writeExternal(int x, int y, const Color<float> &color);

		void copyInternal(const Surface* src, int x, int y, float srcX, float srcY, bool filter);
		void copyInternal(const Surface* src, int x, int y, int z, float srcX, float srcY, float srcZ, bool filter);

		enum Edge { TOP, BOTTOM, RIGHT, LEFT };
		void copyCubeEdge(Edge dstEdge, Surface *src, Edge srcEdge);
		void computeCubeCorner(int x0, int y0, int x1, int y1);

		bool hasStencil() const;
		bool hasDepth() const;
		bool hasPalette() const;
		bool isRenderTarget() const;

		bool hasDirtyContents() const;
		void markContentsClean();
		inline bool isExternalDirty() const;
		Resource *getResource();

		static int bytes(Format format);
		static int pitchB(int width, int border, Format format, bool target);
		static int pitchP(int width, int border, Format format, bool target);
		static int sliceB(int width, int height, int border, Format format, bool target);
		static int sliceP(int width, int height, int border, Format format, bool target);
		static size_t size(int width, int height, int depth, int border, int samples, Format format);

		static bool isStencil(Format format);
		static bool isDepth(Format format);
		static bool hasQuadLayout(Format format);
		static bool isPalette(Format format);

		static bool isFloatFormat(Format format);
		static bool isUnsignedComponent(Format format, int component);
		static bool isSRGBreadable(Format format);
		static bool isSRGBwritable(Format format);
		static bool isSRGBformat(Format format);
		static bool isCompressed(Format format);
		static bool isSignedNonNormalizedInteger(Format format);
		static bool isUnsignedNonNormalizedInteger(Format format);
		static bool isNonNormalizedInteger(Format format);
		static bool isNormalizedInteger(Format format);
		static int componentCount(Format format);

		static void setTexturePalette(unsigned int *palette);

	private:
		sw::Resource *resource;

		typedef unsigned char byte;
		typedef unsigned short word;
		typedef unsigned int dword;
		typedef uint64_t qword;

		struct DXT1
		{
			word c0;
			word c1;
			dword lut;
		};

		struct DXT3
		{
			qword a;

			word c0;
			word c1;
			dword lut;
		};

		struct DXT5
		{
			union
			{
				struct
				{
					byte a0;
					byte a1;
				};

				qword alut;   // Skip first 16 bit
			};

			word c0;
			word c1;
			dword clut;
		};

		struct ATI2
		{
			union
			{
				struct
				{
					byte y0;
					byte y1;
				};

				qword ylut;   // Skip first 16 bit
			};

			union
			{
				struct
				{
					byte x0;
					byte x1;
				};

				qword xlut;   // Skip first 16 bit
			};
		};

		struct ATI1
		{
			union
			{
				struct
				{
					byte r0;
					byte r1;
				};

				qword rlut;   // Skip first 16 bit
			};
		};

		static void decodeR8G8B8(Buffer &destination, Buffer &source);
		static void decodeX1R5G5B5(Buffer &destination, Buffer &source);
		static void decodeA1R5G5B5(Buffer &destination, Buffer &source);
		static void decodeX4R4G4B4(Buffer &destination, Buffer &source);
		static void decodeA4R4G4B4(Buffer &destination, Buffer &source);
		static void decodeP8(Buffer &destination, Buffer &source);

		static void decodeDXT1(Buffer &internal, Buffer &external);
		static void decodeDXT3(Buffer &internal, Buffer &external);
		static void decodeDXT5(Buffer &internal, Buffer &external);
		static void decodeATI1(Buffer &internal, Buffer &external);
		static void decodeATI2(Buffer &internal, Buffer &external);
		static void decodeEAC(Buffer &internal, Buffer &external, int nbChannels, bool isSigned);
		static void decodeETC2(Buffer &internal, Buffer &external, int nbAlphaBits, bool isSRGB);
		static void decodeASTC(Buffer &internal, Buffer &external, int xSize, int ySize, int zSize, bool isSRGB);

		static void update(Buffer &destination, Buffer &source);
		static void genericUpdate(Buffer &destination, Buffer &source);
		static void *allocateBuffer(int width, int height, int depth, int border, int samples, Format format);
		static void memfill4(void *buffer, int pattern, int bytes);

		bool identicalFormats() const;
		Format selectInternalFormat(Format format) const;

		void resolve();

		Buffer external;
		Buffer internal;
		Buffer stencil;

		const bool lockable;
		const bool renderTarget;

		bool dirtyContents;   // Sibling surfaces need updating (mipmaps / cube borders).
		unsigned int paletteUsed;

		static unsigned int *palette;   // FIXME: Not multi-device safe
		static unsigned int paletteID;

		bool hasParent;
		bool ownExternal;
	};
}

#undef min
#undef max

namespace sw
{
	void *Surface::lock(int x, int y, int z, Lock lock, Accessor client, bool internal)
	{
		return internal ? lockInternal(x, y, z, lock, client) : lockExternal(x, y, z, lock, client);
	}

	void Surface::unlock(bool internal)
	{
		return internal ? unlockInternal() : unlockExternal();
	}

	int Surface::getWidth() const
	{
		return external.width;
	}

	int Surface::getHeight() const
	{
		return external.height;
	}

	int Surface::getDepth() const
	{
		return external.depth;
	}

	int Surface::getBorder() const
	{
		return internal.border;
	}

	Format Surface::getFormat(bool internal) const
	{
		return internal ? getInternalFormat() : getExternalFormat();
	}

	int Surface::getPitchB(bool internal) const
	{
		return internal ? getInternalPitchB() : getExternalPitchB();
	}

	int Surface::getPitchP(bool internal) const
	{
		return internal ? getInternalPitchP() : getExternalPitchP();
	}

	int Surface::getSliceB(bool internal) const
	{
		return internal ? getInternalSliceB() : getExternalSliceB();
	}

	int Surface::getSliceP(bool internal) const
	{
		return internal ? getInternalSliceP() : getExternalSliceP();
	}

	Format Surface::getExternalFormat() const
	{
		return external.format;
	}

	int Surface::getExternalPitchB() const
	{
		return external.pitchB;
	}

	int Surface::getExternalPitchP() const
	{
		return external.pitchP;
	}

	int Surface::getExternalSliceB() const
	{
		return external.sliceB;
	}

	int Surface::getExternalSliceP() const
	{
		return external.sliceP;
	}

	Format Surface::getInternalFormat() const
	{
		return internal.format;
	}

	int Surface::getInternalPitchB() const
	{
		return internal.pitchB;
	}

	int Surface::getInternalPitchP() const
	{
		return internal.pitchP;
	}

	int Surface::getInternalSliceB() const
	{
		return internal.sliceB;
	}

	int Surface::getInternalSliceP() const
	{
		return internal.sliceP;
	}

	Format Surface::getStencilFormat() const
	{
		return stencil.format;
	}

	int Surface::getStencilPitchB() const
	{
		return stencil.pitchB;
	}

	int Surface::getStencilSliceB() const
	{
		return stencil.sliceB;
	}

	int Surface::getSamples() const
	{
		return internal.samples;
	}

	int Surface::getMultiSampleCount() const
	{
		return sw::min((int)internal.samples, 4);
	}

	int Surface::getSuperSampleCount() const
	{
		return internal.samples > 4 ? internal.samples / 4 : 1;
	}

	bool Surface::isUnlocked() const
	{
		return external.lock == LOCK_UNLOCKED &&
		       internal.lock == LOCK_UNLOCKED &&
		       stencil.lock == LOCK_UNLOCKED;
	}

	bool Surface::isExternalDirty() const
	{
		return external.buffer && external.buffer != internal.buffer && external.dirty;
	}
}

#endif   // sw_Surface_hpp
