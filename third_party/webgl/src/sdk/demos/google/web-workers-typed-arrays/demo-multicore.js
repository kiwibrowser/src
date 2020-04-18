/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. * Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution. * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *
 * COPYRIGHT NVIDIA CORPORATION 2003. ALL RIGHTS RESERVED. BY ACCESSING OR USING
 * THIS SOFTWARE, YOU AGREE TO:
 *
 * 1) ACKNOWLEDGE NVIDIA'S EXCLUSIVE OWNERSHIP OF ALL RIGHTS IN AND TO THE
 * SOFTWARE;
 *
 * 2) NOT MAKE OR DISTRIBUTE COPIES OF THE SOFTWARE WITHOUT INCLUDING THIS
 * NOTICE AND AGREEMENT;
 *
 * 3) ACKNOWLEDGE THAT TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS
 * SOFTWARE IS PROVIDED *AS IS* AND THAT NVIDIA AND ITS SUPPLIERS DISCLAIM ALL
 * WARRANTIES, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL,
 * INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT
 * LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS
 * OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS), INCLUDING ATTORNEYS'
 * FEES, RELATING TO THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF
 * NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 */

/**
 * @fileoverview Demonstration of using typed arrays and web workers to compute
 *               images for WebGL. Can run in single or multithreaded mode with
 *               and without transferable typed arrays to demonstrate the speed
 *               advantages of using web workers and transferable typed arrays.
 * @author wiltzius@google.com (Tom Wiltzius)
 *
 */

o3djs.require('o3djs.event');
o3djs.require('o3djs.math');
o3djs.require('o3djs.shader');

// ----------------------------------------------------------------------
// Constants and Settings
//

/**
 * Element sizes in bytes.
 */
var BYTES_PER_SHORT = Uint16Array.BYTES_PER_ELEMENT;
var BYTES_PER_FLOAT = Float32Array.BYTES_PER_ELEMENT;

/**
 * Fiddles with some related variables such that there's no effect on the
 * computed image, but makes it easier to parallelize by increasing the number
 * of slabs used to render it.
 */
var MULTIPLIER = 2;

/**
 * Only used for multithreaded operation; how many threads to spawn. NOTE: this
 * can only be as big as the number of slabs, which is tileSize / STRIP_SIZE.
 * Beyond that you spawn useless workers.
 */
var THREAD_POOL_SIZE = 3; 

/**
 * Constants that are shared between this thread and others
 */
var constants = {
    SIN_ARRAY_SIZE: 1024,
    STRIP_SIZE: 144 / MULTIPLIER
};

/**
 * GL-related global variables
 */
var gl = null;
var g_canvas;
var g_math;
var g_width = 0;
var g_height = 0;
var g_fpsCounter = null;
var g_vpsDiv = null;
var phongShader = null;
var BUFFER_LENGTH = 1000000;
var bigVBO = null;
var elementVBO = null;
var primitive = 0; // Primitive: gl.TRIANGLE_STRIP, gl.LINE_STRIP or gl.POINTS

/**
 * Calculation-related global variables
 */
var allWorkers = null;
/**
 * Queue of slabs to be computed
 */
var slabComputeQueue = null;
var allSlabs = null;
var allSlabsReady = null;
var precalc = {
        sinArray : new Float32Array(constants.SIN_ARRAY_SIZE),
        cosArray : new Float32Array(constants.SIN_ARRAY_SIZE),
        constants : constants
    };
// precalculate sin and cos
for (var i = 0; i < constants.SIN_ARRAY_SIZE; i++) {
    var step = i * 2 * Math.PI / constants.SIN_ARRAY_SIZE;
    precalc.sinArray[i] = Math.sin(step);
    precalc.cosArray[i] = Math.cos(step);
}

var g_slabData;

/**
 * Number of workers we're still waiting on before we can draw a frame
 */
var pendingSlabs;

/**
 * Configuration variables shared between this thread and others
 */
var config = {
    tileSize: 3 * constants.STRIP_SIZE * MULTIPLIER,
    hicoef: .06,
    locoef: .10,
    hifreq: 6.1,
    lofreq: 2.5,
    phaseRate: .02,
    phase2Rate: -0.12,
    phase: 0,
    phase2: 0,
    useTransferables: true,
    multithreaded: true
};

// ----------------------------------------------------------------------
// Utility functions
//

/**
 * Writes the given string to the end of the document in a new div.
 *
 * @param {string}
 *            str The string to be written.
 */
function output(str) {
    var div = document.createElement('div');
    div.innerHTML = str;
    document.body.appendChild(div);
}

// ----------------------------------------------------------------------
// Keyboard input
//

var b = new Array(256);

function setFlag(key, val) {
    b[key.charCodeAt(0) & 0xFF] = val;
}

function getFlag(key) {
    return b[key.charCodeAt(0) & 0xFF];
}

// ----------------------------------------------------------------------
// Main demo
//

function initUI() {
    var useTransferablesCheckbox = document.querySelector('#transferablesBoolean');
    if (supportsTransferables()) {
        useTransferablesCheckbox.checked = true;
    } else {
        useTransferablesCheckbox.checked = false;
        useTransferablesCheckbox.disabled = true;
    }
    document.querySelector('#workerCount').value = THREAD_POOL_SIZE;
}

/**
 * Sets up the frame rate counter and keyboard handling, then kicks off initial
 * configuration load and animation loop.
 */
function main() {
    g_math = o3djs.math;
    var fps = document.getElementById('fps');
    if (fps) {
        g_fpsCounter = new FPSCounter(fps);
    }

    // Set up key handling.
    document.onkeypress = onKeyPress;

    // do initial configuration load
    initUI();
    updateSettings();
}

/**
 * Sets up the GL canvas and spawns worker threads if relevant.
 *
 * @return {boolean} false if the GL canvas allocation fails, true otherwise.
 */
function init() {
    g_canvas = document.getElementById('c');
    g_width = g_canvas.width;
    g_height = g_canvas.height;
    attribs = {
        antialias: false
    };
    gl = WebGLUtils.setupWebGL(g_canvas, attribs);
    if (!gl) {
        console.log('Could not get a WebGL context!');
        return false;
    }
    gl.enable(gl.DEPTH_TEST);
    gl.clearColor(0, 0, 0, 1);
    // FIXME: port O3D's effect.js to WebGL
    phongShader = o3djs.shader.loadFromScriptNodes(gl, 'vertexShader',
            'fragmentShader');
    phongShader.bind();
    gl.uniform4f(phongShader.emissiveColorLoc, 0, 0, 0, 0);
    gl.uniform4f(phongShader.ambientColorLoc, .1, .1, 0, 1);
    gl.uniform4f(phongShader.diffuseColorLoc, .6, .6, .1, 1);
    gl.uniform4f(phongShader.specularColorLoc, 1, 1, .75, 1);
    gl.uniform1f(phongShader.shininessLoc, 128);
    gl.uniform1f(phongShader.specularFactorLoc, 1);
    gl.uniform3f(phongShader.lightWorldPosLoc, .5, 0, .5);
    gl.uniform4f(phongShader.lightColorLoc, 1, 1, 1, 1);

    // FIXME: support animation of camera; move this matrix setup into draw().
    var projection = g_math.matrix4.perspective(g_math.degToRad(60),
            g_width / g_height, 0.1, 100);
    var model = g_math.matrix4.identity();
    var view = g_math.matrix4.identity();
    // FIXME: indices might be wrong
    view[3][2] = -1;
    var modelView = g_math.matrix4.mul(model, view);
    var modelViewProjection = g_math.matrix4.mul(modelView, projection);
    var viewInverse = g_math.matrix4.inverse(view);
    var worldInverseTranspose = g_math.transpose(g_math.matrix4.inverse(model));
    gl.uniformMatrix4fv(phongShader.worldViewProjectionLoc, false, g_math
            .getMatrixElements(modelViewProjection));
    gl.uniformMatrix4fv(phongShader.worldLoc, false, g_math
            .getMatrixElements(model));
    gl.uniformMatrix4fv(phongShader.viewInverseLoc, false, g_math
            .getMatrixElements(viewInverse));
    gl.uniformMatrix4fv(phongShader.worldInverseTransposeLoc, false, g_math
            .getMatrixElements(worldInverseTranspose));

    // Allocate the big VBO.
    allocateBigVBO(gl);

    // The first time around we need to recompute a bunch of indices
    allSlabs = null;

    primitive = gl.TRIANGLE_STRIP;

    // Spawn workers if multithreading
    if (config.multithreaded) {
        setUpWorkers(config.numWorkers);
    }

    // Start out animating
    setFlag(' ', true);

    g_vpsDiv = document.getElementById('vps');
    return true;
}

/**
 * Reads settings from the page and restarts drawing.
 */
function resetSettings(newConfig) {

    // clear any pending animations left over from old requestAnimFrame events
    pendingAnimationQueue = [];
    config.multithreaded = newConfig.multithreaded;
    config.useTransferables = newConfig.useTransferables;
    config.numWorkers = newConfig.numWorkers;
    g_fpsCounter.reset();

    console.log('multithreaded? ' + config.multithreaded);

    console.log('using transferables? ' + config.useTransferables);

    console.log('thread pool size: ' + config.numWorkers);

    // re-init, since we can't make these changes in the middle of rendering
    init();
    draw();
}

/**
 * Update settings from state of checkboxes on the page
 */
function updateSettings() {
    var newConfig =
        {   multithreaded : document.querySelector('#workerBoolean').checked,
            useTransferables : supportsTransferables() && document.querySelector('#transferablesBoolean').checked
        };
    if (newConfig.multithreaded) {
        var count = document.querySelector('#workerCount').value;
        if (count) {
            newConfig.numWorkers = count;
        }
        else {
            newConfig.numWorkers = 3;
        }
    }
    document.querySelector('#fps').innerHTML = "calculating frames per second...";
    document.querySelector('#vps').innerHTML = "calculating vertices generated per second..."
    resetSettings(newConfig);
}

/**
 * Extracts the key char in number form from the event, in a cross-browser
 * manner.
 *
 * @param {!Event}
 *            event .
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

/**
 * Updates settings when keys are pressed.
 *
 * @param {Event}
 *            event The keyboard event to handle.
 */
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
        config.hicoef += .005;
    if ('H' == k)
        config.hicoef -= .005;
    if ('l' == k)
        config.locoef += .005;
    if ('L' == k)
        config.locoef -= .005;
    if ('1' == k)
        config.lofreq += .1;
    if ('2' == k)
        config.lofreq -= .1;
    if ('3' == k)
        config.hifreq += .1;
    if ('4' == k)
        config.hifreq -= .1;
    if ('5' == k)
        config.phaseRate += .01;
    if ('6' == k)
        config.phaseRate -= .01;
    if ('7' == k)
        config.phase2Rate += .01;
    if ('8' == k)
        config.phase2Rate -= .01;

    if ('t' == k) {
        // if(config_const.tileSize < 864) {
        if (config.tileSize < 864) { // 432
            config.tileSize += constants.STRIP_SIZE;
            allSlabs = null;
        }
    }

    if ('T' == k) {
        if (config.tileSize > constants.STRIP_SIZE) {
            config.tileSize -= constants.STRIP_SIZE;
            allSlabs = null;
        }
    }

}

/**
 * Create the main GL buffer and bind it to the canvas.
 *
 * @param {Object} gl
 *            The global WebGL context.
 */
function allocateBigVBO(gl) {
    bigVBO = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, bigVBO);
    gl.bufferData(gl.ARRAY_BUFFER, BUFFER_LENGTH * BYTES_PER_FLOAT,
            gl.DYNAMIC_DRAW);
}

/**
 * Computes elements and passes them to the WebGL context
 *
 * @param {Object} gl
 *            The global WebGL context.
 */
function computeElements(gl) {
    console.log('recomputing elements, tile size:' + config.tileSize);

    var elements =
        new Uint16Array((config.tileSize - 1) * (2 * constants.STRIP_SIZE));
    var idx = 0;
    for (var i = 0; i < config.tileSize - 1; i++) {
        for (var j = 0; j < 2 * constants.STRIP_SIZE; j += 2) {
            elements[idx++] = i * constants.STRIP_SIZE + (j / 2);
            elements[idx++] = (i + 1) * constants.STRIP_SIZE + (j / 2);
        }
    }
    elementVBO = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, elementVBO);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, elements, gl.STATIC_DRAW);
}

/**
 * Sets up the sliceInfo array for the given slab.
 *
 * @param {Object} gl
 *            The global WebGL context.
 * @param {Object} slabInfo
 *            the slabInfo object to compute slice information for.
 */
function setupSliceInfo(gl, slabInfo) {
    var sliceSize = config.tileSize * constants.STRIP_SIZE * 6;
    var floatSize = BYTES_PER_FLOAT;
    if (slabInfo.conf.clientArray == null ||
            slabInfo.conf.clientArray.length != sliceSize) {
        slabInfo.conf.clientArray = new Float32Array(sliceSize);
        var clientArrayByteSize = slabInfo.conf.clientArray.byteLength;
        var numSlices = Math.floor(BUFFER_LENGTH / sliceSize) | 0;
        slabInfo.sliceInfo = [];
        for (var i = 0; i < numSlices; i++) {
            var baseOffset = i * clientArrayByteSize;
            slabInfo.sliceInfo.push({
                vertexOffset: baseOffset,
                normalOffset: baseOffset + 3 * floatSize
            });
        }
    }
}

/**
 * First stage of the drawing process; computes a number of trig arrays to later
 * be used in computing location of actual vertices.
 *
 * @param {Object}
 *            slabInfo slabInfo object to set up arrays for.
 * @param {PeriodicIterator}
 *            loY config.phase periodic iterator.
 * @param {PeriodicIterator}
 *            hiY config.phase2 periodic iterator.
 */
function computeSlabDataArrays(curSlabDataArrays, loY, hiY) {
    for (var jj = constants.STRIP_SIZE; --jj >= 0;) {
        curSlabDataArrays.ysinlo[jj] = precalc.sinArray[loY.getIndex()];
        curSlabDataArrays.ycoslo[jj] = precalc.cosArray[loY.getIndex()];
        loY.incr();
        curSlabDataArrays.ysinhi[jj] = precalc.sinArray[hiY.getIndex()];
        curSlabDataArrays.ycoshi[jj] = precalc.cosArray[hiY.getIndex()];
        hiY.incr();
    }
    loY.decr();
    hiY.decr();
}

/**
 * Takes the vertices specified in the slab's client array and pushes them to
 * the GPU using gl.drawElements
 *
 * @param {Object} slabInfo
 *            the slab to draw.
 */
function drawSlabElements(slabInfo) {
    var cur = slabInfo.conf.slab % slabInfo.sliceInfo.length;
    var slice = slabInfo.sliceInfo[cur];

    gl.bufferSubData(gl.ARRAY_BUFFER, slice.vertexOffset,
            slabInfo.conf.clientArray);
    gl.vertexAttribPointer(phongShader.gPositionLoc, 3, gl.FLOAT, false,
            6 * BYTES_PER_FLOAT, slice.vertexOffset);
    gl.vertexAttribPointer(phongShader.gNormalLoc, 3, gl.FLOAT, false,
            6 * BYTES_PER_FLOAT, slice.normalOffset);

    var len = config.tileSize - 1;
    for (var i = 0; i < len; i++) {
        gl.drawElements(primitive, 2 * constants.STRIP_SIZE, gl.UNSIGNED_SHORT,
                i * 2 * constants.STRIP_SIZE * BYTES_PER_SHORT);
    }
}

/**
 * Updates some framerate counters and sets up another pending animation request
 * using window.requestAnimFrame
 */
var frameStop;  // This can be set to only allow a certain number of frames to
                // render before pausing, useful for debugging (e.g. set to 1)
var pendingAnimationQueue = []; // We keep a queue of pending draw requests so
                                // we can clear it if settings change.
var update_count = 0;
function reanimate() {
    if (frameStop <= 0)
        return;
    frameStop--;
    gl.disableVertexAttribArray(phongShader.gPositionLoc);
    gl.disableVertexAttribArray(phongShader.gNormalLoc);
    if (g_fpsCounter) {
        if (g_fpsCounter.update()) {
            // we've updated the framerate
            update_count++;
            if(bench && update_count >= bench.UPDATE_COUNT) {
                console.log('updating benchmark');
                update_count = 0;
                bench.update_benchmark();
                return;
            }
            if (g_vpsDiv) {
                g_vpsDiv.innerHTML = '' +
                            (allSlabs.length * config.tileSize *
                            constants.STRIP_SIZE *
                            g_fpsCounter.getFPS()).toFixed(2) +
                            ' vertices generated per second';
            }
        }
    }
    draw();
  }

/**
 * Callback function to asynchronously complete the drawing process. This
 * function handles the postMessage received from worker threads who have
 * completed their drawing.
 *
 * @param {Event}
 *            event postMessage event object.
 */
function processWorker(event) {
    // msg.sender is the slab#, msg.data is the actual payload, and msg.type
    // tells
    // us what to do with it
    var msg = event.data;
    var worker = event.target;
    switch (msg.type) {
    case 'console':
        // console.log('from worker#' + msg.sender + ':' + msg.data);
        break;
    case 'result':
        var slabInfo = allSlabs[msg.sender];
        slabInfo.conf = msg.data;

        // increment slab
        slabDone();

        break;
    }

}

/**
 * Loops over all slabs and draws them, then schedules the next frame draw.
 * Should only be called after all slabs are finished being computed (i.e. have
 * completed client arrays; when all workers are finished).
 */
function drawAllSlabsAndReAnimate() {
    var tmp = allSlabs;
    allSlabs = allSlabsReady;
    allSlabsReady = tmp;
    reanimate();
    if (allSlabsReady) {
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        phongShader.bind();

        gl.bindBuffer(gl.ARRAY_BUFFER, bigVBO);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, elementVBO);
        gl.enableVertexAttribArray(phongShader.gPositionLoc);
        gl.enableVertexAttribArray(phongShader.gNormalLoc);

        allSlabsReady.forEach(function(slabInfo) {
            drawSlabElements(slabInfo);
        });
    }
}

/**
 * Recursively spawns calculation threads (or just recursively calculates in
 * single-threaded mode). Should be called repeatedly until all the slabs are
 * done (pendingSlabs == 0). If there are no available workers it will simple
 * return.
 */
function startCalculation() {
    var workerIndex = 0;
    for (var slabIndex = 0; slabIndex < slabComputeQueue.length; slabIndex++) {
        // Option 1: blocking calculate
        if (!config.multithreaded) {
            // pull a slab off the queue
            var slabInfo = slabComputeQueue[slabIndex];
            calculate(slabInfo.conf, precalc, g_slabData);
            slabDone();
        }
        // Option 2: use the thread pool to calculate
        else {
            // pull a slab off the queue
            var slabInfo = slabComputeQueue[slabIndex];
            // pull a worker to work on this slab
            var worker = allWorkers[workerIndex++];
            if (workerIndex >= allWorkers.length) workerIndex = 0;
            // Use the webkit postMessage if its available
            worker.postMessage = worker.webkitPostMessage || worker.postMessage;
            // If the browser supports transferables and we're configured to use
            // them, do so.
            if (config.useTransferables) {
                worker.postMessage(slabInfo.conf, [slabInfo.conf.clientArray.buffer]);
            } else {
                worker.postMessage(slabInfo.conf);
            }
        }
    }
    slabComputeQueue = [];
}

/**
 * Decrements the number of pending slabs and draws them all if all slabs are
 * finished. Should be called once for each computed slab once that slab is
 * finished.
 */
function slabDone() {
    // decrement pending slabs still to be computed
    pendingSlabs--;

    if (pendingSlabs == 0) {
        pendingAnimationQueue.push(drawAllSlabsAndReAnimate);
        window.requestAnimFrame(function() {
            if (pendingAnimationQueue.length != 0) {
                func = pendingAnimationQueue.shift();
                func();
            }
        }, g_canvas);
    }
}

/**
 * Spawn the web workers we'll need to do computation in a multithreaded
 * scenario.
 *
 * @param {int}
 *            count the number of workers to spawn.
 */
var globalCounter = 0;
function setUpWorkers(count) {
    if (allWorkers == null)
        allWorkers = [];

    // Kill any existing workers (this seems excessive, but if we don't we get
    // phantom postMessages coming back to haunt us)
    allWorkers.forEach(function(worker) {
        console.log('terminating worker id#' + worker.globalID);
        worker.terminate();
    });
    allWorkers = [];

    // Set up new workers
    console.log('spawning ' + count + ' worker(s)');
    if (config.numWorkers > config.tileSize / constants.STRIP_SIZE) {
        console.error('Warning! More threads than slabs to compute!' +
                'Probably a misconfiguration!');
    }
    for (var i = 0; i < count; i++) {
        var newWorker = new Worker('worker.js');
        newWorker.onmessage = processWorker;
        newWorker.globalID = globalCounter++;
        console.log('creating worker #' + newWorker.globalID);
        newWorker.postMessage({id:"precalc", precalc:precalc});
        allWorkers.push(newWorker);
    }
}

/**
 * Set up the slabInfo constructs for every slab we need to compute for this
 * frame.
 *
 * @param {number}
 *            numSlabs number of slabs to set up.
 */
function setUpSlabs(numSlabs) {
    console.log('setting up slabs');

    allSlabs = [];

    for (var slab = 0; slab < numSlabs; slab++) {
        // this object will hold everything we need for this slab
        var newSlab = {};

        // we store all information that gets passed to and from the calculation
        // function in the slab.conf object
        newSlab.conf = {};
        newSlab.conf.slab = slab;

        allSlabs[slab] = newSlab;
    }
}

/**
 * Recompute slabs and element arrays if necessary, then either directly perform
 * or delegate to workers the calculation of all elements in those slabs.
 */
function draw() {

    if (getFlag(' ')) {
        config.phase += config.phaseRate;
        config.phase2 += config.phase2Rate;

        if (config.phase > 20 * Math.PI)
            config.phase = 0;
        if (config.phase2 < -20 * Math.PI)
            config.phase2 = 0;
    }

    var numSlabs = config.tileSize / constants.STRIP_SIZE;

    if (allSlabs == null) {
        setUpSlabs(numSlabs);
        computeElements(gl);
    }

    var loY = new PeriodicIterator(constants.SIN_ARRAY_SIZE, 2 * Math.PI,
            config.phase, (1.0 / config.tileSize) * config.lofreq * Math.PI);
    var hiY = new PeriodicIterator(constants.SIN_ARRAY_SIZE, 2 * Math.PI,
            config.phase2, (1.0 / config.tileSize) * config.hifreq * Math.PI);

    slabComputeQueue = [];
    allSlabs.forEach(function(slabInfo) {
        // Copy the current state of the config to this slab before doing any
        // calculation, as it changes every draw() cycle
        slabInfo.conf.config = config;
        // Set up the information about how the VBO is sliced. Since the tile
        // size can change at run time, we need to check each time whether this
        // needs to be updated.
        setupSliceInfo(gl, slabInfo);
        // Put it on the compute queue
        slabComputeQueue.push(slabInfo);
    });

    g_slabData = {
        xyArray : new Float32Array(config.tileSize),
        arrays : Array(numSlabs)
    };
    for (var i = 0; i < config.tileSize; i++) {
        g_slabData.xyArray[i] = i / (config.tileSize - 1.0) - 0.5;
    }

    for (var slab = numSlabs; --slab >= 0;) {
        g_slabData.arrays[slab] = {
            ysinlo: new Float32Array(constants.STRIP_SIZE),
            ycoslo: new Float32Array(constants.STRIP_SIZE),
            ysinhi: new Float32Array(constants.STRIP_SIZE),
            ycoshi: new Float32Array(constants.STRIP_SIZE),
        };

        var curSlabDataArrays = g_slabData.arrays[slab];
        computeSlabDataArrays(curSlabDataArrays, loY, hiY);
    }

    if (config.multithreaded) {
        allWorkers.forEach(function(w) {
            w.postMessage({id:"slabData", slabData:g_slabData,config:config});
        });
    }

    // set pendingWorkers to be the number of slabs so we know how many we need
    // to wait for after starting them all off
    pendingSlabs = numSlabs;

    // kick off the calculating!
    startCalculation();

}

var bench;
function start_benchmark() {
    bench = new benchmark();
    bench.start();
}
