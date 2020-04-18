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

#include "FrameBuffer.hpp"

#include "Renderer/Surface.hpp"
#include "Reactor/Reactor.hpp"
#include "Common/Timer.hpp"
#include "Common/Debug.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#define ASYNCHRONOUS_BLIT false   // FIXME: Currently leads to rare race conditions

namespace sw
{
	extern bool forceWindowed;

	FrameBuffer::Cursor FrameBuffer::cursor = {};
	bool FrameBuffer::topLeftOrigin = false;

	FrameBuffer::FrameBuffer(int width, int height, bool fullscreen, bool topLeftOrigin)
	{
		this->topLeftOrigin = topLeftOrigin;

		framebuffer = nullptr;

		this->width = width;
		this->height = height;
		format = FORMAT_X8R8G8B8;
		stride = 0;

		windowed = !fullscreen || forceWindowed;

		blitFunction = nullptr;
		blitRoutine = nullptr;
		blitState = {};

		if(ASYNCHRONOUS_BLIT)
		{
			terminate = false;
			FrameBuffer *parameters = this;
			blitThread = new Thread(threadFunction, &parameters);
		}
	}

	FrameBuffer::~FrameBuffer()
	{
		if(ASYNCHRONOUS_BLIT)
		{
			terminate = true;
			blitEvent.signal();
			blitThread->join();
			delete blitThread;
		}

		delete blitRoutine;
	}

	void FrameBuffer::setCursorImage(sw::Surface *cursorImage)
	{
		if(cursorImage)
		{
			cursor.image = cursorImage->lockExternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
			cursorImage->unlockExternal();

			cursor.width = cursorImage->getWidth();
			cursor.height = cursorImage->getHeight();
		}
		else
		{
			cursor.width = 0;
			cursor.height = 0;
		}
	}

	void FrameBuffer::setCursorOrigin(int x0, int y0)
	{
		cursor.hotspotX = x0;
		cursor.hotspotY = y0;
	}

	void FrameBuffer::setCursorPosition(int x, int y)
	{
		cursor.positionX = x;
		cursor.positionY = y;
	}

	void FrameBuffer::copy(sw::Surface *source)
	{
		if(!source)
		{
			return;
		}

		if(!lock())
		{
			return;
		}

		int sourceStride = source->getInternalPitchB();

		updateState = {};
		updateState.width = width;
		updateState.height = height;
		updateState.destFormat = format;
		updateState.destStride = stride;
		updateState.sourceFormat = source->getInternalFormat();
		updateState.sourceStride = topLeftOrigin ? sourceStride : -sourceStride;
		updateState.cursorWidth = cursor.width;
		updateState.cursorHeight = cursor.height;

		renderbuffer = source->lockInternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);

		if(!topLeftOrigin)
		{
			renderbuffer = (byte*)renderbuffer + (height - 1) * sourceStride;
		}

		cursor.x = cursor.positionX - cursor.hotspotX;
		cursor.y = cursor.positionY - cursor.hotspotY;

		if(ASYNCHRONOUS_BLIT)
		{
			blitEvent.signal();
			syncEvent.wait();
		}
		else
		{
			copyLocked();
		}

		source->unlockInternal();
		unlock();

		profiler.nextFrame();   // Assumes every copy() is a full frame
	}

	void FrameBuffer::copyLocked()
	{
		if(memcmp(&blitState, &updateState, sizeof(BlitState)) != 0)
		{
			blitState = updateState;
			delete blitRoutine;

			blitRoutine = copyRoutine(blitState);
			blitFunction = (void(*)(void*, void*, Cursor*))blitRoutine->getEntry();
		}

		blitFunction(framebuffer, renderbuffer, &cursor);
	}

	Routine *FrameBuffer::copyRoutine(const BlitState &state)
	{
		const int width = state.width;
		const int height = state.height;
		const int dBytes = Surface::bytes(state.destFormat);
		const int dStride = state.destStride;
		const int sBytes = Surface::bytes(state.sourceFormat);
		const int sStride = state.sourceStride;

		Function<Void(Pointer<Byte>, Pointer<Byte>, Pointer<Byte>)> function;
		{
			Pointer<Byte> dst(function.Arg<0>());
			Pointer<Byte> src(function.Arg<1>());
			Pointer<Byte> cursor(function.Arg<2>());

			For(Int y = 0, y < height, y++)
			{
				Pointer<Byte> d = dst + y * dStride;
				Pointer<Byte> s = src + y * sStride;

				Int x0 = 0;

				switch(state.destFormat)
				{
				case FORMAT_X8R8G8B8:
				case FORMAT_A8R8G8B8:
					{
						Int x = x0;

						switch(state.sourceFormat)
						{
						case FORMAT_X8R8G8B8:
						case FORMAT_A8R8G8B8:
							For(, x < width - 3, x += 4)
							{
								*Pointer<Int4>(d, 1) = *Pointer<Int4>(s, sStride % 16 ? 1 : 16);

								s += 4 * sBytes;
								d += 4 * dBytes;
							}
							break;
						case FORMAT_X8B8G8R8:
						case FORMAT_A8B8G8R8:
							For(, x < width - 3, x += 4)
							{
								Int4 bgra = *Pointer<Int4>(s, sStride % 16 ? 1 : 16);

								*Pointer<Int4>(d, 1) = ((bgra & Int4(0x00FF0000)) >> 16) |
								                       ((bgra & Int4(0x000000FF)) << 16) |
								                       (bgra & Int4(0xFF00FF00));

								s += 4 * sBytes;
								d += 4 * dBytes;
							}
							break;
						case FORMAT_A16B16G16R16:
							For(, x < width - 1, x += 2)
							{
								Short4 c0 = As<UShort4>(Swizzle(*Pointer<Short4>(s + 0), 0xC6)) >> 8;
								Short4 c1 = As<UShort4>(Swizzle(*Pointer<Short4>(s + 8), 0xC6)) >> 8;

								*Pointer<Int2>(d) = As<Int2>(PackUnsigned(c0, c1));

								s += 2 * sBytes;
								d += 2 * dBytes;
							}
							break;
						case FORMAT_R5G6B5:
							For(, x < width - 3, x += 4)
							{
								Int4 rgb = Int4(*Pointer<Short4>(s));

								*Pointer<Int4>(d) = (((rgb & Int4(0xF800)) << 8) | ((rgb & Int4(0xE01F)) << 3)) |
								                    (((rgb & Int4(0x07E0)) << 5) | ((rgb & Int4(0x0600)) >> 1)) |
								                    (((rgb & Int4(0x001C)) >> 2) | Int4(0xFF000000));

								s += 4 * sBytes;
								d += 4 * dBytes;
							}
							break;
						default:
							ASSERT(false);
							break;
						}

						For(, x < width, x++)
						{
							switch(state.sourceFormat)
							{
							case FORMAT_X8R8G8B8:
							case FORMAT_A8R8G8B8:
								*Pointer<Int>(d) = *Pointer<Int>(s);
								break;
							case FORMAT_X8B8G8R8:
							case FORMAT_A8B8G8R8:
								{
									Int rgba = *Pointer<Int>(s);

									*Pointer<Int>(d) = ((rgba & Int(0x00FF0000)) >> 16) |
									                   ((rgba & Int(0x000000FF)) << 16) |
									                   (rgba & Int(0xFF00FF00));
								}
								break;
							case FORMAT_A16B16G16R16:
								{
									Short4 c = As<UShort4>(Swizzle(*Pointer<Short4>(s), 0xC6)) >> 8;

									*Pointer<Int>(d) = Int(As<Int2>(PackUnsigned(c, c)));
								}
								break;
							case FORMAT_R5G6B5:
								{
									Int rgb = Int(*Pointer<Short>(s));

									*Pointer<Int>(d) = 0xFF000000 |
									                   ((rgb & 0xF800) << 8) | ((rgb & 0xE01F) << 3) |
								                       ((rgb & 0x07E0) << 5) | ((rgb & 0x0600) >> 1) |
								                       ((rgb & 0x001C) >> 2);
								}
								break;
							default:
								ASSERT(false);
								break;
							}

							s += sBytes;
							d += dBytes;
						}
					}
					break;
				case FORMAT_X8B8G8R8:
				case FORMAT_A8B8G8R8:
				case FORMAT_SRGB8_X8:
				case FORMAT_SRGB8_A8:
					{
						Int x = x0;

						switch(state.sourceFormat)
						{
						case FORMAT_X8B8G8R8:
						case FORMAT_A8B8G8R8:
							For(, x < width - 3, x += 4)
							{
								*Pointer<Int4>(d, 1) = *Pointer<Int4>(s, sStride % 16 ? 1 : 16);

								s += 4 * sBytes;
								d += 4 * dBytes;
							}
							break;
						case FORMAT_X8R8G8B8:
						case FORMAT_A8R8G8B8:
							For(, x < width - 3, x += 4)
							{
								Int4 bgra = *Pointer<Int4>(s, sStride % 16 ? 1 : 16);

								*Pointer<Int4>(d, 1) = ((bgra & Int4(0x00FF0000)) >> 16) |
								                       ((bgra & Int4(0x000000FF)) << 16) |
								                       (bgra & Int4(0xFF00FF00));

								s += 4 * sBytes;
								d += 4 * dBytes;
							}
							break;
						case FORMAT_A16B16G16R16:
							For(, x < width - 1, x += 2)
							{
								Short4 c0 = *Pointer<UShort4>(s + 0) >> 8;
								Short4 c1 = *Pointer<UShort4>(s + 8) >> 8;

								*Pointer<Int2>(d) = As<Int2>(PackUnsigned(c0, c1));

								s += 2 * sBytes;
								d += 2 * dBytes;
							}
							break;
						case FORMAT_R5G6B5:
							For(, x < width - 3, x += 4)
							{
								Int4 rgb = Int4(*Pointer<Short4>(s));

								*Pointer<Int4>(d) = Int4(0xFF000000) |
                                                    (((rgb & Int4(0x001F)) << 19) | ((rgb & Int4(0x001C)) << 14)) |
								                    (((rgb & Int4(0x07E0)) << 5) | ((rgb & Int4(0x0600)) >> 1)) |
								                    (((rgb & Int4(0xF800)) >> 8) | ((rgb & Int4(0xE000)) >> 13));

								s += 4 * sBytes;
								d += 4 * dBytes;
							}
							break;
						default:
							ASSERT(false);
							break;
						}

						For(, x < width, x++)
						{
							switch(state.sourceFormat)
							{
							case FORMAT_X8B8G8R8:
							case FORMAT_A8B8G8R8:
								*Pointer<Int>(d) = *Pointer<Int>(s);
								break;
							case FORMAT_X8R8G8B8:
							case FORMAT_A8R8G8B8:
								{
									Int bgra = *Pointer<Int>(s);
									*Pointer<Int>(d) = ((bgra & Int(0x00FF0000)) >> 16) |
									                   ((bgra & Int(0x000000FF)) << 16) |
									                   (bgra & Int(0xFF00FF00));
								}
								break;
							case FORMAT_A16B16G16R16:
								{
									Short4 c = *Pointer<UShort4>(s) >> 8;

									*Pointer<Int>(d) = Int(As<Int2>(PackUnsigned(c, c)));
								}
								break;
							case FORMAT_R5G6B5:
								{
									Int rgb = Int(*Pointer<Short>(s));

									*Pointer<Int>(d) = 0xFF000000 |
									                   ((rgb & 0x001F) << 19) | ((rgb & 0x001C) << 14) |
								                       ((rgb & 0x07E0) << 5) | ((rgb & 0x0600) >> 1) |
								                       ((rgb & 0xF800) >> 8) | ((rgb & 0xE000) >> 13);
								}
								break;
							default:
								ASSERT(false);
								break;
							}

							s += sBytes;
							d += dBytes;
						}
					}
					break;
				case FORMAT_R8G8B8:
					{
						For(Int x = x0, x < width, x++)
						{
							switch(state.sourceFormat)
							{
							case FORMAT_X8R8G8B8:
							case FORMAT_A8R8G8B8:
								*Pointer<Byte>(d + 0) = *Pointer<Byte>(s + 0);
								*Pointer<Byte>(d + 1) = *Pointer<Byte>(s + 1);
								*Pointer<Byte>(d + 2) = *Pointer<Byte>(s + 2);
								break;
							case FORMAT_X8B8G8R8:
							case FORMAT_A8B8G8R8:
								*Pointer<Byte>(d + 0) = *Pointer<Byte>(s + 2);
								*Pointer<Byte>(d + 1) = *Pointer<Byte>(s + 1);
								*Pointer<Byte>(d + 2) = *Pointer<Byte>(s + 0);
								break;
							case FORMAT_A16B16G16R16:
								*Pointer<Byte>(d + 0) = *Pointer<Byte>(s + 5);
								*Pointer<Byte>(d + 1) = *Pointer<Byte>(s + 3);
								*Pointer<Byte>(d + 2) = *Pointer<Byte>(s + 1);
								break;
							case FORMAT_R5G6B5:
								{
									Int rgb = Int(*Pointer<Short>(s));

									*Pointer<Byte>(d + 0) = Byte(((rgb & 0x001F) << 3) | ((rgb & 0x001C) >> 2));
									*Pointer<Byte>(d + 1) = Byte(((rgb & 0x07E0) << 5) | ((rgb & 0x0600) >> 1));
									*Pointer<Byte>(d + 2) = Byte(((rgb & 0xF800) << 8) | ((rgb & 0xE000) << 3));
								}
								break;
							default:
								ASSERT(false);
								break;
							}

							s += sBytes;
							d += dBytes;
						}
					}
					break;
				case FORMAT_R5G6B5:
					{
						For(Int x = x0, x < width, x++)
						{
							switch(state.sourceFormat)
							{
							case FORMAT_X8R8G8B8:
							case FORMAT_A8R8G8B8:
								{
									Int c = *Pointer<Int>(s);

									*Pointer<Short>(d) = Short((c & 0x00F80000) >> 8 |
									                           (c & 0x0000FC00) >> 5 |
									                           (c & 0x000000F8) >> 3);
								}
								break;
							case FORMAT_X8B8G8R8:
							case FORMAT_A8B8G8R8:
								{
									Int c = *Pointer<Int>(s);

									*Pointer<Short>(d) = Short((c & 0x00F80000) >> 19 |
									                           (c & 0x0000FC00) >> 5 |
									                           (c & 0x000000F8) << 8);
								}
								break;
							case FORMAT_A16B16G16R16:
								{
									Short4 cc = *Pointer<UShort4>(s) >> 8;
									Int c = Int(As<Int2>(PackUnsigned(cc, cc)));

									*Pointer<Short>(d) = Short((c & 0x00F80000) >> 19 |
									                           (c & 0x0000FC00) >> 5 |
									                           (c & 0x000000F8) << 8);
								}
								break;
							case FORMAT_R5G6B5:
								*Pointer<Short>(d) = *Pointer<Short>(s);
								break;
							default:
								ASSERT(false);
								break;
							}

							s += sBytes;
							d += dBytes;
						}
					}
					break;
				default:
					ASSERT(false);
					break;
				}
			}

			if(state.cursorWidth > 0 && state.cursorHeight > 0)
			{
				Int x0 = *Pointer<Int>(cursor + OFFSET(Cursor,x));
				Int y0 = *Pointer<Int>(cursor + OFFSET(Cursor,y));

				For(Int y1 = 0, y1 < state.cursorHeight, y1++)
				{
					Int y = y0 + y1;

					If(y >= 0 && y < height)
					{
						Pointer<Byte> d = dst + y * dStride + x0 * dBytes;
						Pointer<Byte> s = src + y * sStride + x0 * sBytes;
						Pointer<Byte> c = *Pointer<Pointer<Byte>>(cursor + OFFSET(Cursor,image)) + y1 * state.cursorWidth * 4;

						For(Int x1 = 0, x1 < state.cursorWidth, x1++)
						{
							Int x = x0 + x1;

							If(x >= 0 && x < width)
							{
								blend(state, d, s, c);
							}

							c += 4;
							s += sBytes;
							d += dBytes;
						}
					}
				}
			}
		}

		return function(L"FrameBuffer");
	}

	void FrameBuffer::blend(const BlitState &state, const Pointer<Byte> &d, const Pointer<Byte> &s, const Pointer<Byte> &c)
	{
		Short4 c1;
		Short4 c2;

		c1 = Unpack(*Pointer<Byte4>(c));

		switch(state.sourceFormat)
		{
		case FORMAT_X8R8G8B8:
		case FORMAT_A8R8G8B8:
			c2 = Unpack(*Pointer<Byte4>(s));
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_A8B8G8R8:
			c2 = Swizzle(Unpack(*Pointer<Byte4>(s)), 0xC6);
			break;
		case FORMAT_A16B16G16R16:
			c2 = Swizzle(*Pointer<Short4>(s), 0xC6);
			break;
		case FORMAT_R5G6B5:
			{
				Int rgb(*Pointer<Short>(s));
				rgb = 0xFF000000 |
				      ((rgb & 0xF800) << 8) | ((rgb & 0xE01F) << 3) |
				      ((rgb & 0x07E0) << 5) | ((rgb & 0x0600) >> 1) |
				      ((rgb & 0x001C) >> 2);
				c2 = Unpack(As<Byte4>(rgb));
			}
			break;
		default:
			ASSERT(false);
			break;
		}

		c1 = As<Short4>(As<UShort4>(c1) >> 9);
		c2 = As<Short4>(As<UShort4>(c2) >> 9);

		Short4 alpha = Swizzle(c1, 0xFF) & Short4(0xFFFFu, 0xFFFFu, 0xFFFFu, 0x0000);

		c1 = (c1 - c2) * alpha;
		c1 = c1 >> 7;
		c1 = c1 + c2;
		c1 = c1 + c1;

		switch(state.destFormat)
		{
		case FORMAT_X8R8G8B8:
		case FORMAT_A8R8G8B8:
			*Pointer<Byte4>(d) = Byte4(PackUnsigned(c1, c1));
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
			{
				c1 = Swizzle(c1, 0xC6);

				*Pointer<Byte4>(d) = Byte4(PackUnsigned(c1, c1));
			}
			break;
		case FORMAT_R8G8B8:
			{
				Int c = Int(As<Int2>(PackUnsigned(c1, c1)));

				*Pointer<Byte>(d + 0) = Byte(c >> 0);
				*Pointer<Byte>(d + 1) = Byte(c >> 8);
				*Pointer<Byte>(d + 2) = Byte(c >> 16);
			}
			break;
		case FORMAT_R5G6B5:
			{
				Int c = Int(As<Int2>(PackUnsigned(c1, c1)));

				*Pointer<Short>(d) = Short((c & 0x00F80000) >> 8 |
				                           (c & 0x0000FC00) >> 5 |
				                           (c & 0x000000F8) >> 3);
			}
			break;
		default:
			ASSERT(false);
			break;
		}
	}

	void FrameBuffer::threadFunction(void *parameters)
	{
		FrameBuffer *frameBuffer = *static_cast<FrameBuffer**>(parameters);

		while(!frameBuffer->terminate)
		{
			frameBuffer->blitEvent.wait();

			if(!frameBuffer->terminate)
			{
				frameBuffer->copyLocked();

				frameBuffer->syncEvent.signal();
			}
		}
	}
}
