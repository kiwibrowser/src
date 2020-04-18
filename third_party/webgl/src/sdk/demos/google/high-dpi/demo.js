/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// The "model" matrix is the "world" matrix in Standard Annotations
// and Semantics
var model = new Matrix4x4();
var view = new Matrix4x4();
var projection = new Matrix4x4();

var controller = null;

var bumpReflectVertexSource = [
    "attribute vec3 g_Position;",
    "attribute vec3 g_TexCoord0;",
    "attribute vec3 g_Tangent;",
    "attribute vec3 g_Binormal;",
    "attribute vec3 g_Normal;",
    "",
    "uniform mat4 world;",
    "uniform mat4 worldInverseTranspose;",
    "uniform mat4 worldViewProj;",
    "uniform mat4 viewInverse;",
    "",
    "varying vec2 texCoord;",
    "varying vec3 worldEyeVec;",
    "varying vec3 worldNormal;",
    "varying vec3 worldTangent;",
    "varying vec3 worldBinorm;",
    "",
    "void main() {",
    "  gl_Position = worldViewProj * vec4(g_Position.xyz, 1.);",
    "  texCoord.xy = g_TexCoord0.xy;",
    "  worldNormal = (worldInverseTranspose * vec4(g_Normal, 1.)).xyz;",
    "  worldTangent = (worldInverseTranspose * vec4(g_Tangent, 1.)).xyz;",
    "  worldBinorm = (worldInverseTranspose * vec4(g_Binormal, 1.)).xyz;",
    "  vec3 worldPos = (world * vec4(g_Position, 1.)).xyz;",
    "  worldEyeVec = normalize(worldPos - viewInverse[3].xyz);",
    "}"
    ].join("\n");

var bumpReflectFragmentSource = [
    "precision mediump float;\n",
    "const float bumpHeight = 0.2;",
    "",
    "uniform sampler2D normalSampler;",
    "uniform samplerCube envSampler;",
    "",
    "varying vec2 texCoord;",
    "varying vec3 worldEyeVec;",
    "varying vec3 worldNormal;",
    "varying vec3 worldTangent;",
    "varying vec3 worldBinorm;",
    "",
    "void main() {",
    "  vec2 bump = (texture2D(normalSampler, texCoord.xy).xy * 2.0 - 1.0) * bumpHeight;",
    "  vec3 normal = normalize(worldNormal);",
    "  vec3 tangent = normalize(worldTangent);",
    "  vec3 binormal = normalize(worldBinorm);",
    "  vec3 nb = normal + bump.x * tangent + bump.y * binormal;",
    "  nb = normalize(nb);",
    "  vec3 worldEye = normalize(worldEyeVec);",
    "  vec3 lookup = reflect(worldEye, nb);",
    "  vec4 color = textureCube(envSampler, lookup);",
    "  gl_FragColor = color;",
    "}"
    ].join("\n");

var wireVertexSource = [
    "attribute vec3 g_Position;",
    "",
    "uniform mat4 worldViewProj;",
    "",
    "void main() {",
    "  gl_Position = worldViewProj * vec4(g_Position.xyz, 1.);",
    "}"
    ].join("\n");

var wireFragmentSource = [
    "precision mediump float;\n",
    "",
    "void main() {",
    "  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);",
    "}"
    ].join("\n");

function log(msg) {
    if (window.console && window.console.log) {
        console.log(msg);
    }
}

function output(str) {
    document.body.appendChild(document.createTextNode(str));
    document.body.appendChild(document.createElement("br"));
}

function checkGLError(gl) {
    var error = gl.getError();
    if (error != gl.NO_ERROR && error != gl.CONTEXT_LOST_WEBGL) {
        var str = "GL Error: " + error;
        output(str);
        throw str;
    }
}

function main() {
    var highDpiDemo = new TeapotDemo(document.getElementById("c1"));
    var lowDpiDemo = new TeapotDemo(document.getElementById("c2"));

    controller = new CameraController(document.body);
    
    controller.onchange = function(xRot, yRot) {
        highDpiDemo.draw();
        lowDpiDemo.draw();
    };
    
    highDpiDemo.startDpiAwareDemo();
    lowDpiDemo.startDemo();
}

function TeapotDemo(canvas) {
    this.canvas = canvas;
    this.gl = null;
    this.width = 0;
    this.height = 0;
    this.bumpTexture = null;
    this.envTexture = null;
    this.programObject = null;
    this.wireProgramObject = null;
    this.vbo = null;
    this.elementVbo = null;
    this.wireElementVbo = null;
    this.normalsOffset = 0;
    this.tangentsOffset = 0;
    this.binormalsOffset = 0;
    this.texCoordsOffset = 0;
    this.numElements = 0;
    this.numWireElements = 0;

    // Uniform variables
    this.worldLoc = 0;
    this.worldInverseTransposeLoc = 0;
    this.worldViewProjLoc = 0;
    this.viewInverseLoc = 0;
    this.normalSamplerLoc = 0;
    this.envSamplerLoc = 0;
    this.wireWorldViewProjLoc = 0;

    // Array of images curently loading
    this.loadingImages = [];
    this.pendingTextureLoads = 0;

    // Lost context handling
    var self = this;
    
    this.canvas.addEventListener('webglcontextlost', function(e) {
        log("handle context lost");
        e.preventDefault();
        self.clearLoadingImages();
    }, false);

    this.canvas.addEventListener('webglcontextrestored', function(e) {
        log("handle context restored");
        self.init();
    }, false);
}

TeapotDemo.prototype.startDemo = function() {
    this.canvas.width = this.canvas.clientWidth;
    this.canvas.height = this.canvas.clientHeight;
    this.init();
}

TeapotDemo.prototype.startDpiAwareDemo = function() {
    var ratio = window.devicePixelRatio ? window.devicePixelRatio : 1;
    this.canvas.width = this.canvas.clientWidth * ratio;
    this.canvas.height = this.canvas.clientHeight * ratio;
    this.init();
}

TeapotDemo.prototype.init = function() {
    // Antialiasing is disabled to magnify the effects of the Higher DPI settings
    var gl = WebGLUtils.setupWebGL(this.canvas, {antialias: false});
    if (!gl)
        return;
    this.gl = gl;
    this.width = this.canvas.width;
    this.height = this.canvas.height;

    gl.enable(gl.DEPTH_TEST);
    gl.clearColor(0.0, 0.0, 0.0, 0.0);

    this.initTeapot(gl);
    this.initShaders(gl);
    this.bumpTexture = this.loadTexture(gl, "bump.jpg");
    this.envTexture = this.loadCubeMap(gl, "skybox", "jpg");
    this.draw();
}

TeapotDemo.prototype.initTeapot = function(gl) {
    this.vbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vbo);
    gl.bufferData(gl.ARRAY_BUFFER,
                  teapotPositions.byteLength +
                  teapotNormals.byteLength +
                  teapotTangents.byteLength +
                  teapotBinormals.byteLength +
                  teapotTexCoords.byteLength,
                  gl.STATIC_DRAW);
    this.normalsOffset = teapotPositions.byteLength;
    this.tangentsOffset = this.normalsOffset + teapotNormals.byteLength;
    this.binormalsOffset = this.tangentsOffset + teapotTangents.byteLength;
    this.texCoordsOffset = this.binormalsOffset + teapotBinormals.byteLength;
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, teapotPositions);
    gl.bufferSubData(gl.ARRAY_BUFFER, this.normalsOffset, teapotNormals);
    gl.bufferSubData(gl.ARRAY_BUFFER, this.tangentsOffset, teapotTangents);
    gl.bufferSubData(gl.ARRAY_BUFFER, this.binormalsOffset, teapotBinormals);
    gl.bufferSubData(gl.ARRAY_BUFFER, this.texCoordsOffset, teapotTexCoords);

    this.elementVbo = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.elementVbo);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, teapotIndices, gl.STATIC_DRAW);
    this.numElements = teapotIndices.length;
    
    var wireIndices = this.createWireIndicies(teapotIndices);
    this.wireElementVbo = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.wireElementVbo);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, wireIndices, gl.STATIC_DRAW);
    this.numWireElements = wireIndices.length;
    checkGLError(gl);

}

TeapotDemo.prototype.createWireIndicies = function(triIndicies) {
    var wireIndices = new Uint16Array(triIndicies.length * 2);
    var i, j;
    for(i = 0, j = 0; i < triIndicies.length; i += 3) {
        wireIndices[j++] = triIndicies[i];
        wireIndices[j++] = triIndicies[i+1];

        wireIndices[j++] = triIndicies[i+1];
        wireIndices[j++] = triIndicies[i+2];

        wireIndices[j++] = triIndicies[i+2];
        wireIndices[j++] = triIndicies[i];
    }
    return wireIndices;
}

TeapotDemo.prototype.loadShader = function(gl, type, shaderSrc) {
    var shader = gl.createShader(type);
    // Load the shader source
    gl.shaderSource(shader, shaderSrc);
    // Compile the shader
    gl.compileShader(shader);
    // Check the compile status
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS) &&
        !gl.isContextLost()) {
        var infoLog = gl.getShaderInfoLog(shader);
        output("Error compiling shader:\n" + infoLog);
        gl.deleteShader(shader);
        return null;
    }
    return shader;
}

TeapotDemo.prototype.initShaders = function(gl) {
    var vertexShader = this.loadShader(gl, gl.VERTEX_SHADER, bumpReflectVertexSource);
    var fragmentShader = this.loadShader(gl, gl.FRAGMENT_SHADER, bumpReflectFragmentSource);
    // Create the program object
    var programObject = gl.createProgram();
    gl.attachShader(programObject, vertexShader);
    gl.attachShader(programObject, fragmentShader);
    // Bind attributes
    gl.bindAttribLocation(programObject, 0, "g_Position");
    gl.bindAttribLocation(programObject, 1, "g_TexCoord0");
    gl.bindAttribLocation(programObject, 2, "g_Tangent");
    gl.bindAttribLocation(programObject, 3, "g_Binormal");
    gl.bindAttribLocation(programObject, 4, "g_Normal");
    // Link the program
    gl.linkProgram(programObject);
    // Check the link status
    var linked = gl.getProgramParameter(programObject, gl.LINK_STATUS);
    if (!linked && !gl.isContextLost()) {
        var infoLog = gl.getProgramInfoLog(programObject);
        output("Error linking program:\n" + infoLog);
        gl.deleteProgram(programObject);
        return;
    }
    
    this.programObject = programObject;
    // Look up uniform locations
    this.worldLoc = gl.getUniformLocation(programObject, "world");
    this.worldInverseTransposeLoc = gl.getUniformLocation(programObject, "worldInverseTranspose");
    this.worldViewProjLoc = gl.getUniformLocation(programObject, "worldViewProj");
    this.viewInverseLoc = gl.getUniformLocation(programObject, "viewInverse");
    this.normalSamplerLoc = gl.getUniformLocation(programObject, "normalSampler");
    this.envSamplerLoc = gl.getUniformLocation(programObject, "envSampler");
    checkGLError(gl);

    vertexShader = this.loadShader(gl, gl.VERTEX_SHADER, wireVertexSource);
    fragmentShader = this.loadShader(gl, gl.FRAGMENT_SHADER, wireFragmentSource);
    // Create the program object
    programObject = gl.createProgram();
    gl.attachShader(programObject, vertexShader);
    gl.attachShader(programObject, fragmentShader);
    // Bind attributes
    gl.bindAttribLocation(programObject, 0, "g_Position");
    // Link the program
    gl.linkProgram(programObject);
    // Check the link status
    var linked = gl.getProgramParameter(programObject, gl.LINK_STATUS);
    if (!linked && !gl.isContextLost()) {
        var infoLog = gl.getProgramInfoLog(programObject);
        output("Error linking program:\n" + infoLog);
        gl.deleteProgram(programObject);
        return;
    }
    this.wireProgramObject = programObject;
    this.wireWorldViewProjLoc = gl.getUniformLocation(programObject, "worldViewProj");
}

TeapotDemo.prototype.draw = function() {
    var gl = this.gl;

    // Note: the viewport is automatically set up to cover the entire Canvas.
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    // For now, don't render if we have incomplete textures, just to
    // avoid accidentally incurring OpenGL errors -- although we should
    // be fully able to load textures in in the background
    if (this.pendingTextureLoads > 0) {
        return;
    }

    // Set up the model, view and projection matrices
    projection.loadIdentity();
    projection.perspective(45, this.width / this.height, 10, 500);
    view.loadIdentity();
    view.translate(0, -10, -100.0);

    // Add in camera controller's rotation
    model.loadIdentity();
    model.rotate(controller.xRot, 1, 0, 0);
    model.rotate(controller.yRot, 0, 1, 0);

    // Correct for initial placement and orientation of model
    model.translate(0, -10, 0);
    model.rotate(90, 1, 0, 0);

    gl.enable(gl.POLYGON_OFFSET_FILL);
    gl.polygonOffset(1, -1);

    gl.useProgram(this.programObject);

    // Compute necessary matrices
    var mvp = new Matrix4x4();
    mvp.multiply(model);
    mvp.multiply(view);
    mvp.multiply(projection);
    var worldInverseTranspose = model.inverse();
    worldInverseTranspose.transpose();
    var viewInverse = view.inverse();

    // Set up uniforms
    gl.uniformMatrix4fv(this.worldLoc, gl.FALSE, new Float32Array(model.elements));
    gl.uniformMatrix4fv(this.worldInverseTransposeLoc, gl.FALSE, new Float32Array(worldInverseTranspose.elements));
    gl.uniformMatrix4fv(this.worldViewProjLoc, gl.FALSE, new Float32Array(mvp.elements));
    gl.uniformMatrix4fv(this.viewInverseLoc, gl.FALSE, new Float32Array(viewInverse.elements));
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.bumpTexture);
    gl.uniform1i(this.normalSamplerLoc, 0);
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_CUBE_MAP, this.envTexture);
    gl.uniform1i(this.envSamplerLoc, 1);

    // Bind and set up vertex streams
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vbo);
    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(1, 3, gl.FLOAT, false, 0, this.texCoordsOffset);
    gl.enableVertexAttribArray(1);
    gl.vertexAttribPointer(2, 3, gl.FLOAT, false, 0, this.tangentsOffset);
    gl.enableVertexAttribArray(2);
    gl.vertexAttribPointer(3, 3, gl.FLOAT, false, 0, this.binormalsOffset);
    gl.enableVertexAttribArray(3);
    gl.vertexAttribPointer(4, 3, gl.FLOAT, false, 0, this.normalsOffset);
    gl.enableVertexAttribArray(4);
    
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.elementVbo);
    gl.drawElements(gl.TRIANGLES, this.numElements, gl.UNSIGNED_SHORT, 0);

    // Render the mesh wireframe
    gl.useProgram(this.wireProgramObject);

    gl.uniformMatrix4fv(this.wireWorldViewProjLoc, gl.FALSE, new Float32Array(mvp.elements));

    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(0);

    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.wireElementVbo);
    gl.drawElements(gl.LINES, this.numWireElements, gl.UNSIGNED_SHORT, 0);
}

// Clears all the images currently loading.
// This is used to handle context lost events.
TeapotDemo.prototype.clearLoadingImages = function() {
    for (var ii = 0; ii < this.loadingImages.length; ++ii) {
        this.loadingImages[ii].onload = undefined;
    }
    this.loadingImages = [];
}

TeapotDemo.prototype.loadTexture = function(gl, src) {
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
    ++this.pendingTextureLoads;
    var self = this;
    var image = new Image();
    this.loadingImages.push(image);
    image.onload = function() {
        self.loadingImages.splice(self.loadingImages.indexOf(image), 1);
        --self.pendingTextureLoads;
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
        gl.texImage2D(
            gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
        checkGLError(gl);
        self.draw();
    };
    image.src = src;
    return texture;
}

TeapotDemo.prototype.loadCubeMap = function(gl, base, suffix) {
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_CUBE_MAP, texture);
    checkGLError(gl);
    gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    checkGLError(gl);
    gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    checkGLError(gl);
    // FIXME: TEXTURE_WRAP_R doesn't exist in OpenGL ES?!
    //  gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_R, gl.CLAMP_TO_EDGE);
    //  checkGLError();
    gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    checkGLError(gl);
    gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    checkGLError(gl);
    var faces = [["posx", gl.TEXTURE_CUBE_MAP_POSITIVE_X],
                 ["negx", gl.TEXTURE_CUBE_MAP_NEGATIVE_X],
                 ["posy", gl.TEXTURE_CUBE_MAP_POSITIVE_Y],
                 ["negy", gl.TEXTURE_CUBE_MAP_NEGATIVE_Y],
                 ["posz", gl.TEXTURE_CUBE_MAP_POSITIVE_Z],
                 ["negz", gl.TEXTURE_CUBE_MAP_NEGATIVE_Z]];
    var self = this;
    for (var i = 0; i < faces.length; i++) {
        var url = base + "-" + faces[i][0] + "." + suffix;
        var face = faces[i][1];
        ++this.pendingTextureLoads;
        var image = new Image();
        this.loadingImages.push(image);
        // Javascript has function, not block, scope.
        // See "JavaScript: The Good Parts", Chapter 4, "Functions",
        // section "Scope".
        image.onload = function(texture, face, image, url) {
            return function() {
                self.loadingImages.splice(self.loadingImages.indexOf(image), 1);
                --self.pendingTextureLoads;
                gl.bindTexture(gl.TEXTURE_CUBE_MAP, texture);
                gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
                gl.texImage2D(
                   face, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
                checkGLError(gl);
                self.draw();
            }
        }(texture, face, image, url);
        image.src = url;
    }
    return texture;
}
