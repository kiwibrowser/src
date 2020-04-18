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

/*
This file renders a passed in XRStageBounds object and attempts
to render geometry on the floor to indicate where the bounds is.
XRStageBounds' `geometry` is a series of XRStageBoundsPoints (in
clockwise-order) with `x` and `z` properties for each.
*/

import {Material} from '../core/material.js';
import {Node} from '../core/node.js';
import {Primitive, PrimitiveAttribute} from '../core/primitive.js';

const GL = WebGLRenderingContext; // For enums

class BoundsMaterial extends Material {
  constructor() {
    super();

    this.state.blend = true;
    this.state.blendFuncSrc = GL.SRC_ALPHA;
    this.state.blendFuncDst = GL.ONE;
    this.state.depthTest = false;
  }

  get materialName() {
    return 'BOUNDS_RENDERER';
  }

  get vertexSource() {
    return `
    attribute vec2 POSITION;

    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {
      return proj * view * model * vec4(POSITION, 1.0);
    }`;
  }

  get fragmentSource() {
    return `
    precision mediump float;

    vec4 fragment_main() {
      return vec4(0.0, 1.0, 0.0, 0.3);
    }`;
  }
}

export class BoundsRenderer extends Node {
  constructor() {
    super();

    this._stageBounds = null;
  }

  onRendererChanged(renderer) {
    this.stageBounds = this._stageBounds;
  }

  get stageBounds() {
    return this._stageBounds;
  }

  set stageBounds(stageBounds) {
    if (this._stageBounds) {
      this.clearRenderPrimitives();
    }
    this._stageBounds = stageBounds;
    if (!stageBounds || stageBounds.length === 0 || !this._renderer) {
      return;
    }

    let verts = [];
    let indices = [];

    // Tessellate the bounding points from XRStageBounds and connect
    // each point to a neighbor and 0,0,0.
    const pointCount = stageBounds.geometry.length;
    for (let i = 0; i < pointCount; i++) {
      const point = stageBounds.geometry[i];
      verts.push(point.x, 0, point.z);
      indices.push(i, i === 0 ? pointCount - 1 : i - 1, pointCount);
    }
    // Center point
    verts.push(0, 0, 0);

    let vertexBuffer = this._renderer.createRenderBuffer(GL.ARRAY_BUFFER, new Float32Array(verts));
    let indexBuffer = this._renderer.createRenderBuffer(GL.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices));

    let attribs = [
      new PrimitiveAttribute('POSITION', vertexBuffer, 3, GL.FLOAT, 12, 0),
    ];

    let primitive = new Primitive(attribs, indices.length);
    primitive.setIndexBuffer(indexBuffer);

    let renderPrimitive = this._renderer.createRenderPrimitive(primitive, new BoundsMaterial());
    this.addRenderPrimitive(renderPrimitive);
  }
}
