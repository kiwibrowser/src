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

o3djs.require('o3djs.math');
o3djs.require('o3djs.quaternions');
o3djs.require('o3djs.particles');

// requires "cameracontroller.js"
// requires "fpscounter.js"

var gl = null;
var g_canvas;
var g_fpsCounter = null;
var g_width = 0;
var g_height = 0;
var g_requestId;
var g_particleSystem = null;
var g_controller = null;
var g_math;
var g_world;
var g_view;
var g_projection;
var g_textures = [];
var g_keyDown = [];
var g_poofs = [];
var g_poofIndex = 0;
var g_trail;
var g_trailParameters;
var g_startTime = new Date();
var g_animateCheckbox = null;

var MAX_POOFS = 3;

// FIXME: hack for debugging output
function output(str) {
    document.body.appendChild(document.createTextNode(str));
}

function checkGLError() {
    var error = gl.getError();
    if (error != gl.NO_ERROR) {
        var str = "GL Error: " + error;
        output(str);
        throw str;
    }
}

/**
 * Returns a deterministic pseudorandom number bewteen 0 and 1
 * @return {number} a random number between 0 and 1
 */
var g_randSeed = 0;
var g_randRange = Math.pow(2, 32);
function pseudoRandom() {
    return (g_randSeed = (134775813 * g_randSeed + 1) % g_randRange) /
            g_randRange;
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
    if (event.keyIdentifierToChar)
        charCode = o3djs.event.keyIdentifierToChar(event.keyIdentifier);
    if (!charCode)
        charCode = (window.event) ? window.event.keyCode : event.charCode;
    if (!charCode)
        charCode = event.keyCode;
    return charCode;
};

function onKeyPress(event) {
    event = event || window.event;

    var keyChar = String.fromCharCode(getEventKeyChar(event));
    // Just in case they have capslock on.
    keyChar = keyChar.toLowerCase();

    switch (keyChar) {
        case 'p':
            triggerPoof();
            break;
    }
}

function onKeyDown(event) {
    event = event || window.event;
    g_keyDown[event.keyCode] = true;
}

function onKeyUp(event) {
    event = event || window.event;
    g_keyDown[event.keyCode] = false;
}

function main() {
    g_math = o3djs.math;
    g_world = g_math.matrix4.identity();
    g_view = g_math.matrix4.identity();
    g_projection = g_math.matrix4.identity();
    g_canvas = document.getElementById("c");

//    g_canvas = WebGLDebugUtils.makeLostContextSimulatingCanvas(g_canvas);
    // tell the simulator when to lose context.
//    g_canvas.loseContextInNCalls(1);

    g_canvas.addEventListener('webglcontextlost', handleContextLost, false);
    g_canvas.addEventListener('webglcontextrestored', handleContextRestored, false);

    gl = WebGLUtils.setupWebGL(g_canvas);
    if (!gl)
        return;

	var ratio = window.devicePixelRatio ? window.devicePixelRatio : 1;
	g_canvas.width = 800 * ratio;
	g_canvas.height = 600 * ratio;
    g_width = g_canvas.width;
    g_height = g_canvas.height;
    controller = new CameraController(g_canvas);
    controller.onchange = function(xRot, yRot) {
        g_math.matrix4.setIdentity(g_view);
        g_math.matrix4.translate(g_view, [0, -100, -1500.0]);
        g_math.matrix4.rotateX(g_view, g_math.degToRad(-xRot));
        g_math.matrix4.rotateY(g_view, g_math.degToRad(-yRot));
    };
    var fps = document.getElementById("fps");
    if (fps) {
        g_fpsCounter = new FPSCounter(fps);
    }
    g_animateCheckbox = document.getElementById("animate");
    init();
    document.onkeypress = onKeyPress;
    document.onkeydown = onKeyDown;
    document.onkeyup = onKeyUp;
}


function log(msg) {
    if (window.console && window.console.log) {
        console.log(msg);
    }
}

function handleContextLost(e) {
    log("handle context lost");
    e.preventDefault();
    clearLoadingImages();
    if (g_requestId !== undefined) {
      window.cancelAnimFrame(g_requestId);
      g_requestId = undefined;
    }
}

function handleContextRestored() {
    log("handle context restored");
    init();
}

function init() {
    g_textures = [];
    gl.clearColor(0.5, 0.5, 0.5, 1.0);
    gl.viewport(0, 0, g_width, g_height);
    gl.enable(gl.DEPTH_TEST);
    g_projection = g_math.matrix4.perspective(g_math.degToRad(30),
                                              g_width / g_height, 0.1, 5000);
    g_math.matrix4.setIdentity(g_view);
    g_math.matrix4.translate(g_view, [0, -100, -1500.0]);
    g_particleSystem = new o3djs.particles.ParticleSystem(gl, null, pseudoRandom);

    g_textures.push(loadTexture("ripple.png"));
    g_textures.push(loadTexture("particle-anim.png"));

    setupFlame();
    setupNaturalGasFlame();
    setupSmoke();
    setupWhiteEnergy();
    setupRipples();
    setupText();
    setupRain();
    setupAnim();
    setupBall();
    setupCube();
    setupPoof();
    setupTrail();

    draw();
}

// Array of images curently loading
var g_loadingImages = [];

// Clears all the images currently loading.
// This is used to handle context lost events.
function clearLoadingImages() {
    for (var ii = 0; ii < g_loadingImages.length; ++ii) {
        g_loadingImages[ii].onload = undefined;
    }
    g_loadingImages = [];
}

// Loads a texture from the absolute or relative URL "src".
// Returns a WebGLTexture object.
// The texture is downloaded in the background using the browser's
// built-in image handling. Upon completion of the download, our
// onload event handler will be called which uploads the image into
// the WebGLTexture.
function loadTexture(src, opt_completionCallback) {
    // Create and initialize the WebGLTexture object.
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    //  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    // Give the texture something to render with until the image is loaded.
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    // Create a DOM image object.
    var image = new Image();
    // Remember the image so we can stop it if we get lost context.
    g_loadingImages.push(image);
    // Set up the onload handler for the image, which will be called by
    // the browser at some point in the future once the image has
    // finished downloading.
    image.onload = function() {
        // Remove the image from the list of images loading.
        g_loadingImages.splice(g_loadingImages.indexOf(image), 1);
        // This code is not run immediately, but at some point in the
        // future, so we need to re-bind the texture in order to upload
        // the image. Note that we use the JavaScript language feature of
        // closures to refer to the "texture" and "image" variables in the
        // containing function.
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
        gl.texImage2D(
            gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
        gl.generateMipmap(gl.TEXTURE_2D);
        if (opt_completionCallback) {
            opt_completionCallback();
        }
    };
    // Start downloading the image by setting its source.
    image.src = src;
    // Return the WebGLTexture object immediately.
    return texture;
}

function setupFlame() {
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setTranslation(-300, 0, 0);
    emitter.setState(o3djs.particles.ParticleStateIds.ADD);
    emitter.setColorRamp(
        [1, 1, 0, 1,
         1, 0, 0, 1,
         0, 0, 0, 1,
         0, 0, 0, 0.5,
         0, 0, 0, 0]);
    emitter.setParameters({
        numParticles: 20,
        lifeTime: 2,
        timeRange: 2,
        startSize: 50,
        endSize: 90,
        velocity:[0, 60, 0], velocityRange: [15, 15, 15],
        worldAcceleration: [0, -20, 0],
        spinSpeedRange: 4});
}

function setupNaturalGasFlame() {
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setTranslation(-200, 0, 0);
    emitter.setState(o3djs.particles.ParticleStateIds.ADD);
    emitter.setColorRamp(
        [0.2, 0.2, 1, 1,
         0, 0, 1, 1,
         0, 0, 1, 0.5,
         0, 0, 1, 0]);
    emitter.setParameters({
        numParticles: 20,
        lifeTime: 2,
        timeRange: 2,
        startSize: 50,
        endSize: 20,
        velocity:[0, 60, 0],
        worldAcceleration: [0, -20, 0],
        spinSpeedRange: 4});
}

function setupSmoke() {
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setTranslation(-100, 0, 0);
    emitter.setState(o3djs.particles.ParticleStateIds.BLEND);
    emitter.setColorRamp(
        [0, 0, 0, 1,
         0, 0, 0, 0]);
    emitter.setParameters({
        numParticles: 20,
        lifeTime: 2,
        timeRange: 2,
        startSize: 100,
        endSize: 150,
        velocity: [0, 200, 0], velocityRange: [20, 0, 20],
        worldAcceleration: [0, -25, 0],
        spinSpeedRange: 4});
}

function setupWhiteEnergy() {
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setTranslation(0, 0, 0);
    emitter.setState(o3djs.particles.ParticleStateIds.ADD);
    emitter.setColorRamp(
        [1, 1, 1, 1,
         1, 1, 1, 0]);
    emitter.setParameters({
        numParticles: 80,
        lifeTime: 2,
        timeRange: 2,
        startSize: 100,
        endSize: 100,
        positionRange: [100, 0, 100],
        velocityRange: [20, 0, 20]});
}

function setupRipples() {
    var emitter = g_particleSystem.createParticleEmitter(g_textures[0]);
    emitter.setTranslation(-200, 0, 300);
    emitter.setState(o3djs.particles.ParticleStateIds.BLEND);
    emitter.setColorRamp(
        [0.7, 0.8, 1, 1,
         1, 1, 1, 0]);
    emitter.setParameters({
        numParticles: 20,
        lifeTime: 2,
        timeRange: 2,
        startSize: 50,
        endSize: 200,
        positionRange: [100, 0, 100],
        billboard: false});
}

function setupText() {
    var image = [
        'X.....X..XXXXXX..XXXXX....XXXX...X....',
        'X.....X..X.......X....X..X.......X....',
        'X..X..X..XXXXX...XXXXX...X..XXX..X....',
        'X..X..X..X.......X....X..X....X..X....',
        '.XX.XX...XXXXXX..XXXXX....XXXX...XXXXX'];
    var height = image.length;
    var width = image[0].length;

    // Make an array of positions based on the text image.
    var positions = [];
    for (var yy = 0; yy < height; ++yy) {
        for (var xx = 0; xx < width; ++xx) {
            if (image[yy].substring(xx, xx + 1) == 'X') {
                positions.push([(xx - width * 0.5) * 10,
                                -(yy - height * 0.5) * 10]);
            }
        }
    }
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setTranslation(200, 200, 0);
    emitter.setState(o3djs.particles.ParticleStateIds.ADD);
    emitter.setColorRamp(
        [1, 0, 0, 1,
         0, 1, 0, 1,
         0, 0, 1, 1,
         1, 1, 0, 0]);
    emitter.setParameters({
        numParticles: positions.length * 4,
        lifeTime: 2,
        timeRange: 2,
        startSize: 25,
        endSize: 50,
        positionRange: [2, 0, 2],
        velocity: [1, 0, 1]},
        function(particleIndex, parameters) {
            //var index = particleIndex;
            var index = Math.floor(pseudoRandom() * positions.length);
            index = Math.min(index, positions.length - 1);
            parameters.position[0] = positions[index][0];
            parameters.position[1] = positions[index][1];
        });
}

function setupRain() {
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setTranslation(200, 200, 0);
    emitter.setState(o3djs.particles.ParticleStateIds.BLEND);
    emitter.setColorRamp(
        [0.2, 0.2, 1, 1]);
    emitter.setParameters({
        numParticles: 80,
        lifeTime: 2,
        timeRange: 2,
        startSize: 5,
        endSize: 5,
        positionRange: [100, 0, 100],
        velocity: [0,-150,0]});
}

function setupAnim() {
    var emitter = g_particleSystem.createParticleEmitter(g_textures[1]);
    emitter.setTranslation(300, 0, 0);
    emitter.setColorRamp(
        [1, 1, 1, 1,
         1, 1, 1, 1,
         1, 1, 1, 0]);
    emitter.setParameters({
        numParticles: 20,
        numFrames: 8,
        frameDuration: 0.25,
        frameStartRange: 8,
        lifeTime: 2,
        timeRange: 2,
        startSize: 50,
        endSize: 90,
        positionRange: [10, 10, 10],
        velocity:[0, 200, 0], velocityRange: [75, 15, 75],
        acceleration: [0, -150, 0],
        spinSpeedRange: 1});
}

function setupBall() {
    var emitter = g_particleSystem.createParticleEmitter(g_textures[0]);
    emitter.setTranslation(-400, 0, -200);
    emitter.setState(o3djs.particles.ParticleStateIds.BLEND);
    emitter.setColorRamp([1, 1, 1, 1,
                          1, 1, 1, 0]);
    emitter.setParameters({
        numParticles: 300,
        lifeTime: 2,
        timeRange: 2,
        startSize: 10,
        endSize: 50,
        colorMult: [1, 1, 0.5, 1], colorMultRange: [0, 0, 0.5, 0],
        billboard: false},
        function(particleIndex, parameters) {
            var matrix = g_math.matrix4.rotationY(pseudoRandom() * Math.PI * 2);
            g_math.matrix4.rotateX(matrix, pseudoRandom() * Math.PI);
            var position = g_math.matrix4.transformDirection(matrix, [0, 100, 0]);
            parameters.position = position;
            parameters.orientation = o3djs.quaternions.rotationToQuaternion(matrix);
        });
}


function setupCube() {
    var emitter = g_particleSystem.createParticleEmitter(g_textures[0]);
    emitter.setTranslation(200, 0, -300);
    emitter.setState(o3djs.particles.ParticleStateIds.ADD);
    emitter.setColorRamp(
        [1, 1, 1, 1,
         0, 0, 1, 1,
         1, 1, 1, 0]);
    emitter.setParameters({
        numParticles: 300,
        lifeTime: 2,
        timeRange: 2,
        startSize: 10,
        endSize: 50,
        colorMult: [0.8, 0.9, 1, 1],
        billboard: false},
        function(particleIndex, parameters) {
            var matrix = g_math.matrix4.rotationY(
                Math.floor(pseudoRandom() * 4) * Math.PI * 0.5);
            g_math.matrix4.rotateX(matrix,
                                   Math.floor(pseudoRandom() * 3) * Math.PI * 0.5);
            parameters.orientation = o3djs.quaternions.rotationToQuaternion(matrix);
            var position = g_math.matrix4.transformDirection(
                matrix,
                [pseudoRandom() * 200 - 100, 100, pseudoRandom() * 200 - 100]);
            parameters.position = position;
        });
}

function setupPoof() {
    var emitter = g_particleSystem.createParticleEmitter();
    emitter.setState(o3djs.particles.ParticleStateIds.ADD);
    emitter.setColorRamp(
        [1, 1, 1, 0.3,
         1, 1, 1, 0]);
    emitter.setParameters({
        numParticles: 30,
        lifeTime: 1.5,
        startTime: 0,
        startSize: 50,
        endSize: 200,
        spinSpeedRange: 10},
        function(index, parameters) {
            var angle = Math.random() * 2 * Math.PI;
            parameters.velocity = g_math.matrix4.transformPoint(
                g_math.matrix4.rotationY(angle), [300, 0, 0]);
            parameters.acceleration = g_math.mulVectorVector(
                parameters.velocity, [-0.3, 0, -0.3]);
        });
    // make 3 poofs one shots
    for (var ii = 0; ii < MAX_POOFS; ++ii) {
        g_poofs[ii] = emitter.createOneShot();
    }
}

function triggerPoof() {
    // We have multiple poofs because if you only have one and it is still going
    // when you trigger it a second time it will immediately start over.
    g_poofs[g_poofIndex].trigger([100 + 100 * g_poofIndex, 0, 300]);
    g_poofIndex++;
    if (g_poofIndex == MAX_POOFS) {
        g_poofIndex = 0;
    }
}

function setupTrail() {
    g_trailParameters = {
        numParticles: 2,
        lifeTime: 2,
        startSize: 10,
        endSize: 90,
        velocityRange: [20, 20, 20],
        spinSpeedRange: 4};
    g_trail = g_particleSystem.createTrail(
        1000,
        g_trailParameters);
    g_trail.setState(o3djs.particles.ParticleStateIds.ADD);
    g_trail.setColorRamp(
        [1, 0, 0, 1,
         1, 1, 0, 1,
         1, 1, 1, 0]);
}

function leaveTrail() {
    var trailClock = (new Date().getTime() / 1000.0) * -0.8;
    g_trail.birthParticles(
        [Math.sin(trailClock) * 400, 200, Math.cos(trailClock) * 400]);
}

function draw() {
    // Note: the viewport is automatically set up to cover the entire Canvas.
    if (g_animateCheckbox && g_animateCheckbox.checked) {
        var cameraClock = (new Date().getTime() - g_startTime.getTime()) / 1000.0 * 0.3;
        g_view = g_math.matrix4.lookAt(
            [Math.sin(cameraClock) * 1500, 500, Math.cos(cameraClock) * 1500],  // eye
            [0, 100, 0],  // target
            [0, 1, 0]);   // up
    }

    // NOTE: we disable writes to the alpha channel for this sample
    // because we want to treat it as totally opaque, not blending with
    // the web page's background content.
    gl.colorMask(true, true, true, true);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.colorMask(true, true, true, false);

    if (g_keyDown[84]) {  // 'T' key.
        leaveTrail();
    }

    var viewProjection = g_math.mulMatrixMatrix4(g_view, g_projection);
    var viewInverse = g_math.inverse4(g_view);
    g_particleSystem.draw(viewProjection, g_world, viewInverse);
    if (g_fpsCounter) {
        g_fpsCounter.update();
    }
    g_requestId = window.requestAnimFrame(draw, g_canvas);
}
