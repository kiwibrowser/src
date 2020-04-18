/* -*- Mode: js2; js2-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40; -*- */

/*
 Copyright (c) 2009  Mozilla Corporation

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

var shaders = { };

function getShader(gl, id) {
    var shaderScript = document.getElementById(id);
    if (!shaderScript)
        return null;

    var str = "";
    var k = shaderScript.firstChild;
    while (k) {
        if (k.nodeType == 3)
            str += k.textContent;
        k = k.nextSibling;
    }

    var shader;
    if (shaderScript.type == "x-shader/x-fragment") {
        shader = gl.createShader(gl.FRAGMENT_SHADER);
    } else if (shaderScript.type == "x-shader/x-vertex") {
        shader = gl.createShader(gl.VERTEX_SHADER);
    } else {
        return null;
    }

    gl.shaderSource(shader, str);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS) && !gl.isContextLost()) {
        alert(gl.getShaderInfoLog(shader));
        return null;
    }

    return shader;
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

var sf = null;
var canvas;
var gl;
var addedHandlers = false;
var texturesBound = false;

function renderStart() {
  canvas = document.getElementById("canvas");

  //canvas = WebGLDebugUtils.makeLostContextSimulatingCanvas(canvas);

  canvas.addEventListener('webglcontextlost', handleContextLost, false);
  canvas.addEventListener('webglcontextrestored', handleContextRestored, false);

  // tell the simulator when to lose context.
  //canvas.loseContextInNCalls(15);

  gl = WebGLUtils.setupWebGL(canvas);

  if (!gl) {
    return;
  }

  init();
}

function init() {
  canvas.removeEventListener("mousedown", mouseDownHandler, false);
  canvas.removeEventListener("mousemove", mouseMoveHandler, false);
  canvas.removeEventListener("mouseup", mouseUpHandler, false);

  texturesBound = false;
  shaders = {};
  shaders.fs = getShader(gl, "shader-fs");
  shaders.vs = getShader(gl, "shader-vs");

  shaders.sp = gl.createProgram();
  gl.attachShader(shaders.sp, shaders.vs);
  gl.attachShader(shaders.sp, shaders.fs);
  gl.linkProgram(shaders.sp);

  if (!gl.getProgramParameter(shaders.sp, gl.LINK_STATUS) && !gl.isContextLost()) {
    alert(gl.getProgramInfoLog(shader));
  }

  gl.useProgram(shaders.sp);

  var sp = shaders.sp;

  var va = gl.getAttribLocation(sp, "aVertex");
  var na = gl.getAttribLocation(sp, "aNormal");
  var ta = gl.getAttribLocation(sp, "aTexCoord0");

  var mvUniform = gl.getUniformLocation(sp, "uMVMatrix");
  var pmUniform = gl.getUniformLocation(sp, "uPMatrix");
  var tex0Uniform = gl.getUniformLocation(sp, "uTexture0");

  var viewPositionUniform = gl.getUniformLocation(sp, "uViewPosition");
  var colorUniform = gl.getUniformLocation(sp, "uColor");

  if (colorUniform) {
    gl.uniform4fv(colorUniform, new Float32Array([0.1, 0.2, 0.4, 1.0]));
  }

  var pmMatrix = makePerspective(60, 1, 0.1, 100);
  //var pmMatrix = makePerspective(90, 1, 0.01, 10000);
  //var pmMatrix = makeOrtho(-20,20,-20,20, 0.01, 10000);

  var vpos = [0, 0, 0, 1];

  var mvMatrixStack = [];
  var mvMatrix = Matrix.I(4);

  function pushMatrix(m) {
    if (m) {
      mvMatrixStack.push(m.dup());
      mvMatrix = m.dup();
    } else {
      mvMatrixStack.push(mvMatrix.dup());
    }
  }

  function popMatrix() {
    if (mvMatrixStack.length == 0)
      throw "Invalid popMatrix!";
    mvMatrix = mvMatrixStack.pop();
    return mvMatrix;
  }

  function multMatrix(m) {
    //console.log("mult", m.flatten());
    mvMatrix = mvMatrix.x(m);
  }

  function setMatrixUniforms() {
    gl.uniformMatrix4fv(mvUniform, false, new Float32Array(mvMatrix.flatten()));
    gl.uniformMatrix4fv(pmUniform, false, new Float32Array(pmMatrix.flatten()));
    gl.uniform4fv(viewPositionUniform, new Float32Array(vpos));
/*
    console.log("mv", mvMatrix.flatten());
    console.log("pm", pmMatrix.flatten());
    console.log("nm", N.flatten());
*/
  }

  function mvTranslate(v) {
    var m = Matrix.Translation($V([v[0],v[1],v[2]])).ensure4x4();
    multMatrix(m);
  }

  function mvRotate(ang, v) {
    var arad = ang * Math.PI / 180.0;
    var m = Matrix.Rotation(arad, $V([v[0], v[1], v[2]])).ensure4x4();
    multMatrix(m);
  }

  function mvScale(v) {
    var m = Matrix.Diagonal([v[0], v[1], v[2], 1]);
    multMatrix(m);
  }

  function mvInvert() {
    mvMatrix = mvMatrix.inv();
  }

  // set up the vbos
  var buffers = { };
  buffers.position = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(sf.mesh.position), gl.STATIC_DRAW);

  gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
  gl.vertexAttribPointer(va, 3, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(va);

  if (na != -1) {
    buffers.normal = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.normal);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(sf.mesh.normal), gl.STATIC_DRAW);

    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.normal);
    gl.vertexAttribPointer(na, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(na);
  }

  if (ta != -1) {
    buffers.texcoord = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.texcoord);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(sf.mesh.texcoord), gl.STATIC_DRAW);

    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.texcoord);
    gl.vertexAttribPointer(ta, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(ta);
  }

  var numVertexPoints = sf.mesh.position.length / 3;

  gl.clearColor(0.2, 0.2, 0.2, 1.0);
  gl.clearDepth(1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.enable(gl.DEPTH_TEST);

  /*
  multMatrix(makeLookAt(vpos[0], vpos[1], vpos[2], 0, 0, 0, 0, 1, 0));
  mvTranslate([16.9485,11.1548,18.486]);
  mvRotate(45.2, [0, 1, 0]);
  mvRotate(-28.2, [1, 0, 0]);
  mvInvert();

  vpos = [-16.9485,-11.1548,-18.486];
*/

  var bbox = sf.mesh.bbox;

  var midx = (bbox.min.x + bbox.max.x) / 2;
  var midy = (bbox.min.y + bbox.max.y) / 2;
  var midz = (bbox.min.z + bbox.max.z) / 2;

  var maxdim = bbox.max.x - bbox.min.x;
  maxdim = Math.max(maxdim, bbox.max.y - bbox.min.y);
  maxdim = Math.max(maxdim, bbox.max.z - bbox.min.z);

  var m = makeLookAt(midx,midz,midy-(maxdim*1.5),
                     midx,midz,midy,
                     0,1,0);

  //m = makeLookAt(15, 75, -75, 0,0,0, 0,1,0);
  multMatrix(m);
  mvRotate(90,[1,0,0]);
  mvScale([1,1,-1]);

  var currentRotation = 0;

  function draw() {
    pushMatrix();
    mvRotate(currentRotation,[0,0,1]);

    setMatrixUniforms();

    // the texture might still be loading
    if (!texturesBound) {
      if (sf.textures.diffuse) {
          if (sf.textures.diffuse.complete) {
            if (sf.textures.diffuse.width > 0 && sf.textures.diffuse.height > 0) {
              // the texture is ready for binding
              var texid = gl.createTexture();
              gl.activeTexture(gl.TEXTURE0);
              gl.bindTexture(gl.TEXTURE_2D, texid);
              gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
              gl.texImage2D(
                  gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE,
                  sf.textures.diffuse);
              gl.generateMipmap(gl.TEXTURE_2D);

              gl.uniform1i(tex0Uniform, 0);
            }

            texturesBound = true;
          }
      } else {
        texturesBound = true;
      }
    }

    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.drawArrays(gl.TRIANGLES, 0, numVertexPoints);

    popMatrix();
  }

  draw();

  if (!sf.textures.diffuse.complete) {
    sf.textures.diffuse.onload = draw;
  }

  var mouseDown = false;
  var lastX = 0;

  function mouseDownHandler(ev) {
    mouseDown = true;
    lastX = ev.screenX;
    return true;
  }
  function mouseMoveHandler(ev) {
    if (!mouseDown)
      return false;
    var mdelta = ev.screenX - lastX;
    lastX = ev.screenX;
    currentRotation -= mdelta;
    while (currentRotation < 0)
      currentRotation += 360;
    if (currentRotation >= 360)
      currentRotation = currentRotation % 360;

    draw();
    return true;
  }

  function mouseUpHandler(ev) {
    mouseDown = false;
  }

  canvas.addEventListener("mousedown", mouseDownHandler, false);
  canvas.addEventListener("mousemove", mouseMoveHandler, false);
  canvas.addEventListener("mouseup", mouseUpHandler, false);

  var activeTouchIdentifier;

  function findActiveTouch(touches) {
    for (var ii = 0; ii < touches.length; ++ii) {
      if (touches.item(ii).identifier == activeTouchIdentifier) {
        return touches.item(ii);
      }
    }
    return null;
  }
  
  function touchStartHandler(ev) {
    if (mouseDown || ev.targetTouches.length == 0) {
      return;
    }
    var touch = ev.targetTouches.item(0);
    mouseDownHandler(touch);
    activeTouchIdentifier = touch.identifier;
    ev.preventDefault();
  }

  function touchMoveHandler(ev) {
    var touch = findActiveTouch(ev.changedTouches);
    if (touch) {
      mouseMoveHandler(touch);
    }
    ev.preventDefault();
  }

  function touchEndHandler(ev) {
    var touch = findActiveTouch(ev.changedTouches);
    if (touch) {
      mouseUpHandler(touch);
    }
    ev.preventDefault();
  }

  canvas.addEventListener("touchstart", touchStartHandler, false);
  canvas.addEventListener("touchmove", touchMoveHandler, false);
  canvas.addEventListener("touchend", touchEndHandler, false);
  canvas.addEventListener("touchcancel", touchEndHandler, false);
}

function handleLoad() {
  sf = new SporeFile();
  sf._loadHandler = renderStart;
  sf.load("creatures/Amahani.dae");
}

window.onload = handleLoad;
