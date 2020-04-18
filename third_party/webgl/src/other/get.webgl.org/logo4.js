tdl.require('tdl.buffers');
tdl.require('tdl.fast');
tdl.require('tdl.log');
tdl.require('tdl.math');
tdl.require('tdl.models');
tdl.require('tdl.primitives');
tdl.require('tdl.programs');
tdl.require('tdl.textures');
tdl.require('tdl.webgl');

function setupLogo() {
  var program = tdl.programs.loadProgramFromScriptTags(
      'modelVertexShader',
      'modelFragmentShader');
  var arrays = {
    position: new tdl.primitives.AttribBuffer(
       3, [
       -1, -1, -1,
        1, -1, -1,
        1,  1, -1,
       -1,  1, -1,
       -1, -1,  1,
        1, -1,  1,
        1,  1,  1,
       -1,  1,  1
    ]),
    indices: new tdl.primitives.AttribBuffer(2, [
      0, 1, 1, 2, 2, 3, 3, 0,
      4, 5, 5, 6, 6, 7, 7, 4,
      0, 4, 1, 5, 2, 6, 3, 7
    ], 'Uint16Array')
  };
  return new tdl.models.Model(program, arrays, {}, gl.LINES);
};

var g_eyeSpeed  = 1;
var g_eyeHeight = 4.5;
var g_eyeRadius = 5;
var g_fov       = 30;
var g_trans     = [0,0,0];

function initializeLogo(canvas) {
  var math = tdl.math;
  var fast = tdl.fast;
  var model = setupLogo();

  var clock = 0.0;

  // pre-allocate a bunch of arrays
  var projection = new Float32Array(16);
  var view = new Float32Array(16);
  var world = new Float32Array(16);
  var worldInverse = new Float32Array(16);
  var worldInverseTranspose = new Float32Array(16);
  var viewProjection = new Float32Array(16);
  var worldViewProjection = new Float32Array(16);
  var eyePosition = new Float32Array(3);
  var target = new Float32Array(3);
  var up = new Float32Array([0,1,0]);
  var v3t0 = new Float32Array(3);
  var v3t1 = new Float32Array(3);
  var v3t2 = new Float32Array(3);
  var v3t3 = new Float32Array(3);
  var m4t0 = new Float32Array(16);
  var m4t1 = new Float32Array(16);
  var m4t2 = new Float32Array(16);
  var m4t3 = new Float32Array(16);
  var zero4 = new Float32Array(4);
  var one4 = new Float32Array([1,1,1,1]);

  // uniforms.
  var modelConst = {
  };
  var modelPer = {
    worldViewProjection: worldViewProjection
  };

  var then = (new Date()).getTime() * 0.001;
  function render() {
    tdl.webgl.requestAnimationFrame(render, canvas);
    var now = (new Date()).getTime() * 0.001;
    var elapsedTime = now - then;
    then = now;

    clock += elapsedTime;
    eyePosition[0] = Math.sin(clock * g_eyeSpeed) * g_eyeRadius;
    eyePosition[1] = g_eyeHeight;
    eyePosition[2] = Math.cos(clock * g_eyeSpeed) * g_eyeRadius;

    gl.colorMask(true, true, true, true);
    gl.depthMask(true);
    gl.clearColor(0,0,0,0);
    gl.clearDepth(1);
    gl.lineWidth(2);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT);

    gl.enable(gl.CULL_FACE);
    gl.enable(gl.DEPTH_TEST);
    //gl.enable(gl.BLEND);
    //gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

    fast.matrix4.perspective(
        projection,
        math.degToRad(g_fov),
        canvas.clientWidth / canvas.clientHeight,
        1,
        5000);
    fast.matrix4.lookAt(
        view,
        eyePosition,
        target,
        up);
    fast.matrix4.mul(viewProjection, view, projection);

    model.drawPrep(modelConst);
    fast.matrix4.translation(world, g_trans);
    fast.matrix4.mul(worldViewProjection, world, viewProjection);
    model.draw(modelPer);
  }
  render();
}

