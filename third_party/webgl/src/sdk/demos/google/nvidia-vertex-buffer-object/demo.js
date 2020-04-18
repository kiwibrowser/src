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

/*
 *
 * COPYRIGHT NVIDIA CORPORATION 2003. ALL RIGHTS RESERVED.
 * BY ACCESSING OR USING THIS SOFTWARE, YOU AGREE TO:
 *
 *  1) ACKNOWLEDGE NVIDIA'S EXCLUSIVE OWNERSHIP OF ALL RIGHTS
 *     IN AND TO THE SOFTWARE;
 *
 *  2) NOT MAKE OR DISTRIBUTE COPIES OF THE SOFTWARE WITHOUT
 *     INCLUDING THIS NOTICE AND AGREEMENT;
 *
 *  3) ACKNOWLEDGE THAT TO THE MAXIMUM EXTENT PERMITTED BY
 *     APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS* AND
 *     THAT NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES,
 *     EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED
 *     TO, IMPLIED WARRANTIES OF MERCHANTABILITY  AND FITNESS
 *     FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
 * SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS
 * OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS
 * INFORMATION, OR ANY OTHER PECUNIARY LOSS), INCLUDING ATTORNEYS'
 * FEES, RELATING TO THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 */

o3djs.require('o3djs.event');
o3djs.require('o3djs.math');
o3djs.require('o3djs.shader');

// requires "fpscounter.js"

var gl = null;
var g_canvas;
var g_math;
var g_requestId;
var g_width = 0;
var g_height = 0;
var g_fpsCounter = null;
var g_vpsDiv = null;
var phongShader = null;
// var STRIP_SIZE = 48;
var STRIP_SIZE = 144;
var BUFFER_LENGTH = 1000000;
var NUM_SLICES = 1;
var SIN_ARRAY_SIZE = 1024;
// var tileSize = 9 * STRIP_SIZE;
var tileSize = 3 * STRIP_SIZE;
var bigVBO = null;
var sliceInfo = null;
var clientArray = null;
var elementVBO = null;
var recomputeElements = false;
var xyArray = null;

var sinArray = null;
var cosArray = null;

// Primitive: gl.TRIANGLE_STRIP, gl.LINE_STRIP or gl.POINTS
var primitive = 0;

// Animation parameters
var hicoef = .06;
var locoef = .10;
var hifreq = 6.1;
var lofreq = 2.5;
var phaseRate = .02;
var phase2Rate = -0.12;
var phase  = 0;
var phase2 = 0;

// Temporaries for computation
// FIXME: change these to use Float32Array once reads are better optimized
var ysinlo = new Array(STRIP_SIZE);
var ycoslo = new Array(STRIP_SIZE);
var ysinhi = new Array(STRIP_SIZE);
var ycoshi = new Array(STRIP_SIZE);
// var ysinlo = new Float32Array(STRIP_SIZE);
// var ycoslo = new Float32Array(STRIP_SIZE);
// var ysinhi = new Float32Array(STRIP_SIZE);
// var ycoshi = new Float32Array(STRIP_SIZE);

// Element sizes in bytes.
var BYTES_PER_SHORT = Uint16Array.BYTES_PER_ELEMENT;
var BYTES_PER_FLOAT = Float32Array.BYTES_PER_ELEMENT;

//----------------------------------------------------------------------
// Utility functions
//

function output(str) {
    var div = document.createElement("div");
    div.innerHTML = str;
    document.body.appendChild(div);
}

//----------------------------------------------------------------------
// Keyboard input
//

var b = new Array(256);

function setFlag(key, val) {
    b[key.charCodeAt(0) & 0xFF] = val;
}

function getFlag(key) {
    return b[key.charCodeAt(0) & 0xFF];
}

//----------------------------------------------------------------------
// Main demo

function main() {
    g_math = o3djs.math;
    var fps = document.getElementById("fps");
    if (fps) {
        g_fpsCounter = new FPSCounter(fps);
    }

    g_canvas = document.getElementById("c");

    //g_canvas = WebGLDebugUtils.makeLostContextSimulatingCanvas(g_canvas);
    // tell the simulator when to lose context.
    //g_canvas.loseContextInNCalls(15);

    g_canvas.addEventListener('webglcontextlost', handleContextLost, false);
    g_canvas.addEventListener('webglcontextrestored', handleContextRestored, false);


    g_width = g_canvas.width;
    g_height = g_canvas.height;
    attribs = { antialias: false };
    gl = WebGLUtils.setupWebGL(g_canvas, attribs);
    if (!gl) {
        return false;
    }

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
    if (g_requestId !== undefined) {
      cancelAnimFrame(g_requestId);
      g_requestId = undefined;
    }
}

function handleContextRestored() {
    log("handle context restored");
    init();
}


function init() {
    gl.enable(gl.DEPTH_TEST);
    gl.clearColor(0, 0, 0, 1);
    // FIXME: port O3D's effect.js to WebGL
    phongShader = o3djs.shader.loadFromScriptNodes(gl, "vertexShader", "fragmentShader");
    phongShader.bind();
    gl.uniform4f(phongShader.emissiveColorLoc, 0, 0, 0, 0);
    gl.uniform4f(phongShader.ambientColorLoc, .1, .1, 0, 1);
    gl.uniform4f(phongShader.diffuseColorLoc, .6, .6, .1, 1);
    gl.uniform4f(phongShader.specularColorLoc, 1, 1, .75, 1);
    gl.uniform1f(phongShader.shininessLoc, 128);
    gl.uniform1f(phongShader.specularFactorLoc, 1);
    gl.uniform3f(phongShader.lightWorldPosLoc, .5, 0, .5);
    gl.uniform4f(phongShader.lightColorLoc, 1, 1, 1, 1);

    // FIXME: support animation of camera; move this matrix setup into
    // draw().
    var projection = g_math.matrix4.perspective(g_math.degToRad(60),
                                                g_width / g_height,
                                                0.1, 100);
    var model = g_math.matrix4.identity();
    var view = g_math.matrix4.identity();
    // FIXME: indices might be wrong
    view[3][2] = -1;
    var modelView = g_math.matrix4.mul(model, view);
    var modelViewProjection = g_math.matrix4.mul(modelView, projection);
    var viewInverse = g_math.matrix4.inverse(view);
    var worldInverseTranspose = g_math.transpose(g_math.matrix4.inverse(model));
    gl.uniformMatrix4fv(phongShader.worldViewProjectionLoc, false,
                        g_math.getMatrixElements(modelViewProjection));
    gl.uniformMatrix4fv(phongShader.worldLoc, false,
                        g_math.getMatrixElements(model));
    gl.uniformMatrix4fv(phongShader.viewInverseLoc, false,
                        g_math.getMatrixElements(viewInverse));
    gl.uniformMatrix4fv(phongShader.worldInverseTransposeLoc, false,
                        g_math.getMatrixElements(worldInverseTranspose));

    // Allocate the big VBO.
    allocateBigVBO(gl);

    // Set up the indices for the drawElements call.
    computeElements(gl);

    primitive = gl.TRIANGLE_STRIP;

    // FIXME: change these to use Float32Array once reads are better optimized
    sinArray = new Array(SIN_ARRAY_SIZE);
    cosArray = new Array(SIN_ARRAY_SIZE);
    // sinArray = new Float32Array(SIN_ARRAY_SIZE);
    // cosArray = new Float32Array(SIN_ARRAY_SIZE);
    for (var i = 0; i < SIN_ARRAY_SIZE; i++) {
        var step = i * 2 * Math.PI / SIN_ARRAY_SIZE;
        sinArray[i] = Math.sin(step);
        cosArray[i] = Math.cos(step);
    }

    // Start out animating
    setFlag(' ', true);

    g_vpsDiv = document.getElementById('vps');

    // Set up key handling.
    document.onkeypress = onKeyPress;
    draw();
}

/**
 *  Extracts the key char in number form from the event, in a cross-browser
 *  manner.
 * @param {!Event} event .
 * @return {number} unicode code point for the key.
 */
getEventKeyChar = function(event) {
    if (!event) {
        event = window.event;
    }
    var charCode = 0;
    if (event.keyIdentifier)
        charCode = o3djs.event.keyIdentifierToChar(event.keyIdentifier);
    if (!charCode)
        charCode = (window.event) ? window.event.keyCode : event.charCode;
    if (!charCode)
        charCode = event.keyCode;
    return charCode;
};

function onKeyPress(event) {
    event = event || window.event;
    var k = String.fromCharCode(getEventKeyChar(event));
    // Just in case they have capslock on.
    k = k.toLowerCase();
    if (event.shiftKey) {
        k = k.toUpperCase();
    }
    setFlag(k, !getFlag(k));

    if ('w' == k) {
        if (primitive != gl.LINE_STRIP)
            primitive = gl.LINE_STRIP;
        else
            primitive = gl.TRIANGLE_STRIP;
    }

    if ('p' == k) {
        if (primitive != gl.POINTS)
            primitive = gl.POINTS;
        else
            primitive = gl.TRIANGLE_STRIP;
    }

    if ('h' == k)
        hicoef += .005;
    if ('H' == k)
        hicoef -= .005;
    if ('l' == k)
        locoef += .005;
    if ('L' == k)
        locoef -= .005;
    if ('1' == k)
        lofreq += .1;
    if ('2' == k)
        lofreq -= .1;
    if ('3' == k)
        hifreq += .1;
    if ('4' == k)
        hifreq -= .1;
    if ('5' == k)
        phaseRate += .01;
    if ('6' == k)
        phaseRate -= .01;
    if ('7' == k)
        phase2Rate += .01;
    if ('8' == k)
        phase2Rate -= .01;

    if ('t' == k) {
        // if(tileSize < 864) {
        if(tileSize < 432) {
            tileSize += STRIP_SIZE;
            recomputeElements = true;
        }
    }

    if ('T' == k) {
        if(tileSize > STRIP_SIZE) {
            tileSize -= STRIP_SIZE;
            recomputeElements = true;
        }
    }
}

function allocateBigVBO(gl) {
    bigVBO = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, bigVBO);
    gl.bufferData(gl.ARRAY_BUFFER,
                  BUFFER_LENGTH * BYTES_PER_FLOAT,
                  gl.DYNAMIC_DRAW);
}

function computeElements(gl) {
    // FIXME: change this to use Float32Array once reads are better optimized
    xyArray = new Array(tileSize);
    // xyArray = new Float32Array(tileSize);
    for (var i = 0; i < tileSize; i++) {
        xyArray[i] = i / (tileSize - 1.0) - 0.5;
    }

    var elements = new Uint16Array((tileSize - 1) * (2 * STRIP_SIZE));
    var idx = 0;
    for (var i = 0; i < tileSize - 1; i++) {
        for (var j = 0; j < 2 * STRIP_SIZE; j += 2) {
            elements[idx++] = i * STRIP_SIZE + (j / 2);
            elements[idx++] = (i + 1) * STRIP_SIZE + (j / 2);
        }
    }
    elementVBO = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, elementVBO);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, elements, gl.STATIC_DRAW);
}

function setupSliceInfo(gl) {
    var sliceSize = tileSize * STRIP_SIZE * 6;
    var floatSize = BYTES_PER_FLOAT;
    if (clientArray == null || clientArray.length != sliceSize) {
        clientArray = new Float32Array(sliceSize);
        var clientArrayByteSize = clientArray.byteLength;
        var numSlices = Math.floor(BUFFER_LENGTH / sliceSize) | 0;
        sliceInfo = [];
        for (var i = 0; i < numSlices; i++) {
            var baseOffset = i * clientArrayByteSize;
            sliceInfo.push({ vertexOffset: baseOffset,
                             normalOffset: baseOffset + 3 * floatSize });
        }
    }
}

function draw() {
    // Set up the information about how the VBO is sliced.
    // Since the tile size can change at run time, we need to check
    // each time whether this needs to be updated.
    setupSliceInfo(gl);

    if (getFlag(' ')) {
        phase += phaseRate;
        phase2 += phase2Rate;

        if (phase > 20 * Math.PI)
            phase = 0;
        if (phase2 < -20 * Math.PI)
            phase2 = 0;
    }

    var loX = new PeriodicIterator(SIN_ARRAY_SIZE, 2 * Math.PI, phase, (1.0 / tileSize) * lofreq * Math.PI);
    var loY = loX.copy();
    var hiX = new PeriodicIterator(SIN_ARRAY_SIZE, 2 * Math.PI, phase2, (1.0 / tileSize) * hifreq * Math.PI);
    var hiY = hiX.copy();

    if (recomputeElements) {
        computeElements(gl);
        recomputeElements = false;
    }

    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    phongShader.bind();

    var numSlabs = tileSize / STRIP_SIZE;
    gl.bindBuffer(gl.ARRAY_BUFFER, bigVBO);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, elementVBO);
    var sizeOfShort = BYTES_PER_SHORT;
    gl.enableVertexAttribArray(phongShader.gPositionLoc);
    gl.enableVertexAttribArray(phongShader.gNormalLoc);
    var numVertsGeneratedPerFrame = 0;
    for (var slab = numSlabs; --slab >= 0; ) {
        var cur = slab % sliceInfo.length;
        var slice = sliceInfo[cur];
        var vertexIndex = 0;
        for (var jj = STRIP_SIZE; --jj >= 0; ) {
            ysinlo[jj] = sinArray[loY.getIndex()];
            ycoslo[jj] = cosArray[loY.getIndex()]; loY.incr();
            ysinhi[jj] = sinArray[hiY.getIndex()];
            ycoshi[jj] = cosArray[hiY.getIndex()]; hiY.incr();
        }
        loY.decr();
        hiY.decr();

        var locoef_tmp = locoef;
        var hicoef_tmp = hicoef;
        var ysinlo_tmp = ysinlo;
        var ysinhi_tmp = ysinhi;
        var ycoslo_tmp = ycoslo;
        var ycoshi_tmp = ycoshi;
        var cosArray_tmp = cosArray;
        var sinArray_tmp = sinArray;
        var xyArray_tmp = xyArray;

        for (var i = tileSize; --i >= 0; ) {
            var x = xyArray_tmp[i];
            var loXIndex = loX.getIndex();
            var hiXIndex = hiX.getIndex();

            var jOffset = (STRIP_SIZE-1) * slab;
            var nx = locoef_tmp * -cosArray_tmp[loXIndex] + hicoef_tmp * -cosArray_tmp[hiXIndex];

            var v = clientArray;

            for (var j = STRIP_SIZE; --j >= 0; ) {
                var y = xyArray_tmp[j + jOffset];
                var ny = locoef_tmp * -ycoslo_tmp[j] + hicoef_tmp * -ycoshi_tmp[j];
                v[vertexIndex] = x;
                v[vertexIndex + 1] = y;
                v[vertexIndex + 2] = (locoef_tmp * (sinArray_tmp[loXIndex] + ysinlo_tmp[j]) +
                                      hicoef_tmp * (sinArray_tmp[hiXIndex] + ysinhi_tmp[j]));
                v[vertexIndex + 3] = nx;
                v[vertexIndex + 4] = ny;
                v[vertexIndex + 5] = .15; //.15 * (1.0 - Math.sqrt(nx * nx + ny * ny));
                vertexIndex += 6;
                ++numVertsGeneratedPerFrame;
            }
            loX.incr();
            hiX.incr();
        }
        loX.reset();
        hiX.reset();

        gl.bufferSubData(gl.ARRAY_BUFFER, slice.vertexOffset, clientArray);
        gl.vertexAttribPointer(phongShader.gPositionLoc, 3, gl.FLOAT, false, 6 * BYTES_PER_FLOAT, slice.vertexOffset);
        gl.vertexAttribPointer(phongShader.gNormalLoc, 3, gl.FLOAT, false, 6 * BYTES_PER_FLOAT, slice.normalOffset);

        var len = tileSize - 1;
        for (var i = 0; i < len; i++) {
            gl.drawElements(primitive, 2 * STRIP_SIZE, gl.UNSIGNED_SHORT,
                            i * 2 * STRIP_SIZE * sizeOfShort);
        }
    }
    gl.disableVertexAttribArray(phongShader.gPositionLoc);
    gl.disableVertexAttribArray(phongShader.gNormalLoc);
    if (g_fpsCounter) {
        if (g_fpsCounter.update()) {
            if (g_vpsDiv) {
                g_vpsDiv.innerHTML = '' +
                    (numSlabs * tileSize * STRIP_SIZE * g_fpsCounter.getFPS()).toFixed(2) +
                    ' vertices generated per second';
            }
        }
    }
    g_requestId = window.requestAnimFrame(draw, g_canvas);
}
