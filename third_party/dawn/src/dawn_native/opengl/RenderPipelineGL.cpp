// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/opengl/RenderPipelineGL.h"

#include "dawn_native/opengl/DeviceGL.h"
#include "dawn_native/opengl/Forward.h"
#include "dawn_native/opengl/PersistentPipelineStateGL.h"
#include "dawn_native/opengl/UtilsGL.h"

namespace dawn_native { namespace opengl {

    namespace {

        GLenum GLPrimitiveTopology(wgpu::PrimitiveTopology primitiveTopology) {
            switch (primitiveTopology) {
                case wgpu::PrimitiveTopology::PointList:
                    return GL_POINTS;
                case wgpu::PrimitiveTopology::LineList:
                    return GL_LINES;
                case wgpu::PrimitiveTopology::LineStrip:
                    return GL_LINE_STRIP;
                case wgpu::PrimitiveTopology::TriangleList:
                    return GL_TRIANGLES;
                case wgpu::PrimitiveTopology::TriangleStrip:
                    return GL_TRIANGLE_STRIP;
                default:
                    UNREACHABLE();
            }
        }

        void ApplyFrontFaceAndCulling(const OpenGLFunctions& gl,
                                      wgpu::FrontFace face,
                                      wgpu::CullMode mode) {
            if (mode == wgpu::CullMode::None) {
                gl.Disable(GL_CULL_FACE);
            } else {
                gl.Enable(GL_CULL_FACE);
                // Note that we invert winding direction in OpenGL. Because Y axis is up in OpenGL,
                // which is different from WebGPU and other backends (Y axis is down).
                GLenum direction = (face == wgpu::FrontFace::CCW) ? GL_CW : GL_CCW;
                gl.FrontFace(direction);

                GLenum cullMode = (mode == wgpu::CullMode::Front) ? GL_FRONT : GL_BACK;
                gl.CullFace(cullMode);
            }
        }

        GLenum GLBlendFactor(wgpu::BlendFactor factor, bool alpha) {
            switch (factor) {
                case wgpu::BlendFactor::Zero:
                    return GL_ZERO;
                case wgpu::BlendFactor::One:
                    return GL_ONE;
                case wgpu::BlendFactor::SrcColor:
                    return GL_SRC_COLOR;
                case wgpu::BlendFactor::OneMinusSrcColor:
                    return GL_ONE_MINUS_SRC_COLOR;
                case wgpu::BlendFactor::SrcAlpha:
                    return GL_SRC_ALPHA;
                case wgpu::BlendFactor::OneMinusSrcAlpha:
                    return GL_ONE_MINUS_SRC_ALPHA;
                case wgpu::BlendFactor::DstColor:
                    return GL_DST_COLOR;
                case wgpu::BlendFactor::OneMinusDstColor:
                    return GL_ONE_MINUS_DST_COLOR;
                case wgpu::BlendFactor::DstAlpha:
                    return GL_DST_ALPHA;
                case wgpu::BlendFactor::OneMinusDstAlpha:
                    return GL_ONE_MINUS_DST_ALPHA;
                case wgpu::BlendFactor::SrcAlphaSaturated:
                    return GL_SRC_ALPHA_SATURATE;
                case wgpu::BlendFactor::BlendColor:
                    return alpha ? GL_CONSTANT_ALPHA : GL_CONSTANT_COLOR;
                case wgpu::BlendFactor::OneMinusBlendColor:
                    return alpha ? GL_ONE_MINUS_CONSTANT_ALPHA : GL_ONE_MINUS_CONSTANT_COLOR;
                default:
                    UNREACHABLE();
            }
        }

        GLenum GLBlendMode(wgpu::BlendOperation operation) {
            switch (operation) {
                case wgpu::BlendOperation::Add:
                    return GL_FUNC_ADD;
                case wgpu::BlendOperation::Subtract:
                    return GL_FUNC_SUBTRACT;
                case wgpu::BlendOperation::ReverseSubtract:
                    return GL_FUNC_REVERSE_SUBTRACT;
                case wgpu::BlendOperation::Min:
                    return GL_MIN;
                case wgpu::BlendOperation::Max:
                    return GL_MAX;
                default:
                    UNREACHABLE();
            }
        }

        void ApplyColorState(const OpenGLFunctions& gl,
                             uint32_t attachment,
                             const ColorStateDescriptor* descriptor) {
            if (BlendEnabled(descriptor)) {
                gl.Enablei(GL_BLEND, attachment);
                gl.BlendEquationSeparatei(attachment, GLBlendMode(descriptor->colorBlend.operation),
                                          GLBlendMode(descriptor->alphaBlend.operation));
                gl.BlendFuncSeparatei(attachment,
                                      GLBlendFactor(descriptor->colorBlend.srcFactor, false),
                                      GLBlendFactor(descriptor->colorBlend.dstFactor, false),
                                      GLBlendFactor(descriptor->alphaBlend.srcFactor, true),
                                      GLBlendFactor(descriptor->alphaBlend.dstFactor, true));
            } else {
                gl.Disablei(GL_BLEND, attachment);
            }
            gl.ColorMaski(attachment, descriptor->writeMask & wgpu::ColorWriteMask::Red,
                          descriptor->writeMask & wgpu::ColorWriteMask::Green,
                          descriptor->writeMask & wgpu::ColorWriteMask::Blue,
                          descriptor->writeMask & wgpu::ColorWriteMask::Alpha);
        }

        GLuint OpenGLStencilOperation(wgpu::StencilOperation stencilOperation) {
            switch (stencilOperation) {
                case wgpu::StencilOperation::Keep:
                    return GL_KEEP;
                case wgpu::StencilOperation::Zero:
                    return GL_ZERO;
                case wgpu::StencilOperation::Replace:
                    return GL_REPLACE;
                case wgpu::StencilOperation::Invert:
                    return GL_INVERT;
                case wgpu::StencilOperation::IncrementClamp:
                    return GL_INCR;
                case wgpu::StencilOperation::DecrementClamp:
                    return GL_DECR;
                case wgpu::StencilOperation::IncrementWrap:
                    return GL_INCR_WRAP;
                case wgpu::StencilOperation::DecrementWrap:
                    return GL_DECR_WRAP;
                default:
                    UNREACHABLE();
            }
        }

        void ApplyDepthStencilState(const OpenGLFunctions& gl,
                                    const DepthStencilStateDescriptor* descriptor,
                                    PersistentPipelineState* persistentPipelineState) {
            // Depth writes only occur if depth is enabled
            if (descriptor->depthCompare == wgpu::CompareFunction::Always &&
                !descriptor->depthWriteEnabled) {
                gl.Disable(GL_DEPTH_TEST);
            } else {
                gl.Enable(GL_DEPTH_TEST);
            }

            if (descriptor->depthWriteEnabled) {
                gl.DepthMask(GL_TRUE);
            } else {
                gl.DepthMask(GL_FALSE);
            }

            gl.DepthFunc(ToOpenGLCompareFunction(descriptor->depthCompare));

            if (StencilTestEnabled(descriptor)) {
                gl.Enable(GL_STENCIL_TEST);
            } else {
                gl.Disable(GL_STENCIL_TEST);
            }

            GLenum backCompareFunction = ToOpenGLCompareFunction(descriptor->stencilBack.compare);
            GLenum frontCompareFunction = ToOpenGLCompareFunction(descriptor->stencilFront.compare);
            persistentPipelineState->SetStencilFuncsAndMask(
                gl, backCompareFunction, frontCompareFunction, descriptor->stencilReadMask);

            gl.StencilOpSeparate(GL_BACK, OpenGLStencilOperation(descriptor->stencilBack.failOp),
                                 OpenGLStencilOperation(descriptor->stencilBack.depthFailOp),
                                 OpenGLStencilOperation(descriptor->stencilBack.passOp));
            gl.StencilOpSeparate(GL_FRONT, OpenGLStencilOperation(descriptor->stencilFront.failOp),
                                 OpenGLStencilOperation(descriptor->stencilFront.depthFailOp),
                                 OpenGLStencilOperation(descriptor->stencilFront.passOp));

            gl.StencilMask(descriptor->stencilWriteMask);
        }

    }  // anonymous namespace

    RenderPipeline::RenderPipeline(Device* device, const RenderPipelineDescriptor* descriptor)
        : RenderPipelineBase(device, descriptor),
          mVertexArrayObject(0),
          mGlPrimitiveTopology(GLPrimitiveTopology(GetPrimitiveTopology())) {
        PerStage<const ShaderModule*> modules(nullptr);
        modules[SingleShaderStage::Vertex] = ToBackend(descriptor->vertexStage.module);
        modules[SingleShaderStage::Fragment] = ToBackend(descriptor->fragmentStage->module);

        PipelineGL::Initialize(device->gl, ToBackend(GetLayout()), modules);
        CreateVAOForVertexState(descriptor->vertexState);
    }

    RenderPipeline::~RenderPipeline() {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;
        gl.DeleteVertexArrays(1, &mVertexArrayObject);
        gl.BindVertexArray(0);
    }

    GLenum RenderPipeline::GetGLPrimitiveTopology() const {
        return mGlPrimitiveTopology;
    }

    void RenderPipeline::CreateVAOForVertexState(const VertexStateDescriptor* vertexState) {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;

        gl.GenVertexArrays(1, &mVertexArrayObject);
        gl.BindVertexArray(mVertexArrayObject);

        for (uint32_t location : IterateBitSet(GetAttributeLocationsUsed())) {
            const auto& attribute = GetAttribute(location);
            gl.EnableVertexAttribArray(location);

            attributesUsingVertexBuffer[attribute.vertexBufferSlot][location] = true;
            const VertexBufferInfo& vertexBuffer = GetVertexBuffer(attribute.vertexBufferSlot);

            if (vertexBuffer.arrayStride == 0) {
                // Emulate a stride of zero (constant vertex attribute) by
                // setting the attribute instance divisor to a huge number.
                gl.VertexAttribDivisor(location, 0xffffffff);
            } else {
                switch (vertexBuffer.stepMode) {
                    case wgpu::InputStepMode::Vertex:
                        break;
                    case wgpu::InputStepMode::Instance:
                        gl.VertexAttribDivisor(location, 1);
                        break;
                    default:
                        UNREACHABLE();
                }
            }
        }
    }

    void RenderPipeline::ApplyNow(PersistentPipelineState& persistentPipelineState) {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;
        PipelineGL::ApplyNow(gl);

        ASSERT(mVertexArrayObject);
        gl.BindVertexArray(mVertexArrayObject);

        ApplyFrontFaceAndCulling(gl, GetFrontFace(), GetCullMode());

        ApplyDepthStencilState(gl, GetDepthStencilStateDescriptor(), &persistentPipelineState);

        gl.SampleMaski(0, GetSampleMask());

        for (uint32_t attachmentSlot : IterateBitSet(GetColorAttachmentsMask())) {
            ApplyColorState(gl, attachmentSlot, GetColorStateDescriptor(attachmentSlot));
        }
    }

}}  // namespace dawn_native::opengl
