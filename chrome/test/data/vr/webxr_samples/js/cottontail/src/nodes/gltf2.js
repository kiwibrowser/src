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

import {Node} from '../core/node.js';
import {Gltf2Loader} from '../loaders/gltf2.js';

// Using a weak map here allows us to cache a loader per-renderer without
// modifying the renderer object or leaking memory when it's garbage collected.
let gltfLoaderMap = new WeakMap();

export class Gltf2Node extends Node {
  constructor(options) {
    super();
    this._url = options.url;

    this._promise = null;
    this._resolver = null;
    this._rejecter = null;
  }

  onRendererChanged(renderer) {
    let loader = gltfLoaderMap.get(renderer);
    if (!loader) {
      loader = new Gltf2Loader(renderer);
      gltfLoaderMap.set(renderer, loader);
    }

    // Do we have a previously resolved promise? If so clear it.
    if (!this._resolver && this._promise) {
      this._promise = null;
    }

    this._ensurePromise();

    loader.loadFromUrl(this._url).then((sceneNode) => {
      this.addNode(sceneNode);
      this._resolver(sceneNode.waitForComplete());
      this._resolver = null;
      this._rejecter = null;
    }).catch((err) => {
      this._rejecter(err);
      this._resolver = null;
      this._rejecter = null;
    });
  }

  _ensurePromise() {
    if (!this._promise) {
      this._promise = new Promise((resolve, reject) => {
        this._resolver = resolve;
        this._rejecter = reject;
      });
    }
    return this._promise;
  }

  waitForComplete() {
    return this._ensurePromise();
  }
}
