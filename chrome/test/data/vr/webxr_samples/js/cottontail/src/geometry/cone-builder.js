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

export class ConeBuilder extends GeometryBuilderBase {
  pushCone(size = 0.5) {
    let stream = this.primitiveStream;
    let coneSegments = 64;

    stream.startGeometry();

    // Cone side vertices
    for (let i = 0; i < coneSegments; ++i) {
      let idx = stream.nextVertexIndex;

      stream.pushTriangle(idx, idx + 1, idx + 2);

      let rad = ((Math.PI * 2) / coneSegments) * i;
      let rad2 = ((Math.PI * 2) / coneSegments) * (i + 1);

      stream.pushVertex(
          Math.sin(rad) * (size / 2), -size, Math.cos(rad) * (size / 2),
          i / coneSegments, 0.0,
          Math.sin(rad), 0.25, Math.cos(rad));

      stream.pushVertex(
          Math.sin(rad2) * (size / 2), -size, Math.cos(rad2) * (size / 2),
          i / coneSegments, 0.0,
          Math.sin(rad2), 0.25, Math.cos(rad2));

      stream.pushVertex(
          0, size, 0,
          i / coneSegments, 1.0,
          Math.sin((rad + rad2) / 2), 0.25, Math.cos((rad + rad2) / 2));
    }

    // Base triangles
    let baseCenterIndex = stream.nextVertexIndex;
    stream.pushVertex(
        0, -size, 0,
        0.5, 0.5,
        0, -1, 0);
    for (let i = 0; i < coneSegments; ++i) {
      let idx = stream.nextVertexIndex;
      stream.pushTriangle(baseCenterIndex, idx, idx + 1);
      let rad = ((Math.PI * 2) / coneSegments) * i;
      let rad2 = ((Math.PI * 2) / coneSegments) * (i + 1);
      stream.pushVertex(
          Math.sin(rad2) * (size / 2.0), -size, Math.cos(rad2) * (size / 2.0),
          (Math.sin(rad2) + 1.0) * 0.5, (Math.cos(rad2) + 1.0) * 0.5,
          0, -1, 0);
      stream.pushVertex(
          Math.sin(rad) * (size / 2.0), -size, Math.cos(rad) * (size / 2.0),
          (Math.sin(rad) + 1.0) * 0.5, (Math.cos(rad) + 1.0) * 0.5,
          0, -1, 0);
    }

    stream.endGeometry();
  }
}
