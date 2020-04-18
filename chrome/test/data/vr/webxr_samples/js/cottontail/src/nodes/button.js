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

import {Material} from '../core/material.js';
import {Node} from '../core/node.js';
import {PrimitiveStream} from '../geometry/primitive-stream.js';

const BUTTON_SIZE = 0.1;
const BUTTON_CORNER_RADIUS = 0.025;
const BUTTON_CORNER_SEGMENTS = 8;
const BUTTON_ICON_SIZE = 0.07;
const BUTTON_LAYER_DISTANCE = 0.005;
const BUTTON_COLOR = 0.75;
const BUTTON_ALPHA = 0.85;
const BUTTON_HOVER_COLOR = 0.9;
const BUTTON_HOVER_ALPHA = 1.0;
const BUTTON_HOVER_SCALE = 1.1;
const BUTTON_HOVER_TRANSITION_TIME_MS = 200;

class ButtonMaterial extends Material {
  constructor() {
    super();

    this.state.blend = true;

    this.defineUniform('hoverAmount', 0);
  }

  get materialName() {
    return 'BUTTON_MATERIAL';
  }

  get vertexSource() {
    return `
    attribute vec3 POSITION;

    uniform float hoverAmount;

    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {
      float scale = mix(1.0, ${BUTTON_HOVER_SCALE}, hoverAmount);
      vec4 pos = vec4(POSITION.x * scale, POSITION.y * scale, POSITION.z * (scale + (hoverAmount * 0.2)), 1.0);
      return proj * view * model * pos;
    }`;
  }

  get fragmentSource() {
    return `
    uniform float hoverAmount;

    const vec4 default_color = vec4(${BUTTON_COLOR}, ${BUTTON_COLOR}, ${BUTTON_COLOR}, ${BUTTON_ALPHA});
    const vec4 hover_color = vec4(${BUTTON_HOVER_COLOR}, ${BUTTON_HOVER_COLOR},
                                  ${BUTTON_HOVER_COLOR}, ${BUTTON_HOVER_ALPHA});

    vec4 fragment_main() {
      return mix(default_color, hover_color, hoverAmount);
    }`;
  }
}

class ButtonIconMaterial extends Material {
  constructor() {
    super();

    this.state.blend = true;

    this.defineUniform('hoverAmount', 0);
    this.icon = this.defineSampler('icon');
  }

  get materialName() {
    return 'BUTTON_ICON_MATERIAL';
  }

  get vertexSource() {
    return `
    attribute vec3 POSITION;
    attribute vec2 TEXCOORD_0;

    uniform float hoverAmount;

    varying vec2 vTexCoord;

    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {
      vTexCoord = TEXCOORD_0;
      float scale = mix(1.0, ${BUTTON_HOVER_SCALE}, hoverAmount);
      vec4 pos = vec4(POSITION.x * scale, POSITION.y * scale, POSITION.z * (scale + (hoverAmount * 0.2)), 1.0);
      return proj * view * model * pos;
    }`;
  }

  get fragmentSource() {
    return `
    uniform sampler2D icon;
    varying vec2 vTexCoord;

    vec4 fragment_main() {
      return texture2D(icon, vTexCoord);
    }`;
  }
}

export class ButtonNode extends Node {
  constructor(iconTexture, callback) {
    super();

    // All buttons are selectable by default.
    this.selectable = true;

    this._selectHandler = callback;
    this._iconTexture = iconTexture;
    this._hovered = false;
    this._hoverT = 0;
  }

  get iconTexture() {
    return this._iconTexture;
  }

  set iconTexture(value) {
    if (this._iconTexture == value) {
      return;
    }

    this._iconTexture = value;
    this._iconRenderPrimitive.samplers.icon.texture = value;
  }

  onRendererChanged(renderer) {
    let stream = new PrimitiveStream();

    let hd = BUTTON_LAYER_DISTANCE * 0.5;

    // Build a rounded rect for the background.
    let hs = BUTTON_SIZE * 0.5;
    let ihs = hs - BUTTON_CORNER_RADIUS;
    stream.startGeometry();

    // Rounded corners and sides
    let segments = BUTTON_CORNER_SEGMENTS * 4;
    for (let i = 0; i < segments; ++i) {
      let rad = i * ((Math.PI * 2.0) / segments);
      let x = Math.cos(rad) * BUTTON_CORNER_RADIUS;
      let y = Math.sin(rad) * BUTTON_CORNER_RADIUS;
      let section = Math.floor(i / BUTTON_CORNER_SEGMENTS);
      switch (section) {
        case 0:
          x += ihs;
          y += ihs;
          break;
        case 1:
          x -= ihs;
          y += ihs;
          break;
        case 2:
          x -= ihs;
          y -= ihs;
          break;
        case 3:
          x += ihs;
          y -= ihs;
          break;
      }

      stream.pushVertex(x, y, -hd, 0, 0, 0, 0, 1);

      if (i > 1) {
        stream.pushTriangle(0, i-1, i);
      }
    }

    stream.endGeometry();

    let buttonPrimitive = stream.finishPrimitive(renderer);
    this._buttonRenderPrimitive = renderer.createRenderPrimitive(buttonPrimitive, new ButtonMaterial());
    this.addRenderPrimitive(this._buttonRenderPrimitive);

    // Build a simple textured quad for the foreground.
    hs = BUTTON_ICON_SIZE * 0.5;
    stream.clear();
    stream.startGeometry();

    stream.pushVertex(-hs, hs, hd, 0, 0, 0, 0, 1);
    stream.pushVertex(-hs, -hs, hd, 0, 1, 0, 0, 1);
    stream.pushVertex(hs, -hs, hd, 1, 1, 0, 0, 1);
    stream.pushVertex(hs, hs, hd, 1, 0, 0, 0, 1);

    stream.pushTriangle(0, 1, 2);
    stream.pushTriangle(0, 2, 3);

    stream.endGeometry();

    let iconPrimitive = stream.finishPrimitive(renderer);
    let iconMaterial = new ButtonIconMaterial();
    iconMaterial.icon.texture = this._iconTexture;
    this._iconRenderPrimitive = renderer.createRenderPrimitive(iconPrimitive, iconMaterial);
    this.addRenderPrimitive(this._iconRenderPrimitive);
  }

  onHoverStart() {
    this._hovered = true;
  }

  onHoverEnd() {
    this._hovered = false;
  }

  _updateHoverState() {
    let t = this._hoverT / BUTTON_HOVER_TRANSITION_TIME_MS;
    // Cubic Ease In/Out
    // TODO: Get a better animation system
    let hoverAmount = t<.5 ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1;
    this._buttonRenderPrimitive.uniforms.hoverAmount.value = hoverAmount;
    this._iconRenderPrimitive.uniforms.hoverAmount.value = hoverAmount;
  }

  onUpdate(timestamp, frameDelta) {
    if (this._hovered && this._hoverT < BUTTON_HOVER_TRANSITION_TIME_MS) {
      this._hoverT = Math.min(BUTTON_HOVER_TRANSITION_TIME_MS, this._hoverT + frameDelta);
      this._updateHoverState();
    } else if (!this._hovered && this._hoverT > 0) {
      this._hoverT = Math.max(0.0, this._hoverT - frameDelta);
      this._updateHoverState();
    }
  }
}
