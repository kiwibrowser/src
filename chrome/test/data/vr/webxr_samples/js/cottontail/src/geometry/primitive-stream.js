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

import {Primitive, PrimitiveAttribute} from '../core/primitive.js';
import {mat3, vec3} from '../math/gl-matrix.js';

const GL = WebGLRenderingContext; // For enums

const tempVec3 = vec3.create();

export class PrimitiveStream {
  constructor(options) {
    this._vertices = [];
    this._indices = [];

    this._geometryStarted = false;

    this._vertexOffset = 0;
    this._vertexIndex = 0;
    this._highIndex = 0;

    this._flipWinding = false;
    this._invertNormals = false;
    this._transform = null;
    this._normalTransform = null;
    this._min = null;
    this._max = null;
  }

  set flipWinding(value) {
    if (this._geometryStarted) {
      throw new Error(`Cannot change flipWinding before ending the current geometry.`);
    }
    this._flipWinding = value;
  }

  get flipWinding() {
    this._flipWinding;
  }

  set invertNormals(value) {
    if (this._geometryStarted) {
      throw new Error(`Cannot change invertNormals before ending the current geometry.`);
    }
    this._invertNormals = value;
  }

  get invertNormals() {
    this._invertNormals;
  }

  set transform(value) {
    if (this._geometryStarted) {
      throw new Error(`Cannot change transform before ending the current geometry.`);
    }
    this._transform = value;
    if (this._transform) {
      if (!this._normalTransform) {
        this._normalTransform = mat3.create();
      }
      mat3.fromMat4(this._normalTransform, this._transform);
    }
  }

  get transform() {
    this._transform;
  }

  startGeometry() {
    if (this._geometryStarted) {
      throw new Error(`Attempted to start a new geometry before the previous one was ended.`);
    }

    this._geometryStarted = true;
    this._vertexIndex = 0;
    this._highIndex = 0;
  }

  endGeometry() {
    if (!this._geometryStarted) {
      throw new Error(`Attempted to end a geometry before one was started.`);
    }

    if (this._highIndex >= this._vertexIndex) {
      throw new Error(`Geometry contains indices that are out of bounds.
                       (Contains an index of ${this._highIndex} when the vertex count is ${this._vertexIndex})`);
    }

    this._geometryStarted = false;
    this._vertexOffset += this._vertexIndex;

    // TODO: Anything else need to be done to finish processing here?
  }

  pushVertex(x, y, z, u, v, nx, ny, nz) {
    if (!this._geometryStarted) {
      throw new Error(`Cannot push vertices before calling startGeometry().`);
    }

    // Transform the incoming vertex if we have a transformation matrix
    if (this._transform) {
      tempVec3[0] = x;
      tempVec3[1] = y;
      tempVec3[2] = z;
      vec3.transformMat4(tempVec3, tempVec3, this._transform);
      x = tempVec3[0];
      y = tempVec3[1];
      z = tempVec3[2];

      tempVec3[0] = nx;
      tempVec3[1] = ny;
      tempVec3[2] = nz;
      vec3.transformMat3(tempVec3, tempVec3, this._normalTransform);
      nx = tempVec3[0];
      ny = tempVec3[1];
      nz = tempVec3[2];
    }

    if (this._invertNormals) {
      nx *= -1.0;
      ny *= -1.0;
      nz *= -1.0;
    }

    this._vertices.push(x, y, z, u, v, nx, ny, nz);

    if (this._min) {
      this._min[0] = Math.min(this._min[0], x);
      this._min[1] = Math.min(this._min[1], y);
      this._min[2] = Math.min(this._min[2], z);
      this._max[0] = Math.max(this._max[0], x);
      this._max[1] = Math.max(this._max[1], y);
      this._max[2] = Math.max(this._max[2], z);
    } else {
      this._min = vec3.fromValues(x, y, z);
      this._max = vec3.fromValues(x, y, z);
    }

    return this._vertexIndex++;
  }

  get nextVertexIndex() {
    return this._vertexIndex;
  }

  pushTriangle(idxA, idxB, idxC) {
    if (!this._geometryStarted) {
      throw new Error(`Cannot push triangles before calling startGeometry().`);
    }

    this._highIndex = Math.max(this._highIndex, idxA, idxB, idxC);

    idxA += this._vertexOffset;
    idxB += this._vertexOffset;
    idxC += this._vertexOffset;

    if (this._flipWinding) {
      this._indices.push(idxC, idxB, idxA);
    } else {
      this._indices.push(idxA, idxB, idxC);
    }
  }

  clear() {
    if (this._geometryStarted) {
      throw new Error(`Cannot clear before ending the current geometry.`);
    }

    this._vertices = [];
    this._indices = [];
    this._vertexOffset = 0;
    this._min = null;
    this._max = null;
  }

  finishPrimitive(renderer) {
    if (!this._vertexOffset) {
      throw new Error(`Attempted to call finishPrimitive() before creating any geometry.`);
    }

    let vertexBuffer = renderer.createRenderBuffer(GL.ARRAY_BUFFER, new Float32Array(this._vertices));
    let indexBuffer = renderer.createRenderBuffer(GL.ELEMENT_ARRAY_BUFFER, new Uint16Array(this._indices));

    let attribs = [
      new PrimitiveAttribute('POSITION', vertexBuffer, 3, GL.FLOAT, 32, 0),
      new PrimitiveAttribute('TEXCOORD_0', vertexBuffer, 2, GL.FLOAT, 32, 12),
      new PrimitiveAttribute('NORMAL', vertexBuffer, 3, GL.FLOAT, 32, 20),
    ];

    let primitive = new Primitive(attribs, this._indices.length);
    primitive.setIndexBuffer(indexBuffer);
    primitive.setBounds(this._min, this._max);

    return primitive;
  }
}

export class GeometryBuilderBase {
  constructor(primitiveStream) {
    if (primitiveStream) {
      this._stream = primitiveStream;
    } else {
      this._stream = new PrimitiveStream();
    }
  }

  set primitiveStream(value) {
    this._stream = value;
  }

  get primitiveStream() {
    return this._stream;
  }

  finishPrimitive(renderer) {
    return this._stream.finishPrimitive(renderer);
  }

  clear() {
    this._stream.clear();
  }
}
