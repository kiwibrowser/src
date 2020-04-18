/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "WebRTC/RTCMTLVideoView.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#import "WebRTC/RTCLogging.h"
#import "WebRTC/RTCVideoFrame.h"
#import "WebRTC/RTCVideoFrameBuffer.h"

#import "RTCMTLI420Renderer.h"
#import "RTCMTLNV12Renderer.h"
#import "RTCMTLRGBRenderer.h"

// To avoid unreconized symbol linker errors, we're taking advantage of the objc runtime.
// Linking errors occur when compiling for architectures that don't support Metal.
#define MTKViewClass NSClassFromString(@"MTKView")
#define RTCMTLNV12RendererClass NSClassFromString(@"RTCMTLNV12Renderer")
#define RTCMTLI420RendererClass NSClassFromString(@"RTCMTLI420Renderer")
#define RTCMTLRGBRendererClass NSClassFromString(@"RTCMTLRGBRenderer")

@interface RTCMTLVideoView () <MTKViewDelegate>
@property(nonatomic, strong) RTCMTLI420Renderer *rendererI420;
@property(nonatomic, strong) RTCMTLNV12Renderer *rendererNV12;
@property(nonatomic, strong) RTCMTLRGBRenderer *rendererRGB;
@property(nonatomic, strong) MTKView *metalView;
@property(atomic, strong) RTCVideoFrame *videoFrame;
@end

@implementation RTCMTLVideoView {
  int64_t _lastFrameTimeNs;
  CGSize _videoFrameSize;
}

@synthesize delegate = _delegate;
@synthesize rendererI420 = _rendererI420;
@synthesize rendererNV12 = _rendererNV12;
@synthesize rendererRGB = _rendererRGB;
@synthesize metalView = _metalView;
@synthesize videoFrame = _videoFrame;

- (instancetype)initWithFrame:(CGRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
    [self configure];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder *)aCoder {
  self = [super initWithCoder:aCoder];
  if (self) {
    [self configure];
  }
  return self;
}

#pragma mark - Private

+ (BOOL)isMetalAvailable {
#if defined(RTC_SUPPORTS_METAL)
  return MTLCreateSystemDefaultDevice() != nil;
#else
  return NO;
#endif
}

+ (MTKView *)createMetalView:(CGRect)frame {
  MTKView *view = [[MTKViewClass alloc] initWithFrame:frame];
  return view;
}

+ (RTCMTLNV12Renderer *)createNV12Renderer {
  return [[RTCMTLNV12RendererClass alloc] init];
}

+ (RTCMTLI420Renderer *)createI420Renderer {
  return [[RTCMTLI420RendererClass alloc] init];
}

+ (RTCMTLRGBRenderer *)createRGBRenderer {
  return [[RTCMTLRGBRenderer alloc] init];
}

- (void)configure {
  NSAssert([RTCMTLVideoView isMetalAvailable], @"Metal not availiable on this device");

  _metalView = [RTCMTLVideoView createMetalView:self.bounds];
  [self configureMetalView];
}

- (void)configureMetalView {
  if (_metalView) {
    _metalView.delegate = self;
    [self addSubview:_metalView];
    _metalView.contentMode = UIViewContentModeScaleAspectFit;
    _videoFrameSize = CGSizeZero;
  }
}

- (void)setVideoContentMode:(UIViewContentMode)mode {
  _metalView.contentMode = mode;
}

#pragma mark - Private

- (void)layoutSubviews {
  [super layoutSubviews];
  CGRect bounds = self.bounds;
  _metalView.frame = bounds;
  if (!CGSizeEqualToSize(_videoFrameSize, CGSizeZero)) {
    _metalView.drawableSize = _videoFrameSize;
  } else {
    _metalView.drawableSize = bounds.size;
  }
}

#pragma mark - MTKViewDelegate methods

- (void)drawInMTKView:(nonnull MTKView *)view {
  NSAssert(view == self.metalView, @"Receiving draw callbacks from foreign instance.");
  RTCVideoFrame *videoFrame = self.videoFrame;
  // Skip rendering if we've already rendered this frame.
  if (!videoFrame || videoFrame.timeStampNs == _lastFrameTimeNs) {
    return;
  }

  if ([videoFrame.buffer isKindOfClass:[RTCCVPixelBuffer class]]) {
    RTCCVPixelBuffer *buffer = (RTCCVPixelBuffer*)videoFrame.buffer;
    const OSType pixelFormat = CVPixelBufferGetPixelFormatType(buffer.pixelBuffer);
    if (pixelFormat == kCVPixelFormatType_32BGRA || pixelFormat == kCVPixelFormatType_32ARGB) {
      if (!self.rendererRGB) {
        self.rendererRGB = [RTCMTLVideoView createRGBRenderer];
        if (![self.rendererRGB addRenderingDestination:self.metalView]) {
          self.rendererRGB = nil;
          RTCLogError(@"Failed to create RGB renderer");
          return;
        }
      }
      [self.rendererRGB drawFrame:videoFrame];
    } else {
      if (!self.rendererNV12) {
        self.rendererNV12 = [RTCMTLVideoView createNV12Renderer];
        if (![self.rendererNV12 addRenderingDestination:self.metalView]) {
          self.rendererNV12 = nil;
          RTCLogError(@"Failed to create NV12 renderer");
          return;
        }
      }
      [self.rendererNV12 drawFrame:videoFrame];
    }
  } else {
    if (!self.rendererI420) {
      self.rendererI420 = [RTCMTLVideoView createI420Renderer];
      if (![self.rendererI420 addRenderingDestination:self.metalView]) {
        self.rendererI420 = nil;
        RTCLogError(@"Failed to create I420 renderer");
        return;
      }
    }
    [self.rendererI420 drawFrame:videoFrame];
  }
  _lastFrameTimeNs = videoFrame.timeStampNs;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}

#pragma mark - RTCVideoRenderer

- (void)setSize:(CGSize)size {
  self.metalView.drawableSize = size;
  dispatch_async(dispatch_get_main_queue(), ^{
    _videoFrameSize = size;
    [self.delegate videoView:self didChangeVideoSize:size];
  });
}

- (void)renderFrame:(nullable RTCVideoFrame *)frame {
  if (frame == nil) {
    RTCLogInfo(@"Incoming frame is nil. Exiting render callback.");
    return;
  }
  self.videoFrame = frame;
}

@end
