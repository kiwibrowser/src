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
  var textures = {
      diffuseSampler: tdl.textures.loadTexture('webgl-logo-pot.png')
   };
  var program = tdl.programs.loadProgramFromScriptTags(
      'modelVertexShader',
      'modelFragmentShader');
  var arrays = tdl.primitives.createCube(1.2);
  tdl.primitives.reorient(arrays,
      [2, 0, 0, 0,
       0, 1, 0, 0,
       0, 0, 2, 0,
       0, 0, 0, 1]);
  return new tdl.models.Model(program, arrays, textures);
};

var g_eyeSpeed  = 1;
var g_eyeHeight = 1.5;
var g_eyeRadius = 3;
var g_trans     = [0,0.4,0];

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
  var viewInverse = new Float32Array(16);
  var viewProjectionInverse = new Float32Array(16);
  var eyePosition = new Float32Array(3);
  var target = new Float32Array(3);
  var up = new Float32Array([0,1,0]);
  var lightWorldPos = new Float32Array(3);
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
    viewInverse: viewInverse,
    lightWorldPos: lightWorldPos,
    specular: one4,
    shininess: 50,
    specularFactor: 0.2,
    lightColor: new Float32Array([1,1,1,1]),
  };
  var modelPer = {
    world: world,
    worldViewProjection: worldViewProjection,
    worldInverse: worldInverse,
    worldInverseTranspose: worldInverseTranspose};

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
    gl.clearColor(0,0,0.5,0);
    gl.clearDepth(1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT);

    gl.enable(gl.CULL_FACE);
    gl.enable(gl.DEPTH_TEST);
    //gl.enable(gl.BLEND);
    //gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

    fast.matrix4.perspective(
        projection,
        math.degToRad(60),
        canvas.clientWidth / canvas.clientHeight,
        1,
        5000);
    fast.matrix4.lookAt(
        view,
        eyePosition,
        target,
        up);
    fast.matrix4.mul(viewProjection, view, projection);
    fast.matrix4.inverse(viewInverse, view);
    fast.matrix4.inverse(viewProjectionInverse, viewProjection);

    fast.matrix4.getAxis(v3t0, viewInverse, 0); // x
    fast.matrix4.getAxis(v3t1, viewInverse, 1); // y;
    fast.matrix4.getAxis(v3t2, viewInverse, 2); // z;
    fast.mulScalarVector(v3t0, 0, v3t0);
    fast.mulScalarVector(v3t1, 1, v3t1);
    fast.mulScalarVector(v3t2, 1, v3t2);
    fast.addVector(lightWorldPos, eyePosition, v3t0);
    fast.addVector(lightWorldPos, lightWorldPos, v3t1);
    fast.addVector(lightWorldPos, lightWorldPos, v3t2);

    model.drawPrep(modelConst);
    fast.matrix4.translation(world, g_trans);
    fast.matrix4.mul(worldViewProjection, world, viewProjection);
    fast.matrix4.inverse(worldInverse, world);
    fast.matrix4.transpose(worldInverseTranspose, worldInverse);
    model.draw(modelPer);

    // Set the alpha to 255.
    gl.colorMask(false, false, false, true);
    gl.clearColor(0,0,0,1);
    gl.clear(gl.COLOR_BUFFER_BIT);
  }
  render();
}

