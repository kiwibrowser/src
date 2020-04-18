// Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import {GeometryBuilderBase} from './primitive-stream.js';

export class BoxBuilder extends GeometryBuilderBase {
  pushBox(min, max) {
    let stream = this.primitiveStream;

    let w = max[0] - min[0];
    let h = max[1] - min[1];
    let d = max[2] - min[2];

    let wh = w * 0.5;
    let hh = h * 0.5;
    let dh = d * 0.5;

    let cx = min[0] + wh;
    let cy = min[1] + hh;
    let cz = min[2] + dh;

    stream.startGeometry();

    // Bottom
    let idx = stream.nextVertexIndex;
    stream.pushTriangle(idx, idx+1, idx+2);
    stream.pushTriangle(idx, idx+2, idx+3);

    //                 X       Y       Z      U    V    NX    NY   NZ
    stream.pushVertex(-wh+cx, -hh+cy, -dh+cz, 0.0, 1.0, 0.0, -1.0, 0.0);
    stream.pushVertex(+wh+cx, -hh+cy, -dh+cz, 1.0, 1.0, 0.0, -1.0, 0.0);
    stream.pushVertex(+wh+cx, -hh+cy, +dh+cz, 1.0, 0.0, 0.0, -1.0, 0.0);
    stream.pushVertex(-wh+cx, -hh+cy, +dh+cz, 0.0, 0.0, 0.0, -1.0, 0.0);

    // Top
    idx = stream.nextVertexIndex;
    stream.pushTriangle(idx, idx+2, idx+1);
    stream.pushTriangle(idx, idx+3, idx+2);

    stream.pushVertex(-wh+cx, +hh+cy, -dh+cz, 0.0, 0.0, 0.0, 1.0, 0.0);
    stream.pushVertex(+wh+cx, +hh+cy, -dh+cz, 1.0, 0.0, 0.0, 1.0, 0.0);
    stream.pushVertex(+wh+cx, +hh+cy, +dh+cz, 1.0, 1.0, 0.0, 1.0, 0.0);
    stream.pushVertex(-wh+cx, +hh+cy, +dh+cz, 0.0, 1.0, 0.0, 1.0, 0.0);

    // Left
    idx = stream.nextVertexIndex;
    stream.pushTriangle(idx, idx+2, idx+1);
    stream.pushTriangle(idx, idx+3, idx+2);

    stream.pushVertex(-wh+cx, -hh+cy, -dh+cz, 0.0, 1.0, -1.0, 0.0, 0.0);
    stream.pushVertex(-wh+cx, +hh+cy, -dh+cz, 0.0, 0.0, -1.0, 0.0, 0.0);
    stream.pushVertex(-wh+cx, +hh+cy, +dh+cz, 1.0, 0.0, -1.0, 0.0, 0.0);
    stream.pushVertex(-wh+cx, -hh+cy, +dh+cz, 1.0, 1.0, -1.0, 0.0, 0.0);

    // Right
    idx = stream.nextVertexIndex;
    stream.pushTriangle(idx, idx+1, idx+2);
    stream.pushTriangle(idx, idx+2, idx+3);

    stream.pushVertex(+wh+cx, -hh+cy, -dh+cz, 1.0, 1.0, 1.0, 0.0, 0.0);
    stream.pushVertex(+wh+cx, +hh+cy, -dh+cz, 1.0, 0.0, 1.0, 0.0, 0.0);
    stream.pushVertex(+wh+cx, +hh+cy, +dh+cz, 0.0, 0.0, 1.0, 0.0, 0.0);
    stream.pushVertex(+wh+cx, -hh+cy, +dh+cz, 0.0, 1.0, 1.0, 0.0, 0.0);

    // Back
    idx = stream.nextVertexIndex;
    stream.pushTriangle(idx, idx+2, idx+1);
    stream.pushTriangle(idx, idx+3, idx+2);

    stream.pushVertex(-wh+cx, -hh+cy, -dh+cz, 1.0, 1.0, 0.0, 0.0, -1.0);
    stream.pushVertex(+wh+cx, -hh+cy, -dh+cz, 0.0, 1.0, 0.0, 0.0, -1.0);
    stream.pushVertex(+wh+cx, +hh+cy, -dh+cz, 0.0, 0.0, 0.0, 0.0, -1.0);
    stream.pushVertex(-wh+cx, +hh+cy, -dh+cz, 1.0, 0.0, 0.0, 0.0, -1.0);

    // Front
    idx = stream.nextVertexIndex;
    stream.pushTriangle(idx, idx+1, idx+2);
    stream.pushTriangle(idx, idx+2, idx+3);

    stream.pushVertex(-wh+cx, -hh+cy, +dh+cz, 0.0, 1.0, 0.0, 0.0, 1.0);
    stream.pushVertex(+wh+cx, -hh+cy, +dh+cz, 1.0, 1.0, 0.0, 0.0, 1.0);
    stream.pushVertex(+wh+cx, +hh+cy, +dh+cz, 1.0, 0.0, 0.0, 0.0, 1.0);
    stream.pushVertex(-wh+cx, +hh+cy, +dh+cz, 0.0, 0.0, 0.0, 0.0, 1.0);

    stream.endGeometry();
  }

  pushCube(center = [0, 0, 0], size = 1.0) {
    let hs = size * 0.5;
    this.pushBox([center[0] - hs, center[1] - hs, center[2] - hs],
                 [center[0] + hs, center[1] + hs, center[2] + hs]);
  }
}
