
vertexShader =
  "attribute vec2 position;\n" +
  "void main() {\n" +
  "  gl_Position = vec4(position, 0., 1.);\n" +
  "}";

fragmentShader =
  'precision mediump float;\n' +
  "void main() {" +
  "  gl_FragColor = vec4(.2, .4, .6, 1.);" +
  "}";

vertexData = [
  -0.8, -0.8,
   0.8, -0.8,
   0.8,  0.8,
  -0.8,  0.8,
];
indexData = [ 0, 1, 2, 0, 2, 3 ];

gl = null
positionLocation = null
vbo = null

function init() {
  program = gl.createProgram();
  vs = gl.createShader(gl.VERTEX_SHADER);
  gl.shaderSource(vs, vertexShader);
  gl.compileShader(vs);
  log(vertexShader);

  fs = gl.createShader(gl.FRAGMENT_SHADER);
  gl.shaderSource(fs, fragmentShader);

  gl.attachShader(program, vs);
  gl.attachShader(program, fs);
  gl.linkProgram(program);
  log(gl.getProgramInfoLog(program));

  gl.useProgram(program);
  // log(gl.getError());

  positionLocation = gl.getAttribLocation(program, "position");
  log(gl.getError());

  vbo = gl.genBuffers(1)[0];
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
  gl.bufferData(gl.ARRAY_BUFFER, vertexData, gl.FLOAT, gl.STATIC_DRAW);
  // ELEMENT_ARRAY_BUFFER
  log(gl.getError());
}

function draw() {
  gl.clearColor(0.7, 0.7, 0.7, 0.7);
  gl.clear(gl.COLOR_BUFFER_BIT);
  log("positionLocation = " + positionLocation);
  // gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, vertexData);
  gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(positionLocation);

  gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, indexData);

  // gl.drawArrays(gl.TRIANGLES, 0, 3);



  gl.swapBuffers();
}

function main() {
  gl = $("#c")[0].getContext("moz-glweb20");
  init();
  draw();
}

$(document).ready(main)

