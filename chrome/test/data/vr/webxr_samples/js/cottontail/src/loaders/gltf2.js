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

import {PbrMaterial} from '../materials/pbr.js';
import {Node} from '../core/node.js';
import {Primitive, PrimitiveAttribute} from '../core/primitive.js';
import {ImageTexture, ColorTexture} from '../core/texture.js';

const GL = WebGLRenderingContext; // For enums

const GLB_MAGIC = 0x46546C67;
const CHUNK_TYPE = {
  JSON: 0x4E4F534A,
  BIN: 0x004E4942,
};

function isAbsoluteUri(uri) {
  let absRegEx = new RegExp('^'+window.location.protocol, 'i');
  return !!uri.match(absRegEx);
}

function isDataUri(uri) {
  let dataRegEx = /^data:/;
  return !!uri.match(dataRegEx);
}

function resolveUri(uri, baseUrl) {
  if (isAbsoluteUri(uri) || isDataUri(uri)) {
      return uri;
  }
  return baseUrl + uri;
}

function getComponentCount(type) {
  switch (type) {
    case 'SCALAR': return 1;
    case 'VEC2': return 2;
    case 'VEC3': return 3;
    case 'VEC4': return 4;
    default: return 0;
  }
}

/**
 * Gltf2SceneLoader
 * Loads glTF 2.0 scenes into a renderable node tree.
 */

export class Gltf2Loader {
  constructor(renderer) {
    this.renderer = renderer;
    this._gl = renderer._gl;
  }

  loadFromUrl(url) {
    return fetch(url)
        .then((response) => {
          let i = url.lastIndexOf('/');
          let baseUrl = (i !== 0) ? url.substring(0, i + 1) : '';

          if (url.endsWith('.gltf')) {
            return response.json().then((json) => {
              return this.loadFromJson(json, baseUrl);
            });
          } else if (url.endsWith('.glb')) {
            return response.arrayBuffer().then((arrayBuffer) => {
              return this.loadFromBinary(arrayBuffer, baseUrl);
            });
          } else {
            throw new Error('Unrecognized file extension');
          }
        });
  }

  loadFromBinary(arrayBuffer, baseUrl) {
    let headerView = new DataView(arrayBuffer, 0, 12);
    let magic = headerView.getUint32(0, true);
    let version = headerView.getUint32(4, true);
    let length = headerView.getUint32(8, true);

    if (magic != GLB_MAGIC) {
      throw new Error('Invalid magic string in binary header.');
    }

    if (version != 2) {
      throw new Error('Incompatible version in binary header.');
    }

    let chunks = {};
    let chunkOffset = 12;
    while (chunkOffset < length) {
      let chunkHeaderView = new DataView(arrayBuffer, chunkOffset, 8);
      let chunkLength = chunkHeaderView.getUint32(0, true);
      let chunkType = chunkHeaderView.getUint32(4, true);
      chunks[chunkType] = arrayBuffer.slice(chunkOffset + 8, chunkOffset + 8 + chunkLength);
      chunkOffset += chunkLength + 8;
    }

    if (!chunks[CHUNK_TYPE.JSON]) {
      throw new Error('File contained no json chunk.');
    }

    let decoder = new TextDecoder('utf-8');
    let jsonString = decoder.decode(chunks[CHUNK_TYPE.JSON]);
    let json = JSON.parse(jsonString);
    return this.loadFromJson(json, baseUrl, chunks[CHUNK_TYPE.BIN]);
  }

  loadFromJson(json, baseUrl, binaryChunk) {
    if (!json.asset) {
      throw new Error('Missing asset description.');
    }

    if (json.asset.minVersion != '2.0' && json.asset.version != '2.0') {
      throw new Error('Incompatible asset version.');
    }

    let buffers = [];
    if (binaryChunk) {
      buffers[0] = new Gltf2Resource({}, baseUrl, binaryChunk);
    } else {
      for (let buffer of json.buffers) {
        buffers.push(new Gltf2Resource(buffer, baseUrl));
      }
    }

    let bufferViews = [];
    for (let bufferView of json.bufferViews) {
      bufferViews.push(new Gltf2BufferView(bufferView, buffers));
    }

    let images = [];
    if (json.images) {
      for (let image of json.images) {
        images.push(new Gltf2Resource(image, baseUrl));
      }
    }

    let textures = [];
    if (json.textures) {
      for (let texture of json.textures) {
        let image = images[texture.source];
        let glTexture = image.texture(bufferViews);
        if (texture.sampler) {
          let sampler = sampler[texture.sampler];
          glTexture.sampler.minFilter = sampler.minFilter;
          glTexture.sampler.magFilter = sampler.magFilter;
          glTexture.sampler.wrapS = sampler.wrapS;
          glTexture.sampler.wrapT = sampler.wrapT;
        }
        textures.push(glTexture);
      }
    }

    function getTexture(textureInfo) {
      if (!textureInfo) {
        return null;
      }
      return textures[textureInfo.index];
    }

    let materials = [];
    if (json.materials) {
      for (let material of json.materials) {
        let glMaterial = new PbrMaterial();
        let pbr = material.pbrMetallicRoughness || {};

        glMaterial.baseColorFactor.value = pbr.baseColorFactor || [1, 1, 1, 1];
        glMaterial.baseColor.texture = getTexture(pbr.baseColorTexture);
        glMaterial.metallicRoughnessFactor.value = [
          pbr.metallicFactor || 1.0,
          pbr.roughnessFactor || 1.0,
        ];
        glMaterial.metallicRoughness.texture = getTexture(pbr.metallicRoughnessTexture);
        glMaterial.normal.texture = getTexture(json.normalTexture);
        glMaterial.occlusion.texture = getTexture(json.occlusionTexture);
        glMaterial.occlusionStrength.value = (json.occlusionTexture && json.occlusionTexture.strength) ?
                                              json.occlusionTexture.strength : 1.0;
        glMaterial.emissiveFactor.value = material.emissiveFactor || [0, 0, 0];
        glMaterial.emissive.texture = getTexture(json.emissiveTexture);
        if (!glMaterial.emissive.texture && json.emissiveFactor) {
          glMaterial.emissive.texture = new ColorTexture(1.0, 1.0, 1.0, 1.0);
        }

        switch (material.alphaMode) {
          case 'BLEND':
            glMaterial.state.blend = true;
            break;
          case 'MASK':
            // Not really supported.
            glMaterial.state.blend = true;
            break;
          default: // Includes 'OPAQUE'
            glMaterial.state.blend = false;
        }

        // glMaterial.alpha_mode = material.alphaMode;
        // glMaterial.alpha_cutoff = material.alphaCutoff;
        glMaterial.state.cullFace = !(material.doubleSided);

        materials.push(glMaterial);
      }
    }

    let accessors = json.accessors;

    let meshes = [];
    for (let mesh of json.meshes) {
      let glMesh = new Gltf2Mesh();
      meshes.push(glMesh);

      for (let primitive of mesh.primitives) {
        let material = null;
        if ('material' in primitive) {
          material = materials[primitive.material];
        } else {
          // Create a "default" material if the primitive has none.
          material = new PbrMaterial();
        }

        let attributes = [];
        let elementCount = 0;
        /* let glPrimitive = new Gltf2Primitive(primitive, material);
        glMesh.primitives.push(glPrimitive); */

        let min = null;
        let max = null;

        for (let name in primitive.attributes) {
          let accessor = accessors[primitive.attributes[name]];
          let bufferView = bufferViews[accessor.bufferView];
          elementCount = accessor.count;

          let glAttribute = new PrimitiveAttribute(
            name,
            bufferView.renderBuffer(this.renderer, GL.ARRAY_BUFFER),
            getComponentCount(accessor.type),
            accessor.componentType,
            bufferView.byteStride || 0,
            accessor.byteOffset || 0
          );
          glAttribute.normalized = accessor.normalized || false;

          if (name == 'POSITION') {
            min = accessor.min;
            max = accessor.max;
          }

          attributes.push(glAttribute);
        }

        let glPrimitive = new Primitive(attributes, elementCount, primitive.mode);

        if ('indices' in primitive) {
          let accessor = accessors[primitive.indices];
          let bufferView = bufferViews[accessor.bufferView];

          glPrimitive.setIndexBuffer(
            bufferView.renderBuffer(this.renderer, GL.ELEMENT_ARRAY_BUFFER),
            accessor.byteOffset || 0,
            accessor.componentType
          );
          glPrimitive.indexType = accessor.componentType;
          glPrimitive.indexByteOffset = accessor.byteOffset || 0;
          glPrimitive.elementCount = accessor.count;
        }

        if (min && max) {
          glPrimitive.setBounds(min, max);
        }

        // After all the attributes have been processed, get a program that is
        // appropriate for both the material and the primitive attributes.
        glMesh.primitives.push(
            this.renderer.createRenderPrimitive(glPrimitive, material));
      }
    }

    let sceneNode = new Node();
    let scene = json.scenes[json.scene];
    for (let nodeId of scene.nodes) {
      let node = json.nodes[nodeId];
      sceneNode.addNode(
          this.processNodes(node, json.nodes, meshes));
    }

    return sceneNode;
  }

  processNodes(node, nodes, meshes) {
    let glNode = new Node();
    glNode.name = node.name;

    if ('mesh' in node) {
      let mesh = meshes[node.mesh];
      for (let primitive of mesh.primitives) {
        glNode.addRenderPrimitive(primitive);
      }
    }

    if (node.matrix) {
      glNode.matrix = new Float32Array(node.matrix);
    } else if (node.translation || node.rotation || node.scale) {
      if (node.translation) {
        glNode.translation = new Float32Array(node.translation);
      }

      if (node.rotation) {
        glNode.rotation = new Float32Array(node.rotation);
      }

      if (node.scale) {
        glNode.scale = new Float32Array(node.scale);
      }
    }

    if (node.children) {
      for (let nodeId of node.children) {
        let node = nodes[nodeId];
        glNode.addNode(this.processNodes(node, nodes, meshes));
      }
    }

    return glNode;
  }
}

class Gltf2Mesh {
  constructor() {
    this.primitives = [];
  }
}

class Gltf2BufferView {
  constructor(json, buffers) {
    this.buffer = buffers[json.buffer];
    this.byteOffset = json.byteOffset || 0;
    this.byteLength = json.byteLength || null;
    this.byteStride = json.byteStride;

    this._viewPromise = null;
    this._renderBuffer = null;
  }

  dataView() {
    if (!this._viewPromise) {
      this._viewPromise = this.buffer.arrayBuffer().then((arrayBuffer) => {
        return new DataView(arrayBuffer, this.byteOffset, this.byteLength);
      });
    }
    return this._viewPromise;
  }

  renderBuffer(renderer, target) {
    if (!this._renderBuffer) {
      this._renderBuffer = renderer.createRenderBuffer(target, this.dataView());
    }
    return this._renderBuffer;
  }
}

class Gltf2Resource {
  constructor(json, baseUrl, arrayBuffer) {
    this.json = json;
    this.baseUrl = baseUrl;

    this._dataPromise = null;
    this._texture = null;
    if (arrayBuffer) {
      this._dataPromise = Promise.resolve(arrayBuffer);
    }
  }

  arrayBuffer() {
    if (!this._dataPromise) {
      if (isDataUri(this.json.uri)) {
        let base64String = this.json.uri.replace('data:application/octet-stream;base64,', '');
        let binaryArray = Uint8Array.from(atob(base64String), (c) => c.charCodeAt(0));
        this._dataPromise = Promise.resolve(binaryArray.buffer);
        return this._dataPromise;
      }

      this._dataPromise = fetch(resolveUri(this.json.uri, this.baseUrl))
          .then((response) => response.arrayBuffer());
    }
    return this._dataPromise;
  }

  texture(bufferViews) {
    if (!this._texture) {
      let img = new Image();
      this._texture = new ImageTexture(img);

      if (this.json.uri) {
        if (isDataUri(this.json.uri)) {
          img.src = this.json.uri;
        } else {
          img.src = `${this.baseUrl}${this.json.uri}`;
        }
      } else {
        let view = bufferViews[this.json.bufferView];
        view.dataView().then((dataView) => {
          let blob = new Blob([dataView], {type: this.json.mimeType});
          img.src = window.URL.createObjectURL(blob);
        });
      }
    }
    return this._texture;
  }
}
