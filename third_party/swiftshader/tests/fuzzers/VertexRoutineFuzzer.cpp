// Copyright 2017 The SwiftShader Authors. All Rights Reserved.
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

#include "OpenGL/compiler/InitializeGlobals.h"
#include "OpenGL/compiler/InitializeParseContext.h"
#include "OpenGL/compiler/TranslatorASM.h"

// TODO: Debug macros of the GLSL compiler clash with core SwiftShader's.
// They should not be exposed through the interface headers above.
#undef ASSERT
#undef UNIMPLEMENTED

#include "Renderer/VertexProcessor.hpp"
#include "Shader/VertexProgram.hpp"

#include <cstdint>
#include <memory>
#include <cassert>

namespace {

// TODO(cwallez@google.com): Like in ANGLE, disable most of the pool allocator for fuzzing
// This is a helper class to make sure all the resources used by the compiler are initialized
class ScopedPoolAllocatorAndTLS {
	public:
		ScopedPoolAllocatorAndTLS() {
			InitializeParseContextIndex();
			InitializePoolIndex();
			SetGlobalPoolAllocator(&allocator);
		}
		~ScopedPoolAllocatorAndTLS() {
			SetGlobalPoolAllocator(nullptr);
			FreePoolIndex();
			FreeParseContextIndex();
		}

	private:
		TPoolAllocator allocator;
};

// Trivial implementation of the glsl::Shader interface that fakes being an API-level
// shader object.
class FakeVS : public glsl::Shader {
	public:
		FakeVS(sw::VertexShader* bytecode) : bytecode(bytecode) {
		}

		sw::Shader *getShader() const override {
			return bytecode;
		}
		sw::VertexShader *getVertexShader() const override {
			return bytecode;
		}

	private:
		sw::VertexShader* bytecode;
};

} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// Data layout:
	//
	// byte: boolean states
	// {
	//   byte: stream type
	//   byte: stream count and normalized
	// } [MAX_VERTEX_INPUTS]
	// {
	//   byte[32]: reserved sampler state
	// } [VERTEX_TEXTURE_IMAGE_UNITS]
	//
	// char source[] // null terminated
	const size_t kHeaderSize = 1 + 2 * sw::MAX_VERTEX_INPUTS + 32 * sw::VERTEX_TEXTURE_IMAGE_UNITS;

	if(size <= kHeaderSize)
	{
		return 0;
	}

	if (data[size -1] != 0)
	{
		return 0;
	}

	std::unique_ptr<ScopedPoolAllocatorAndTLS> allocatorAndTLS(new ScopedPoolAllocatorAndTLS);
	std::unique_ptr<sw::VertexShader> shader(new sw::VertexShader);
	std::unique_ptr<FakeVS> fakeVS(new FakeVS(shader.get()));
	
	std::unique_ptr<TranslatorASM> glslCompiler(new TranslatorASM(fakeVS.get(), GL_VERTEX_SHADER));

	// TODO(cwallez@google.com): have a function to init to default values somewhere
	ShBuiltInResources resources;
	resources.MaxVertexAttribs = sw::MAX_VERTEX_INPUTS;
	resources.MaxVertexUniformVectors = sw::VERTEX_UNIFORM_VECTORS - 3;
	resources.MaxVaryingVectors = MIN(sw::MAX_VERTEX_OUTPUTS, sw::MAX_VERTEX_INPUTS);
	resources.MaxVertexTextureImageUnits = sw::VERTEX_TEXTURE_IMAGE_UNITS;
	resources.MaxCombinedTextureImageUnits = sw::TEXTURE_IMAGE_UNITS + sw::VERTEX_TEXTURE_IMAGE_UNITS;
	resources.MaxTextureImageUnits = sw::TEXTURE_IMAGE_UNITS;
	resources.MaxFragmentUniformVectors = sw::FRAGMENT_UNIFORM_VECTORS - 3;
	resources.MaxDrawBuffers = sw::RENDERTARGETS;
	resources.MaxVertexOutputVectors = 16; // ???
	resources.MaxFragmentInputVectors = 15; // ???
	resources.MinProgramTexelOffset = sw::MIN_PROGRAM_TEXEL_OFFSET;
	resources.MaxProgramTexelOffset = sw::MAX_PROGRAM_TEXEL_OFFSET;
	resources.OES_standard_derivatives = 1;
	resources.OES_fragment_precision_high = 1;
	resources.OES_EGL_image_external = 1;
	resources.EXT_draw_buffers = 1;
	resources.ARB_texture_rectangle = 1;
	resources.MaxCallStackDepth = 16;

	glslCompiler->Init(resources);

	const char* glslSource = reinterpret_cast<const char*>(data + kHeaderSize);
	if (!glslCompiler->compile(&glslSource, 1, SH_OBJECT_CODE))
	{
		return 0;
	}

	std::unique_ptr<sw::VertexShader> bytecodeShader(new sw::VertexShader(fakeVS->getVertexShader()));

	sw::VertexProcessor::State state;

	state.textureSampling = bytecodeShader->containsTextureSampling();
	state.positionRegister = bytecodeShader->getPositionRegister();
	state.pointSizeRegister = bytecodeShader->getPointSizeRegister();

	state.preTransformed = (data[0] & 0x01) != 0;
	state.superSampling = (data[0] & 0x02) != 0;
	state.multiSampling = (data[0] & 0x04) != 0;

	state.transformFeedbackQueryEnabled = (data[0] & 0x08) != 0;
	state.transformFeedbackEnabled = (data[0] & 0x10) != 0;
	state.verticesPerPrimitive = 1 + ((data[0] & 0x20) != 0) + ((data[0] & 0x40) != 0);

	if((data[0] & 0x80) != 0)   // Unused/reserved.
	{
		return 0;
	}

	constexpr int MAX_ATTRIBUTE_COMPONENTS = 4;

	struct Stream
	{
		uint8_t count : BITS(MAX_ATTRIBUTE_COMPONENTS);
		bool normalized : 1;
		uint8_t reserved : 8 - BITS(MAX_ATTRIBUTE_COMPONENTS) - 1;
	};

	for(int i = 0; i < sw::MAX_VERTEX_INPUTS; i++)
	{
		sw::StreamType type = (sw::StreamType)data[1 + 2 * i + 0];
		Stream stream = (Stream&)data[1 + 2 * i + 1];

		if(type > sw::STREAMTYPE_LAST) return 0;
		if(stream.count > MAX_ATTRIBUTE_COMPONENTS) return 0;
		if(stream.reserved != 0) return 0;

		state.input[i].type = type;
		state.input[i].count = stream.count;
		state.input[i].normalized = stream.normalized;
		state.input[i].attribType = bytecodeShader->getAttribType(i);
	}

	for(unsigned int i = 0; i < sw::VERTEX_TEXTURE_IMAGE_UNITS; i++)
	{
		// TODO
	//	if(bytecodeShader->usesSampler(i))
	//	{
	//		state.samplerState[i] = context->sampler[sw::TEXTURE_IMAGE_UNITS + i].samplerState();
	//	}

		for(int j = 0; j < 32; j++)
		{
			if(data[1 + 2 * sw::MAX_VERTEX_INPUTS + 32 * i + j] != 0)
			{
				return 0;
			}
		}
	}

	for(int i = 0; i < sw::MAX_VERTEX_OUTPUTS; i++)
	{
		state.output[i].xWrite = bytecodeShader->getOutput(i, 0).active();
		state.output[i].yWrite = bytecodeShader->getOutput(i, 1).active();
		state.output[i].zWrite = bytecodeShader->getOutput(i, 2).active();
		state.output[i].wWrite = bytecodeShader->getOutput(i, 3).active();
	}

	sw::VertexProgram program(state, bytecodeShader.get());
	program.generate();

	sw::Routine *routine = program(L"VertexRoutine");
	assert(routine);
	const void *entry = routine->getEntry();
	assert(entry); (void)entry;
	delete routine;

	return 0;
}
