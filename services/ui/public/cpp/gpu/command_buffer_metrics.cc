// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/command_buffer_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace ui {
namespace command_buffer_metrics {

namespace {

enum CommandBufferContextLostReason {
  // Don't add new values here.
  CONTEXT_INIT_FAILED,
  CONTEXT_LOST_GPU_CHANNEL_ERROR,
  CONTEXT_PARSE_ERROR_INVALID_SIZE,
  CONTEXT_PARSE_ERROR_OUT_OF_BOUNDS,
  CONTEXT_PARSE_ERROR_UNKNOWN_COMMAND,
  CONTEXT_PARSE_ERROR_INVALID_ARGS,
  CONTEXT_PARSE_ERROR_GENERIC_ERROR,
  CONTEXT_LOST_GUILTY,
  CONTEXT_LOST_INNOCENT,
  CONTEXT_LOST_UNKNOWN,
  CONTEXT_LOST_OUT_OF_MEMORY,
  CONTEXT_LOST_MAKECURRENT_FAILED,
  CONTEXT_LOST_INVALID_GPU_MESSAGE,
  // Add new values here and update _MAX_ENUM.
  // Also update //tools/metrics/histograms/histograms.xml
  CONTEXT_LOST_REASON_MAX_ENUM = CONTEXT_LOST_INVALID_GPU_MESSAGE
};

CommandBufferContextLostReason GetContextLostReason(
    gpu::error::Error error,
    gpu::error::ContextLostReason reason) {
  if (error == gpu::error::kLostContext) {
    switch (reason) {
      case gpu::error::kGuilty:
        return CONTEXT_LOST_GUILTY;
      case gpu::error::kInnocent:
        return CONTEXT_LOST_INNOCENT;
      case gpu::error::kUnknown:
        return CONTEXT_LOST_UNKNOWN;
      case gpu::error::kOutOfMemory:
        return CONTEXT_LOST_OUT_OF_MEMORY;
      case gpu::error::kMakeCurrentFailed:
        return CONTEXT_LOST_MAKECURRENT_FAILED;
      case gpu::error::kGpuChannelLost:
        return CONTEXT_LOST_GPU_CHANNEL_ERROR;
      case gpu::error::kInvalidGpuMessage:
        return CONTEXT_LOST_INVALID_GPU_MESSAGE;
    }
  }
  switch (error) {
    case gpu::error::kInvalidSize:
      return CONTEXT_PARSE_ERROR_INVALID_SIZE;
    case gpu::error::kOutOfBounds:
      return CONTEXT_PARSE_ERROR_OUT_OF_BOUNDS;
    case gpu::error::kUnknownCommand:
      return CONTEXT_PARSE_ERROR_UNKNOWN_COMMAND;
    case gpu::error::kInvalidArguments:
      return CONTEXT_PARSE_ERROR_INVALID_ARGS;
    case gpu::error::kGenericError:
      return CONTEXT_PARSE_ERROR_GENERIC_ERROR;
    case gpu::error::kDeferCommandUntilLater:
    case gpu::error::kDeferLaterCommands:
    case gpu::error::kNoError:
    case gpu::error::kLostContext:
      NOTREACHED();
      return CONTEXT_LOST_UNKNOWN;
  }
  NOTREACHED();
  return CONTEXT_LOST_UNKNOWN;
}

void RecordContextLost(ContextType type,
                       CommandBufferContextLostReason reason) {
  switch (type) {
    case DISPLAY_COMPOSITOR_ONSCREEN_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.BrowserCompositor", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case BROWSER_OFFSCREEN_MAINTHREAD_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.BrowserMainThread", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case BROWSER_WORKER_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.BrowserWorker", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case RENDER_COMPOSITOR_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.RenderCompositor", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case RENDER_WORKER_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.RenderWorker", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case RENDERER_MAINTHREAD_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.RenderMainThread", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case GPU_VIDEO_ACCELERATOR_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.VideoAccelerator", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case OFFSCREEN_VIDEO_CAPTURE_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.VideoCapture", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case OFFSCREEN_CONTEXT_FOR_WEBGL:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.WebGL", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case MEDIA_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.Media", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case MUS_CLIENT_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.MusClient", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case UI_COMPOSITOR_CONTEXT:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.UICompositor", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
    case CONTEXT_TYPE_UNKNOWN:
      UMA_HISTOGRAM_ENUMERATION("GPU.ContextLost.Unknown", reason,
                                CONTEXT_LOST_REASON_MAX_ENUM);
      break;
  }
}

}  // anonymous namespace

std::string ContextTypeToString(ContextType type) {
  switch (type) {
    case OFFSCREEN_CONTEXT_FOR_TESTING:
      return "Context-For-Testing";
    case DISPLAY_COMPOSITOR_ONSCREEN_CONTEXT:
      return "DisplayCompositor";
    case BROWSER_OFFSCREEN_MAINTHREAD_CONTEXT:
      return "Offscreen-MainThread";
    case BROWSER_WORKER_CONTEXT:
      return "CompositorWorker";
    case RENDER_COMPOSITOR_CONTEXT:
      return "RenderCompositor";
    case RENDER_WORKER_CONTEXT:
      return "RenderWorker";
    case RENDERER_MAINTHREAD_CONTEXT:
      return "Offscreen-MainThread";
    case GPU_VIDEO_ACCELERATOR_CONTEXT:
      return "GPU-VideoAccelerator-Offscreen";
    case OFFSCREEN_VIDEO_CAPTURE_CONTEXT:
      return "Offscreen-CaptureThread";
    case OFFSCREEN_CONTEXT_FOR_WEBGL:
      return "Offscreen-For-WebGL";
    case MEDIA_CONTEXT:
      return "Media";
    case MUS_CLIENT_CONTEXT:
      return "MusClient";
    case UI_COMPOSITOR_CONTEXT:
      return "UICompositor";
    default:
      NOTREACHED();
      return "unknown";
  }
}

void UmaRecordContextInitFailed(ContextType type) {
  RecordContextLost(type, CONTEXT_INIT_FAILED);
}

void UmaRecordContextLost(ContextType type,
                          gpu::error::Error error,
                          gpu::error::ContextLostReason reason) {
  CommandBufferContextLostReason converted_reason =
      GetContextLostReason(error, reason);
  RecordContextLost(type, converted_reason);
}

}  // namespace command_buffer_metrics
}  // namespace ui
