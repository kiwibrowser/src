// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.piet.ui;

import com.google.search.now.ui.piet.RoundedCornersProto.RoundedCorners.Corners;
import java.util.Arrays;

/** Helper class to help work with rounded corner views. */
public class RoundedCornerViewHelper {

  public static final int TOP_CORNERS = Corners.TOP_LEFT_VALUE | Corners.TOP_RIGHT_VALUE;
  public static final int BOTTOM_CORNERS = Corners.BOTTOM_LEFT_VALUE | Corners.BOTTOM_RIGHT_VALUE;

  static float[] createRoundedCornerMask(float radius, int cornerField) {
    float[] radii = new float[8];
    // If we don't have any radius, don't bother creating the mask.
    if (radius > 0) {
      if (cornerField == Corners.CORNERS_UNSPECIFIED_VALUE) {
        Arrays.fill(radii, 0, 8, radius);
        return radii;
      }

      if ((cornerField & Corners.TOP_LEFT_VALUE) != 0) {
        radii[0] = radius;
        radii[1] = radius;
      }
      if ((cornerField & Corners.TOP_RIGHT_VALUE) != 0) {
        radii[2] = radius;
        radii[3] = radius;
      }
      if ((cornerField & Corners.BOTTOM_RIGHT_VALUE) != 0) {
        radii[4] = radius;
        radii[5] = radius;
      }
      if ((cornerField & Corners.BOTTOM_LEFT_VALUE) != 0) {
        radii[6] = radius;
        radii[7] = radius;
      }
    }

    return radii;
  }
}
