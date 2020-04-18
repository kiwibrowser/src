function initializeLogo(canvas) {
  try {
    startLogo(canvas);
  } catch (e) {
    console.log(e);
  }
}

function startLogo(canvas) {
  "use strict";
  var m4 = twgl.m4;
  var gl = canvas.getContext("webgl2");
  var programInfo = twgl.createProgramInfo(gl, ["vs", "fs"]);
  var devicePixelRatio = window.devicePixelRatio | 1;


  var bufferInfo = twgl.primitives.createCubeBufferInfo(gl, 2);

  var ctx = document.createElement("canvas").getContext("2d");
  ctx.canvas.width = 256;
  ctx.canvas.height = 256;
  ctx.font = "bold 200px sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.lineWidth = 2;
  ctx.strokeText("2", 128, 128);
  ctx.strokeRect(0, 0, 256, 256);

  var tex = twgl.createTexture(gl, {
    src: ctx.canvas,
    min: gl.LINEAR_MIPMAP_LINEAR,
    premultiplyAlpha: true,
  });

  var eye = [1, 4, -6];
  var target = [0, 0, 0];
  var up = [0, 1, 0];
  var camera = m4.identity();
  var view = m4.identity();
  var mat = m4.identity();
  var uniforms = {
    u_matrix: mat,
  };

  function render(time) {
    time *= 0.001;
    twgl.resizeCanvasToDisplaySize(gl.canvas, devicePixelRatio);
    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

    //gl.enable(gl.DEPTH_TEST);
    //gl.enable(gl.CULL_FACE);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA);

    m4.perspective(30 * Math.PI / 180, gl.canvas.clientWidth / gl.canvas.clientHeight, 0.5, 10, mat);

    m4.lookAt(eye, target, up, camera);
    m4.inverse(camera, view);
    m4.multiply(view, mat, mat);
    m4.rotateY(mat, time, mat);

    gl.useProgram(programInfo.program);
    twgl.setBuffersAndAttributes(gl, programInfo, bufferInfo);
    twgl.setUniforms(programInfo, uniforms);
    twgl.drawBufferInfo(gl, gl.TRIANGLES, bufferInfo);

    requestAnimationFrame(render);
  }
  requestAnimationFrame(render);
}
