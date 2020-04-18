/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.List;
import javax.annotation.Nullable;

public class DefaultVideoDecoderFactory implements VideoDecoderFactory {
  private final HardwareVideoDecoderFactory hardwareVideoDecoderFactory;
  private final SoftwareVideoDecoderFactory softwareVideoDecoderFactory;

  public DefaultVideoDecoderFactory(EglBase.Context eglContext) {
    hardwareVideoDecoderFactory =
        new HardwareVideoDecoderFactory(eglContext, false /* fallbackToSoftware */);
    softwareVideoDecoderFactory = new SoftwareVideoDecoderFactory();
  }

  @Override
  public @Nullable VideoDecoder createDecoder(String codecType) {
    VideoDecoder decoder = hardwareVideoDecoderFactory.createDecoder(codecType);
    if (decoder != null) {
      return decoder;
    }
    return softwareVideoDecoderFactory.createDecoder(codecType);
  }

  @Override
  public VideoCodecInfo[] getSupportedCodecs() {
    LinkedHashSet<VideoCodecInfo> supportedCodecInfos = new LinkedHashSet<VideoCodecInfo>();

    supportedCodecInfos.addAll(Arrays.asList(softwareVideoDecoderFactory.getSupportedCodecs()));
    supportedCodecInfos.addAll(Arrays.asList(hardwareVideoDecoderFactory.getSupportedCodecs()));

    return supportedCodecInfos.toArray(new VideoCodecInfo[supportedCodecInfos.size()]);
  }
}
