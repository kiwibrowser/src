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
Renders simple text using a seven-segment LED style pattern. Only really good
for numbers and a limited number of other characters.
*/

import {Material} from '../core/material.js';
import {Node} from '../core/node.js';
import {Primitive, PrimitiveAttribute} from '../core/primitive.js';

const TEXT_KERNING = 2.0;

class SevenSegmentMaterial extends Material {
  get materialName() {
    return 'SEVEN_SEGMENT_TEXT';
  }

  get vertexSource() {
    return `
    attribute vec2 POSITION;

    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {
      return proj * view * model * vec4(POSITION, 0.0, 1.0);
    }`;
  }

  get fragmentSource() {
    return `
    precision mediump float;
    const vec4 color = vec4(0.0, 1.0, 0.0, 1.0);

    vec4 fragment_main() {
      return color;
    }`;
  }
}

export class SevenSegmentText extends Node {
  constructor() {
    super();

    this._text = '';
    this._charNodes = [];
  }

  onRendererChanged(renderer) {
    this.clearNodes();
    this._charNodes = [];

    let vertices = [];
    let segmentIndices = {};
    let indices = [];

    const width = 0.5;
    const thickness = 0.25;

    function defineSegment(id, left, top, right, bottom) {
      let idx = vertices.length / 2;
      vertices.push(
          left, top,
          right, top,
          right, bottom,
          left, bottom);

      segmentIndices[id] = [
        idx, idx+2, idx+1,
        idx, idx+3, idx+2,
      ];
    }

    let characters = {};
    function defineCharacter(c, segments) {
      let character = {
        character: c,
        offset: indices.length * 2,
        count: 0,
      };

      for (let i = 0; i < segments.length; ++i) {
        let idx = segments[i];
        let segment = segmentIndices[idx];
        character.count += segment.length;
        indices.push(...segment);
      }

      characters[c] = character;
    }

    /* Segment layout is as follows:

    |-0-|
    3   4
    |-1-|
    5   6
    |-2-|

    */

    defineSegment(0, -1, 1, width, 1-thickness);
    defineSegment(1, -1, thickness*0.5, width, -thickness*0.5);
    defineSegment(2, -1, -1+thickness, width, -1);
    defineSegment(3, -1, 1, -1+thickness, -thickness*0.5);
    defineSegment(4, width-thickness, 1, width, -thickness*0.5);
    defineSegment(5, -1, thickness*0.5, -1+thickness, -1);
    defineSegment(6, width-thickness, thickness*0.5, width, -1);


    defineCharacter('0', [0, 2, 3, 4, 5, 6]);
    defineCharacter('1', [4, 6]);
    defineCharacter('2', [0, 1, 2, 4, 5]);
    defineCharacter('3', [0, 1, 2, 4, 6]);
    defineCharacter('4', [1, 3, 4, 6]);
    defineCharacter('5', [0, 1, 2, 3, 6]);
    defineCharacter('6', [0, 1, 2, 3, 5, 6]);
    defineCharacter('7', [0, 4, 6]);
    defineCharacter('8', [0, 1, 2, 3, 4, 5, 6]);
    defineCharacter('9', [0, 1, 2, 3, 4, 6]);
    defineCharacter('A', [0, 1, 3, 4, 5, 6]);
    defineCharacter('B', [1, 2, 3, 5, 6]);
    defineCharacter('C', [0, 2, 3, 5]);
    defineCharacter('D', [1, 2, 4, 5, 6]);
    defineCharacter('E', [0, 1, 2, 4, 6]);
    defineCharacter('F', [0, 1, 3, 5]);
    defineCharacter('P', [0, 1, 3, 4, 5]);
    defineCharacter('-', [1]);
    defineCharacter(' ', []);
    defineCharacter('_', [2]); // Used for undefined characters

    let gl = renderer.gl;
    let vertexBuffer = renderer.createRenderBuffer(gl.ARRAY_BUFFER, new Float32Array(vertices));
    let indexBuffer = renderer.createRenderBuffer(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices));

    let vertexAttribs = [
      new PrimitiveAttribute('POSITION', vertexBuffer, 2, gl.FLOAT, 8, 0),
    ];

    let primitive = new Primitive(vertexAttribs, indices.length);
    primitive.setIndexBuffer(indexBuffer);

    let material = new SevenSegmentMaterial();

    this._charPrimitives = {};
    for (let char in characters) {
      let charDef = characters[char];
      primitive.elementCount = charDef.count;
      primitive.indexByteOffset = charDef.offset;
      this._charPrimitives[char] = renderer.createRenderPrimitive(primitive, material);
    }

    this.text = this._text;
  }

  get text() {
    return this._text;
  }

  set text(value) {
    this._text = value;

    let i = 0;
    let charPrimitive = null;
    for (; i < value.length; ++i) {
      if (value[i] in this._charPrimitives) {
        charPrimitive = this._charPrimitives[value[i]];
      } else {
        charPrimitive = this._charPrimitives['_'];
      }

      if (this._charNodes.length <= i) {
        let node = new Node();
        node.addRenderPrimitive(charPrimitive);
        let offset = i * TEXT_KERNING;
        node.translation = [offset, 0, 0];
        this._charNodes.push(node);
        this.addNode(node);
      } else {
        // This is sort of an abuse of how these things are expected to work,
        // but it's the cheapest thing I could think of that didn't break the
        // world.
        this._charNodes[i].clearRenderPrimitives();
        this._charNodes[i].addRenderPrimitive(charPrimitive);
        this._charNodes[i].visible = true;
      }
    }

    // If there's any nodes left over make them invisible
    for (; i < this._charNodes.length; ++i) {
      this._charNodes[i].visible = false;
    }
  }
}
