/**
 * Wrapped logging function.
 * @param {string} msg The message to log.
 */
var error = function(msg) {
  if (window.console) {
    if (window.console.error) {
      window.console.error(msg);
    }
    else if (window.console.log) {
      window.console.log(msg);
    }
  }
};

/**
 * Loads a shader.
 * @param {!WebGLContext} gl The WebGLContext to use.
 * @param {string} shaderSource The shader source.
 * @param {number} shaderType The type of shader.
 * @param {function(string): void) opt_errorCallback callback for errors.
 * @return {!WebGLShader} The created shader.
 */
var loadShader = function(gl, shaderSource, shaderType, opt_errorCallback) {
  var errFn = opt_errorCallback || error;
  // Create the shader object
  var shader = gl.createShader(shaderType);

  // Load the shader source
  gl.shaderSource(shader, shaderSource);

  // Compile the shader
  gl.compileShader(shader);

  // Check the compile status
  var compiled = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
  if (!compiled) {
    // Something went wrong during compilation; get the error
    lastError = gl.getShaderInfoLog(shader);
    errFn("*** Error compiling shader '" + shader + "':" + lastError);
    gl.deleteShader(shader);
    return null;
  }

  return shader;
}

/**
 * Creates a program, attaches shaders, binds attrib locations, links the
 * program and calls useProgram.
 * @param {!Array.<!WebGLShader>} shaders The shaders to attach
 * @param {!Array.<string>} opt_attribs The attribs names.
 * @param {!Array.<number>} opt_locations The locations for the attribs.
 */
var loadProgram = function(gl, shaders, opt_attribs, opt_locations) {
  var program = gl.createProgram();
  for (var ii = 0; ii < shaders.length; ++ii) {
    gl.attachShader(program, shaders[ii]);
  }
  if (opt_attribs) {
    for (var ii = 0; ii < opt_attribs.length; ++ii) {
      gl.bindAttribLocation(
          program,
          opt_locations ? opt_locations[ii] : ii,
          opt_attribs[ii]);
    }
  }
  gl.linkProgram(program);

  // Check the link status
  var linked = gl.getProgramParameter(program, gl.LINK_STATUS);
  if (!linked) {
      // something went wrong with the link
      lastError = gl.getProgramInfoLog (program);
      error("Error in program linking:" + lastError);

      gl.deleteProgram(program);
      return null;
  }
  return program;
};

/**
 * Loads a shader from a script tag.
 * @param {!WebGLContext} gl The WebGLContext to use.
 * @param {string} scriptId The id of the script tag.
 * @param {number} opt_shaderType The type of shader. If not passed in it will
 *     be derived from the type of the script tag.
 * @param {function(string): void) opt_errorCallback callback for errors.
 * @return {!WebGLShader} The created shader.
 */
var createShaderFromScript = function(
    gl, scriptId, opt_shaderType, opt_errorCallback) {
  var shaderSource = "";
  var shaderType;
  var shaderScript = document.getElementById(scriptId);
  if (!shaderScript) {
    throw("*** Error: unknown script element" + scriptId);
  }
  shaderSource = shaderScript.text;

  if (!opt_shaderType) {
    if (shaderScript.type == "x-shader/x-vertex") {
      shaderType = gl.VERTEX_SHADER;
    } else if (shaderScript.type == "x-shader/x-fragment") {
      shaderType = gl.FRAGMENT_SHADER;
    } else if (shaderType != gl.VERTEX_SHADER && shaderType != gl.FRAGMENT_SHADER) {
      throw("*** Error: unknown shader type");
      return null;
    }
  }

  return loadShader(
      gl, shaderSource, opt_shaderType ? opt_shaderType : shaderType,
      opt_errorCallback);
};

/**
 * Creates a program from 2 script tags.
 *
 * @param {!WebGLContext} gl The WebGLContext to use.
 * @param {!Array.<string>} shaderScriptIds Array of ids of the
 *        script tags for the shaders. The first is assumed to
 *        be the vertex shader, the second the fragment shader.
 * @param {function(string): void) opt_errorCallback callback for errors.
 * @return {!WebGLProgram} The created program.
 */
var createProgramFromScripts = (function(){
  var defaultShaderType = [
    "VERTEX_SHADER",
    "FRAGMENT_SHADER"
  ];

  return function(gl, shaderScriptIds, opt_errorCallback) {
    var shaders = [];
    for (var ii = 0; ii < shaderScriptIds.length; ++ii) {
      shaders.push(createShaderFromScript(gl, shaderScriptIds[ii], gl[defaultShaderType[ii]], opt_errorCallback));
    }
    return loadProgram(gl, shaders);
  }
}());

