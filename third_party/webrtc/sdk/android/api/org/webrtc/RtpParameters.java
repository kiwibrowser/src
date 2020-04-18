/*
 *  Copyright 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import javax.annotation.Nullable;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.webrtc.MediaStreamTrack;

/**
 * The parameters for an {@code RtpSender}, as defined in
 * http://w3c.github.io/webrtc-pc/#rtcrtpsender-interface.
 *
 * Note: These structures use nullable Integer/etc. types because in the
 * future, they may be used to construct ORTC RtpSender/RtpReceivers, in
 * which case "null" will be used to represent "choose the implementation
 * default value".
 */
public class RtpParameters {
  public static class Encoding {
    // Set to true to cause this encoding to be sent, and false for it not to
    // be sent.
    public boolean active = true;
    // If non-null, this represents the Transport Independent Application
    // Specific maximum bandwidth defined in RFC3890. If null, there is no
    // maximum bitrate.
    @Nullable public Integer maxBitrateBps;
    // SSRC to be used by this encoding.
    // Can't be changed between getParameters/setParameters.
    public Long ssrc;

    @CalledByNative("Encoding")
    Encoding(boolean active, Integer maxBitrateBps, Long ssrc) {
      this.active = active;
      this.maxBitrateBps = maxBitrateBps;
      this.ssrc = ssrc;
    }

    @CalledByNative("Encoding")
    boolean getActive() {
      return active;
    }

    @Nullable
    @CalledByNative("Encoding")
    Integer getMaxBitrateBps() {
      return maxBitrateBps;
    }

    @CalledByNative("Encoding")
    Long getSsrc() {
      return ssrc;
    }
  }

  public static class Codec {
    // Payload type used to identify this codec in RTP packets.
    public int payloadType;
    // Name used to identify the codec. Equivalent to MIME subtype.
    public String name;
    // The media type of this codec. Equivalent to MIME top-level type.
    MediaStreamTrack.MediaType kind;
    // Clock rate in Hertz.
    public Integer clockRate;
    // The number of audio channels used. Set to null for video codecs.
    public Integer numChannels;
    // The "format specific parameters" field from the "a=fmtp" line in the SDP
    public Map<String, String> parameters;

    @CalledByNative("Codec")
    Codec(int payloadType, String name, MediaStreamTrack.MediaType kind, Integer clockRate,
        Integer numChannels, Map<String, String> parameters) {
      this.payloadType = payloadType;
      this.name = name;
      this.kind = kind;
      this.clockRate = clockRate;
      this.numChannels = numChannels;
      this.parameters = parameters;
    }

    @CalledByNative("Codec")
    int getPayloadType() {
      return payloadType;
    }

    @CalledByNative("Codec")
    String getName() {
      return name;
    }

    @CalledByNative("Codec")
    MediaStreamTrack.MediaType getKind() {
      return kind;
    }

    @CalledByNative("Codec")
    Integer getClockRate() {
      return clockRate;
    }

    @CalledByNative("Codec")
    Integer getNumChannels() {
      return numChannels;
    }

    @CalledByNative("Codec")
    Map getParameters() {
      return parameters;
    }
  }

  public final String transactionId;

  public final List<Encoding> encodings;
  // Codec parameters can't currently be changed between getParameters and
  // setParameters. Though in the future it will be possible to reorder them or
  // remove them.
  public final List<Codec> codecs;

  @CalledByNative
  RtpParameters(String transactionId, List<Encoding> encodings, List<Codec> codecs) {
    this.transactionId = transactionId;
    this.encodings = encodings;
    this.codecs = codecs;
  }

  @CalledByNative
  String getTransactionId() {
    return transactionId;
  }

  @CalledByNative
  List<Encoding> getEncodings() {
    return encodings;
  }

  @CalledByNative
  List<Codec> getCodecs() {
    return codecs;
  }
}
