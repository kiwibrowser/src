/*
 * Copyright (c) 2011 The Chromium Authors. All rights reserved.
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

var gl = null;
var g_width = 0;
var g_height = 0;
var g_texture = null;
var g_textureLoc = -1;
var g_programObject = null;
var g_vbo = null;
var g_texCoordOffset=0;

function main() {
    var c = document.getElementById("c");

    //c = WebGLDebugUtils.makeLostContextSimulatingCanvas(c);
    // tell the simulator when to lose context.
    //c.loseContextInNCalls(15);

    c.addEventListener('webglcontextlost', handleContextLost, false);
    c.addEventListener('webglcontextrestored', handleContextRestored, false);

    gl = WebGLUtils.setupWebGL(c);
    if (!gl)
        return;
    g_width = c.width;
    g_height = c.height;
    init();
}

function log(msg) {
    if (window.console && window.console.log) {
        console.log(msg);
    }
}

function handleContextLost(e) {
    log("handle context lost");
    e.preventDefault();
}

function handleContextRestored() {
    log("handle context restored");
    init();
}

function init() {
    gl.clearColor(0., 0., .7, 1.);
    g_texture = createCheckerboardTexture();
    initShaders();
    draw();
}

function checkGLError() {
    var error = gl.getError();
    if (error != gl.NO_ERROR && error != gl.CONTEXT_LOST_WEBGL) {
        var str = "GL Error: " + error;
        document.body.appendChild(document.createTextNode(str));
        throw str;
    }
}

function loadShader(type, shaderSrc) {
    var shader = gl.createShader(type);
    // Load the shader source
    gl.shaderSource(shader, shaderSrc);
    // Compile the shader
    gl.compileShader(shader);
    // Check the compile status
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS) &&
        !gl.isContextLost()) {
        var infoLog = gl.getShaderInfoLog(shader);
        alert("Error compiling shader:\n" + infoLog);
        gl.deleteShader(shader);
        return null;
    }
    return shader;
}

function initShaders() {
    var vShaderStr = [
        "attribute vec3 g_Position;",
        "attribute vec2 g_TexCoord0;",
        "varying vec2 texCoord;",
        "void main()",
        "{",
        "   gl_Position = vec4(g_Position.x, g_Position.y, g_Position.z, 1.0);",
        "   texCoord = g_TexCoord0;",
        "}"
    ].join("\n");
    var fShaderStr = [
        "precision mediump float;\n",
        "uniform sampler2D tex;",
        "varying vec2 texCoord;",
        "void main()",
        "{",
        "  gl_FragColor = texture2D(tex, texCoord);",
        "}"
    ].join("\n");

    var vertexShader = loadShader(gl.VERTEX_SHADER, vShaderStr);
    var fragmentShader = loadShader(gl.FRAGMENT_SHADER, fShaderStr);
    // Create the program object
    var programObject = gl.createProgram();
    gl.attachShader(programObject, vertexShader);
    gl.attachShader(programObject, fragmentShader);
    // Bind g_Position to attribute 0
    // Bind g_TexCoord0 to attribute 1
    gl.bindAttribLocation(programObject, 0, "g_Position");
    gl.bindAttribLocation(programObject, 1, "g_TexCoord0");
    // Link the program
    gl.linkProgram(programObject);
    // Check the link status
    var linked = gl.getProgramParameter(programObject, gl.LINK_STATUS);
    if (!linked && !gl.isContextLost()) {
        var infoLog = gl.getProgramInfoLog(programObject);
        alert("Error linking program:\n" + infoLog);
        gl.deleteProgram(programObject);
        return;
    }
    g_programObject = programObject;
    g_textureLoc = gl.getUniformLocation(g_programObject, "tex");
    g_vbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, g_vbo);
    var vertices = new Float32Array([
        0.25,  0.75, 0.0,
        -0.75,  0.75, 0.0,
        -0.75, -0.25, 0.0,
        0.25,  0.75, 0.0,
        -0.75, -0.25, 0.0,
        0.25, -0.25, 0.0]);
    var texCoords = new Float32Array([
        1.0, 1.0,
        0.0, 1.0,
        0.0, 0.0,
        1.0, 1.0,
        0.0, 0.0,
        1.0, 0.0]);
    g_texCoordOffset = vertices.byteLength;
    gl.bufferData(gl.ARRAY_BUFFER,
                  g_texCoordOffset + texCoords.byteLength,
                  gl.STATIC_DRAW);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, vertices);
    gl.bufferSubData(gl.ARRAY_BUFFER, g_texCoordOffset, texCoords);
    checkGLError();
}

function draw() {
    // Note: the viewport is automatically set up to cover the entire Canvas.
    // Clear the color buffer
    gl.clear(gl.COLOR_BUFFER_BIT);
    checkGLError();
    // Use the program object
    gl.useProgram(g_programObject);
    checkGLError();
    // Load the vertex data
    gl.bindBuffer(gl.ARRAY_BUFFER, g_vbo);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 0, 0);
    gl.enableVertexAttribArray(1);
    gl.vertexAttribPointer(1, 2, gl.FLOAT, gl.FALSE, 0, g_texCoordOffset);
    checkGLError();
    // Bind the texture to texture unit 0
    gl.bindTexture(gl.TEXTURE_2D, g_texture);
    checkGLError();
    // Point the uniform sampler to texture unit 0
    gl.uniform1i(g_textureLoc, 0);
    checkGLError();
    gl.drawArrays(gl.TRIANGLES, 0, 6);
    checkGLError();
}

function createCheckerboardTexture() {
    var pixels = new Uint8Array([255, 255, 255,
                                             0,   0,   0,
                                             0,   0,   0,
                                             255, 255, 255]);
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    //  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    //  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, 2, 2, 0, gl.RGB, gl.UNSIGNED_BYTE, pixels);
    return texture;
}
