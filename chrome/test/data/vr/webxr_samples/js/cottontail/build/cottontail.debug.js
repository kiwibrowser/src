/*!
 * Copyright 2018 The Immersive Web Community Group
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */
(function webpackUniversalModuleDefinition(root, factory) {
	if(typeof exports === 'object' && typeof module === 'object')
		module.exports = factory();
	else if(typeof define === 'function' && define.amd)
		define([], factory);
	else {
		var a = factory();
		for(var i in a) (typeof exports === 'object' ? exports : root)[i] = a[i];
	}
})(window, function() {
return /******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, {
/******/ 				configurable: false,
/******/ 				enumerable: true,
/******/ 				get: getter
/******/ 			});
/******/ 		}
/******/ 	};
/******/
/******/ 	// define __esModule on exports
/******/ 	__webpack_require__.r = function(exports) {
/******/ 		Object.defineProperty(exports, '__esModule', { value: true });
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = "./src/cottontail.js");
/******/ })
/************************************************************************/
/******/ ({

/***/ "./node_modules/gl-matrix/src/gl-matrix/common.js":
/*!********************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/common.js ***!
  \********************************************************/
/*! exports provided: EPSILON, ARRAY_TYPE, RANDOM, setMatrixArrayType, toRadian, equals */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "EPSILON", function() { return EPSILON; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ARRAY_TYPE", function() { return ARRAY_TYPE; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "RANDOM", function() { return RANDOM; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "setMatrixArrayType", function() { return setMatrixArrayType; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "toRadian", function() { return toRadian; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

/**
 * Common utilities
 * @module glMatrix
 */

// Configuration Constants
const EPSILON = 0.000001;
let ARRAY_TYPE = (typeof Float32Array !== 'undefined') ? Float32Array : Array;
const RANDOM = Math.random;

/**
 * Sets the type of array used when creating new vectors and matrices
 *
 * @param {Type} type Array type, such as Float32Array or Array
 */
function setMatrixArrayType(type) {
  ARRAY_TYPE = type;
}

const degree = Math.PI / 180;

/**
 * Convert Degree To Radian
 *
 * @param {Number} a Angle in Degrees
 */
function toRadian(a) {
  return a * degree;
}

/**
 * Tests whether or not the arguments have approximately the same value, within an absolute
 * or relative tolerance of glMatrix.EPSILON (an absolute tolerance is used for values less
 * than or equal to 1.0, and a relative tolerance is used for larger values)
 *
 * @param {Number} a The first number to test.
 * @param {Number} b The second number to test.
 * @returns {Boolean} True if the numbers are approximately equal, false otherwise.
 */
function equals(a, b) {
  return Math.abs(a - b) <= EPSILON*Math.max(1.0, Math.abs(a), Math.abs(b));
}


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/mat2.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/mat2.js ***!
  \******************************************************/
/*! exports provided: create, clone, copy, identity, fromValues, set, transpose, invert, adjoint, determinant, multiply, rotate, scale, fromRotation, fromScaling, str, frob, LDU, add, subtract, exactEquals, equals, multiplyScalar, multiplyScalarAndAdd, mul, sub */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "identity", function() { return identity; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transpose", function() { return transpose; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "invert", function() { return invert; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "adjoint", function() { return adjoint; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "determinant", function() { return determinant; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotate", function() { return rotate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotation", function() { return fromRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromScaling", function() { return fromScaling; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "frob", function() { return frob; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "LDU", function() { return LDU; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalar", function() { return multiplyScalar; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalarAndAdd", function() { return multiplyScalarAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * 2x2 Matrix
 * @module mat2
 */

/**
 * Creates a new identity mat2
 *
 * @returns {mat2} a new 2x2 matrix
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  return out;
}

/**
 * Creates a new mat2 initialized with values from an existing matrix
 *
 * @param {mat2} a matrix to clone
 * @returns {mat2} a new 2x2 matrix
 */
function clone(a) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  return out;
}

/**
 * Copy the values from one mat2 to another
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the source matrix
 * @returns {mat2} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  return out;
}

/**
 * Set a mat2 to the identity matrix
 *
 * @param {mat2} out the receiving matrix
 * @returns {mat2} out
 */
function identity(out) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  return out;
}

/**
 * Create a new mat2 with the given values
 *
 * @param {Number} m00 Component in column 0, row 0 position (index 0)
 * @param {Number} m01 Component in column 0, row 1 position (index 1)
 * @param {Number} m10 Component in column 1, row 0 position (index 2)
 * @param {Number} m11 Component in column 1, row 1 position (index 3)
 * @returns {mat2} out A new 2x2 matrix
 */
function fromValues(m00, m01, m10, m11) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = m00;
  out[1] = m01;
  out[2] = m10;
  out[3] = m11;
  return out;
}

/**
 * Set the components of a mat2 to the given values
 *
 * @param {mat2} out the receiving matrix
 * @param {Number} m00 Component in column 0, row 0 position (index 0)
 * @param {Number} m01 Component in column 0, row 1 position (index 1)
 * @param {Number} m10 Component in column 1, row 0 position (index 2)
 * @param {Number} m11 Component in column 1, row 1 position (index 3)
 * @returns {mat2} out
 */
function set(out, m00, m01, m10, m11) {
  out[0] = m00;
  out[1] = m01;
  out[2] = m10;
  out[3] = m11;
  return out;
}

/**
 * Transpose the values of a mat2
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the source matrix
 * @returns {mat2} out
 */
function transpose(out, a) {
  // If we are transposing ourselves we can skip a few steps but have to cache
  // some values
  if (out === a) {
    let a1 = a[1];
    out[1] = a[2];
    out[2] = a1;
  } else {
    out[0] = a[0];
    out[1] = a[2];
    out[2] = a[1];
    out[3] = a[3];
  }

  return out;
}

/**
 * Inverts a mat2
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the source matrix
 * @returns {mat2} out
 */
function invert(out, a) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];

  // Calculate the determinant
  let det = a0 * a3 - a2 * a1;

  if (!det) {
    return null;
  }
  det = 1.0 / det;

  out[0] =  a3 * det;
  out[1] = -a1 * det;
  out[2] = -a2 * det;
  out[3] =  a0 * det;

  return out;
}

/**
 * Calculates the adjugate of a mat2
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the source matrix
 * @returns {mat2} out
 */
function adjoint(out, a) {
  // Caching this value is nessecary if out == a
  let a0 = a[0];
  out[0] =  a[3];
  out[1] = -a[1];
  out[2] = -a[2];
  out[3] =  a0;

  return out;
}

/**
 * Calculates the determinant of a mat2
 *
 * @param {mat2} a the source matrix
 * @returns {Number} determinant of a
 */
function determinant(a) {
  return a[0] * a[3] - a[2] * a[1];
}

/**
 * Multiplies two mat2's
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the first operand
 * @param {mat2} b the second operand
 * @returns {mat2} out
 */
function multiply(out, a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
  let b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
  out[0] = a0 * b0 + a2 * b1;
  out[1] = a1 * b0 + a3 * b1;
  out[2] = a0 * b2 + a2 * b3;
  out[3] = a1 * b2 + a3 * b3;
  return out;
}

/**
 * Rotates a mat2 by the given angle
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat2} out
 */
function rotate(out, a, rad) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
  let s = Math.sin(rad);
  let c = Math.cos(rad);
  out[0] = a0 *  c + a2 * s;
  out[1] = a1 *  c + a3 * s;
  out[2] = a0 * -s + a2 * c;
  out[3] = a1 * -s + a3 * c;
  return out;
}

/**
 * Scales the mat2 by the dimensions in the given vec2
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the matrix to rotate
 * @param {vec2} v the vec2 to scale the matrix by
 * @returns {mat2} out
 **/
function scale(out, a, v) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
  let v0 = v[0], v1 = v[1];
  out[0] = a0 * v0;
  out[1] = a1 * v0;
  out[2] = a2 * v1;
  out[3] = a3 * v1;
  return out;
}

/**
 * Creates a matrix from a given angle
 * This is equivalent to (but much faster than):
 *
 *     mat2.identity(dest);
 *     mat2.rotate(dest, dest, rad);
 *
 * @param {mat2} out mat2 receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat2} out
 */
function fromRotation(out, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);
  out[0] = c;
  out[1] = s;
  out[2] = -s;
  out[3] = c;
  return out;
}

/**
 * Creates a matrix from a vector scaling
 * This is equivalent to (but much faster than):
 *
 *     mat2.identity(dest);
 *     mat2.scale(dest, dest, vec);
 *
 * @param {mat2} out mat2 receiving operation result
 * @param {vec2} v Scaling vector
 * @returns {mat2} out
 */
function fromScaling(out, v) {
  out[0] = v[0];
  out[1] = 0;
  out[2] = 0;
  out[3] = v[1];
  return out;
}

/**
 * Returns a string representation of a mat2
 *
 * @param {mat2} a matrix to represent as a string
 * @returns {String} string representation of the matrix
 */
function str(a) {
  return 'mat2(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' + a[3] + ')';
}

/**
 * Returns Frobenius norm of a mat2
 *
 * @param {mat2} a the matrix to calculate Frobenius norm of
 * @returns {Number} Frobenius norm
 */
function frob(a) {
  return(Math.sqrt(Math.pow(a[0], 2) + Math.pow(a[1], 2) + Math.pow(a[2], 2) + Math.pow(a[3], 2)))
}

/**
 * Returns L, D and U matrices (Lower triangular, Diagonal and Upper triangular) by factorizing the input matrix
 * @param {mat2} L the lower triangular matrix
 * @param {mat2} D the diagonal matrix
 * @param {mat2} U the upper triangular matrix
 * @param {mat2} a the input matrix to factorize
 */

function LDU(L, D, U, a) {
  L[2] = a[2]/a[0];
  U[0] = a[0];
  U[1] = a[1];
  U[3] = a[3] - L[2] * U[1];
  return [L, D, U];
}

/**
 * Adds two mat2's
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the first operand
 * @param {mat2} b the second operand
 * @returns {mat2} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  out[3] = a[3] + b[3];
  return out;
}

/**
 * Subtracts matrix b from matrix a
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the first operand
 * @param {mat2} b the second operand
 * @returns {mat2} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  out[3] = a[3] - b[3];
  return out;
}

/**
 * Returns whether or not the matrices have exactly the same elements in the same position (when compared with ===)
 *
 * @param {mat2} a The first matrix.
 * @param {mat2} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] && a[3] === b[3];
}

/**
 * Returns whether or not the matrices have approximately the same elements in the same position.
 *
 * @param {mat2} a The first matrix.
 * @param {mat2} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
  let b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
          Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a2), Math.abs(b2)) &&
          Math.abs(a3 - b3) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a3), Math.abs(b3)));
}

/**
 * Multiply each element of the matrix by a scalar.
 *
 * @param {mat2} out the receiving matrix
 * @param {mat2} a the matrix to scale
 * @param {Number} b amount to scale the matrix's elements by
 * @returns {mat2} out
 */
function multiplyScalar(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  out[3] = a[3] * b;
  return out;
}

/**
 * Adds two mat2's after multiplying each element of the second operand by a scalar value.
 *
 * @param {mat2} out the receiving vector
 * @param {mat2} a the first operand
 * @param {mat2} b the second operand
 * @param {Number} scale the amount to scale b's elements by before adding
 * @returns {mat2} out
 */
function multiplyScalarAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  out[2] = a[2] + (b[2] * scale);
  out[3] = a[3] + (b[3] * scale);
  return out;
}

/**
 * Alias for {@link mat2.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link mat2.subtract}
 * @function
 */
const sub = subtract;


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/mat2d.js":
/*!*******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/mat2d.js ***!
  \*******************************************************/
/*! exports provided: create, clone, copy, identity, fromValues, set, invert, determinant, multiply, rotate, scale, translate, fromRotation, fromScaling, fromTranslation, str, frob, add, subtract, multiplyScalar, multiplyScalarAndAdd, exactEquals, equals, mul, sub */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "identity", function() { return identity; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "invert", function() { return invert; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "determinant", function() { return determinant; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotate", function() { return rotate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "translate", function() { return translate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotation", function() { return fromRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromScaling", function() { return fromScaling; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromTranslation", function() { return fromTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "frob", function() { return frob; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalar", function() { return multiplyScalar; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalarAndAdd", function() { return multiplyScalarAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * 2x3 Matrix
 * @module mat2d
 *
 * @description
 * A mat2d contains six elements defined as:
 * <pre>
 * [a, c, tx,
 *  b, d, ty]
 * </pre>
 * This is a short form for the 3x3 matrix:
 * <pre>
 * [a, c, tx,
 *  b, d, ty,
 *  0, 0, 1]
 * </pre>
 * The last row is ignored so the array is shorter and operations are faster.
 */

/**
 * Creates a new identity mat2d
 *
 * @returns {mat2d} a new 2x3 matrix
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](6);
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  out[4] = 0;
  out[5] = 0;
  return out;
}

/**
 * Creates a new mat2d initialized with values from an existing matrix
 *
 * @param {mat2d} a matrix to clone
 * @returns {mat2d} a new 2x3 matrix
 */
function clone(a) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](6);
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  return out;
}

/**
 * Copy the values from one mat2d to another
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the source matrix
 * @returns {mat2d} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  return out;
}

/**
 * Set a mat2d to the identity matrix
 *
 * @param {mat2d} out the receiving matrix
 * @returns {mat2d} out
 */
function identity(out) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  out[4] = 0;
  out[5] = 0;
  return out;
}

/**
 * Create a new mat2d with the given values
 *
 * @param {Number} a Component A (index 0)
 * @param {Number} b Component B (index 1)
 * @param {Number} c Component C (index 2)
 * @param {Number} d Component D (index 3)
 * @param {Number} tx Component TX (index 4)
 * @param {Number} ty Component TY (index 5)
 * @returns {mat2d} A new mat2d
 */
function fromValues(a, b, c, d, tx, ty) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](6);
  out[0] = a;
  out[1] = b;
  out[2] = c;
  out[3] = d;
  out[4] = tx;
  out[5] = ty;
  return out;
}

/**
 * Set the components of a mat2d to the given values
 *
 * @param {mat2d} out the receiving matrix
 * @param {Number} a Component A (index 0)
 * @param {Number} b Component B (index 1)
 * @param {Number} c Component C (index 2)
 * @param {Number} d Component D (index 3)
 * @param {Number} tx Component TX (index 4)
 * @param {Number} ty Component TY (index 5)
 * @returns {mat2d} out
 */
function set(out, a, b, c, d, tx, ty) {
  out[0] = a;
  out[1] = b;
  out[2] = c;
  out[3] = d;
  out[4] = tx;
  out[5] = ty;
  return out;
}

/**
 * Inverts a mat2d
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the source matrix
 * @returns {mat2d} out
 */
function invert(out, a) {
  let aa = a[0], ab = a[1], ac = a[2], ad = a[3];
  let atx = a[4], aty = a[5];

  let det = aa * ad - ab * ac;
  if(!det){
    return null;
  }
  det = 1.0 / det;

  out[0] = ad * det;
  out[1] = -ab * det;
  out[2] = -ac * det;
  out[3] = aa * det;
  out[4] = (ac * aty - ad * atx) * det;
  out[5] = (ab * atx - aa * aty) * det;
  return out;
}

/**
 * Calculates the determinant of a mat2d
 *
 * @param {mat2d} a the source matrix
 * @returns {Number} determinant of a
 */
function determinant(a) {
  return a[0] * a[3] - a[1] * a[2];
}

/**
 * Multiplies two mat2d's
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the first operand
 * @param {mat2d} b the second operand
 * @returns {mat2d} out
 */
function multiply(out, a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
  let b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5];
  out[0] = a0 * b0 + a2 * b1;
  out[1] = a1 * b0 + a3 * b1;
  out[2] = a0 * b2 + a2 * b3;
  out[3] = a1 * b2 + a3 * b3;
  out[4] = a0 * b4 + a2 * b5 + a4;
  out[5] = a1 * b4 + a3 * b5 + a5;
  return out;
}

/**
 * Rotates a mat2d by the given angle
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat2d} out
 */
function rotate(out, a, rad) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
  let s = Math.sin(rad);
  let c = Math.cos(rad);
  out[0] = a0 *  c + a2 * s;
  out[1] = a1 *  c + a3 * s;
  out[2] = a0 * -s + a2 * c;
  out[3] = a1 * -s + a3 * c;
  out[4] = a4;
  out[5] = a5;
  return out;
}

/**
 * Scales the mat2d by the dimensions in the given vec2
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the matrix to translate
 * @param {vec2} v the vec2 to scale the matrix by
 * @returns {mat2d} out
 **/
function scale(out, a, v) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
  let v0 = v[0], v1 = v[1];
  out[0] = a0 * v0;
  out[1] = a1 * v0;
  out[2] = a2 * v1;
  out[3] = a3 * v1;
  out[4] = a4;
  out[5] = a5;
  return out;
}

/**
 * Translates the mat2d by the dimensions in the given vec2
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the matrix to translate
 * @param {vec2} v the vec2 to translate the matrix by
 * @returns {mat2d} out
 **/
function translate(out, a, v) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
  let v0 = v[0], v1 = v[1];
  out[0] = a0;
  out[1] = a1;
  out[2] = a2;
  out[3] = a3;
  out[4] = a0 * v0 + a2 * v1 + a4;
  out[5] = a1 * v0 + a3 * v1 + a5;
  return out;
}

/**
 * Creates a matrix from a given angle
 * This is equivalent to (but much faster than):
 *
 *     mat2d.identity(dest);
 *     mat2d.rotate(dest, dest, rad);
 *
 * @param {mat2d} out mat2d receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat2d} out
 */
function fromRotation(out, rad) {
  let s = Math.sin(rad), c = Math.cos(rad);
  out[0] = c;
  out[1] = s;
  out[2] = -s;
  out[3] = c;
  out[4] = 0;
  out[5] = 0;
  return out;
}

/**
 * Creates a matrix from a vector scaling
 * This is equivalent to (but much faster than):
 *
 *     mat2d.identity(dest);
 *     mat2d.scale(dest, dest, vec);
 *
 * @param {mat2d} out mat2d receiving operation result
 * @param {vec2} v Scaling vector
 * @returns {mat2d} out
 */
function fromScaling(out, v) {
  out[0] = v[0];
  out[1] = 0;
  out[2] = 0;
  out[3] = v[1];
  out[4] = 0;
  out[5] = 0;
  return out;
}

/**
 * Creates a matrix from a vector translation
 * This is equivalent to (but much faster than):
 *
 *     mat2d.identity(dest);
 *     mat2d.translate(dest, dest, vec);
 *
 * @param {mat2d} out mat2d receiving operation result
 * @param {vec2} v Translation vector
 * @returns {mat2d} out
 */
function fromTranslation(out, v) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  out[4] = v[0];
  out[5] = v[1];
  return out;
}

/**
 * Returns a string representation of a mat2d
 *
 * @param {mat2d} a matrix to represent as a string
 * @returns {String} string representation of the matrix
 */
function str(a) {
  return 'mat2d(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' +
          a[3] + ', ' + a[4] + ', ' + a[5] + ')';
}

/**
 * Returns Frobenius norm of a mat2d
 *
 * @param {mat2d} a the matrix to calculate Frobenius norm of
 * @returns {Number} Frobenius norm
 */
function frob(a) {
  return(Math.sqrt(Math.pow(a[0], 2) + Math.pow(a[1], 2) + Math.pow(a[2], 2) + Math.pow(a[3], 2) + Math.pow(a[4], 2) + Math.pow(a[5], 2) + 1))
}

/**
 * Adds two mat2d's
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the first operand
 * @param {mat2d} b the second operand
 * @returns {mat2d} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  out[3] = a[3] + b[3];
  out[4] = a[4] + b[4];
  out[5] = a[5] + b[5];
  return out;
}

/**
 * Subtracts matrix b from matrix a
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the first operand
 * @param {mat2d} b the second operand
 * @returns {mat2d} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  out[3] = a[3] - b[3];
  out[4] = a[4] - b[4];
  out[5] = a[5] - b[5];
  return out;
}

/**
 * Multiply each element of the matrix by a scalar.
 *
 * @param {mat2d} out the receiving matrix
 * @param {mat2d} a the matrix to scale
 * @param {Number} b amount to scale the matrix's elements by
 * @returns {mat2d} out
 */
function multiplyScalar(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  out[3] = a[3] * b;
  out[4] = a[4] * b;
  out[5] = a[5] * b;
  return out;
}

/**
 * Adds two mat2d's after multiplying each element of the second operand by a scalar value.
 *
 * @param {mat2d} out the receiving vector
 * @param {mat2d} a the first operand
 * @param {mat2d} b the second operand
 * @param {Number} scale the amount to scale b's elements by before adding
 * @returns {mat2d} out
 */
function multiplyScalarAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  out[2] = a[2] + (b[2] * scale);
  out[3] = a[3] + (b[3] * scale);
  out[4] = a[4] + (b[4] * scale);
  out[5] = a[5] + (b[5] * scale);
  return out;
}

/**
 * Returns whether or not the matrices have exactly the same elements in the same position (when compared with ===)
 *
 * @param {mat2d} a The first matrix.
 * @param {mat2d} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] && a[3] === b[3] && a[4] === b[4] && a[5] === b[5];
}

/**
 * Returns whether or not the matrices have approximately the same elements in the same position.
 *
 * @param {mat2d} a The first matrix.
 * @param {mat2d} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
  let b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
          Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a2), Math.abs(b2)) &&
          Math.abs(a3 - b3) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a3), Math.abs(b3)) &&
          Math.abs(a4 - b4) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a4), Math.abs(b4)) &&
          Math.abs(a5 - b5) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a5), Math.abs(b5)));
}

/**
 * Alias for {@link mat2d.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link mat2d.subtract}
 * @function
 */
const sub = subtract;


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/mat3.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/mat3.js ***!
  \******************************************************/
/*! exports provided: create, fromMat4, clone, copy, fromValues, set, identity, transpose, invert, adjoint, determinant, multiply, translate, rotate, scale, fromTranslation, fromRotation, fromScaling, fromMat2d, fromQuat, normalFromMat4, projection, str, frob, add, subtract, multiplyScalar, multiplyScalarAndAdd, exactEquals, equals, mul, sub */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromMat4", function() { return fromMat4; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "identity", function() { return identity; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transpose", function() { return transpose; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "invert", function() { return invert; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "adjoint", function() { return adjoint; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "determinant", function() { return determinant; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "translate", function() { return translate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotate", function() { return rotate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromTranslation", function() { return fromTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotation", function() { return fromRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromScaling", function() { return fromScaling; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromMat2d", function() { return fromMat2d; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromQuat", function() { return fromQuat; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "normalFromMat4", function() { return normalFromMat4; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "projection", function() { return projection; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "frob", function() { return frob; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalar", function() { return multiplyScalar; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalarAndAdd", function() { return multiplyScalarAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * 3x3 Matrix
 * @module mat3
 */

/**
 * Creates a new identity mat3
 *
 * @returns {mat3} a new 3x3 matrix
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](9);
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 1;
  out[5] = 0;
  out[6] = 0;
  out[7] = 0;
  out[8] = 1;
  return out;
}

/**
 * Copies the upper-left 3x3 values into the given mat3.
 *
 * @param {mat3} out the receiving 3x3 matrix
 * @param {mat4} a   the source 4x4 matrix
 * @returns {mat3} out
 */
function fromMat4(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[4];
  out[4] = a[5];
  out[5] = a[6];
  out[6] = a[8];
  out[7] = a[9];
  out[8] = a[10];
  return out;
}

/**
 * Creates a new mat3 initialized with values from an existing matrix
 *
 * @param {mat3} a matrix to clone
 * @returns {mat3} a new 3x3 matrix
 */
function clone(a) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](9);
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  out[6] = a[6];
  out[7] = a[7];
  out[8] = a[8];
  return out;
}

/**
 * Copy the values from one mat3 to another
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the source matrix
 * @returns {mat3} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  out[6] = a[6];
  out[7] = a[7];
  out[8] = a[8];
  return out;
}

/**
 * Create a new mat3 with the given values
 *
 * @param {Number} m00 Component in column 0, row 0 position (index 0)
 * @param {Number} m01 Component in column 0, row 1 position (index 1)
 * @param {Number} m02 Component in column 0, row 2 position (index 2)
 * @param {Number} m10 Component in column 1, row 0 position (index 3)
 * @param {Number} m11 Component in column 1, row 1 position (index 4)
 * @param {Number} m12 Component in column 1, row 2 position (index 5)
 * @param {Number} m20 Component in column 2, row 0 position (index 6)
 * @param {Number} m21 Component in column 2, row 1 position (index 7)
 * @param {Number} m22 Component in column 2, row 2 position (index 8)
 * @returns {mat3} A new mat3
 */
function fromValues(m00, m01, m02, m10, m11, m12, m20, m21, m22) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](9);
  out[0] = m00;
  out[1] = m01;
  out[2] = m02;
  out[3] = m10;
  out[4] = m11;
  out[5] = m12;
  out[6] = m20;
  out[7] = m21;
  out[8] = m22;
  return out;
}

/**
 * Set the components of a mat3 to the given values
 *
 * @param {mat3} out the receiving matrix
 * @param {Number} m00 Component in column 0, row 0 position (index 0)
 * @param {Number} m01 Component in column 0, row 1 position (index 1)
 * @param {Number} m02 Component in column 0, row 2 position (index 2)
 * @param {Number} m10 Component in column 1, row 0 position (index 3)
 * @param {Number} m11 Component in column 1, row 1 position (index 4)
 * @param {Number} m12 Component in column 1, row 2 position (index 5)
 * @param {Number} m20 Component in column 2, row 0 position (index 6)
 * @param {Number} m21 Component in column 2, row 1 position (index 7)
 * @param {Number} m22 Component in column 2, row 2 position (index 8)
 * @returns {mat3} out
 */
function set(out, m00, m01, m02, m10, m11, m12, m20, m21, m22) {
  out[0] = m00;
  out[1] = m01;
  out[2] = m02;
  out[3] = m10;
  out[4] = m11;
  out[5] = m12;
  out[6] = m20;
  out[7] = m21;
  out[8] = m22;
  return out;
}

/**
 * Set a mat3 to the identity matrix
 *
 * @param {mat3} out the receiving matrix
 * @returns {mat3} out
 */
function identity(out) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 1;
  out[5] = 0;
  out[6] = 0;
  out[7] = 0;
  out[8] = 1;
  return out;
}

/**
 * Transpose the values of a mat3
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the source matrix
 * @returns {mat3} out
 */
function transpose(out, a) {
  // If we are transposing ourselves we can skip a few steps but have to cache some values
  if (out === a) {
    let a01 = a[1], a02 = a[2], a12 = a[5];
    out[1] = a[3];
    out[2] = a[6];
    out[3] = a01;
    out[5] = a[7];
    out[6] = a02;
    out[7] = a12;
  } else {
    out[0] = a[0];
    out[1] = a[3];
    out[2] = a[6];
    out[3] = a[1];
    out[4] = a[4];
    out[5] = a[7];
    out[6] = a[2];
    out[7] = a[5];
    out[8] = a[8];
  }

  return out;
}

/**
 * Inverts a mat3
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the source matrix
 * @returns {mat3} out
 */
function invert(out, a) {
  let a00 = a[0], a01 = a[1], a02 = a[2];
  let a10 = a[3], a11 = a[4], a12 = a[5];
  let a20 = a[6], a21 = a[7], a22 = a[8];

  let b01 = a22 * a11 - a12 * a21;
  let b11 = -a22 * a10 + a12 * a20;
  let b21 = a21 * a10 - a11 * a20;

  // Calculate the determinant
  let det = a00 * b01 + a01 * b11 + a02 * b21;

  if (!det) {
    return null;
  }
  det = 1.0 / det;

  out[0] = b01 * det;
  out[1] = (-a22 * a01 + a02 * a21) * det;
  out[2] = (a12 * a01 - a02 * a11) * det;
  out[3] = b11 * det;
  out[4] = (a22 * a00 - a02 * a20) * det;
  out[5] = (-a12 * a00 + a02 * a10) * det;
  out[6] = b21 * det;
  out[7] = (-a21 * a00 + a01 * a20) * det;
  out[8] = (a11 * a00 - a01 * a10) * det;
  return out;
}

/**
 * Calculates the adjugate of a mat3
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the source matrix
 * @returns {mat3} out
 */
function adjoint(out, a) {
  let a00 = a[0], a01 = a[1], a02 = a[2];
  let a10 = a[3], a11 = a[4], a12 = a[5];
  let a20 = a[6], a21 = a[7], a22 = a[8];

  out[0] = (a11 * a22 - a12 * a21);
  out[1] = (a02 * a21 - a01 * a22);
  out[2] = (a01 * a12 - a02 * a11);
  out[3] = (a12 * a20 - a10 * a22);
  out[4] = (a00 * a22 - a02 * a20);
  out[5] = (a02 * a10 - a00 * a12);
  out[6] = (a10 * a21 - a11 * a20);
  out[7] = (a01 * a20 - a00 * a21);
  out[8] = (a00 * a11 - a01 * a10);
  return out;
}

/**
 * Calculates the determinant of a mat3
 *
 * @param {mat3} a the source matrix
 * @returns {Number} determinant of a
 */
function determinant(a) {
  let a00 = a[0], a01 = a[1], a02 = a[2];
  let a10 = a[3], a11 = a[4], a12 = a[5];
  let a20 = a[6], a21 = a[7], a22 = a[8];

  return a00 * (a22 * a11 - a12 * a21) + a01 * (-a22 * a10 + a12 * a20) + a02 * (a21 * a10 - a11 * a20);
}

/**
 * Multiplies two mat3's
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the first operand
 * @param {mat3} b the second operand
 * @returns {mat3} out
 */
function multiply(out, a, b) {
  let a00 = a[0], a01 = a[1], a02 = a[2];
  let a10 = a[3], a11 = a[4], a12 = a[5];
  let a20 = a[6], a21 = a[7], a22 = a[8];

  let b00 = b[0], b01 = b[1], b02 = b[2];
  let b10 = b[3], b11 = b[4], b12 = b[5];
  let b20 = b[6], b21 = b[7], b22 = b[8];

  out[0] = b00 * a00 + b01 * a10 + b02 * a20;
  out[1] = b00 * a01 + b01 * a11 + b02 * a21;
  out[2] = b00 * a02 + b01 * a12 + b02 * a22;

  out[3] = b10 * a00 + b11 * a10 + b12 * a20;
  out[4] = b10 * a01 + b11 * a11 + b12 * a21;
  out[5] = b10 * a02 + b11 * a12 + b12 * a22;

  out[6] = b20 * a00 + b21 * a10 + b22 * a20;
  out[7] = b20 * a01 + b21 * a11 + b22 * a21;
  out[8] = b20 * a02 + b21 * a12 + b22 * a22;
  return out;
}

/**
 * Translate a mat3 by the given vector
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the matrix to translate
 * @param {vec2} v vector to translate by
 * @returns {mat3} out
 */
function translate(out, a, v) {
  let a00 = a[0], a01 = a[1], a02 = a[2],
    a10 = a[3], a11 = a[4], a12 = a[5],
    a20 = a[6], a21 = a[7], a22 = a[8],
    x = v[0], y = v[1];

  out[0] = a00;
  out[1] = a01;
  out[2] = a02;

  out[3] = a10;
  out[4] = a11;
  out[5] = a12;

  out[6] = x * a00 + y * a10 + a20;
  out[7] = x * a01 + y * a11 + a21;
  out[8] = x * a02 + y * a12 + a22;
  return out;
}

/**
 * Rotates a mat3 by the given angle
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat3} out
 */
function rotate(out, a, rad) {
  let a00 = a[0], a01 = a[1], a02 = a[2],
    a10 = a[3], a11 = a[4], a12 = a[5],
    a20 = a[6], a21 = a[7], a22 = a[8],

    s = Math.sin(rad),
    c = Math.cos(rad);

  out[0] = c * a00 + s * a10;
  out[1] = c * a01 + s * a11;
  out[2] = c * a02 + s * a12;

  out[3] = c * a10 - s * a00;
  out[4] = c * a11 - s * a01;
  out[5] = c * a12 - s * a02;

  out[6] = a20;
  out[7] = a21;
  out[8] = a22;
  return out;
};

/**
 * Scales the mat3 by the dimensions in the given vec2
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the matrix to rotate
 * @param {vec2} v the vec2 to scale the matrix by
 * @returns {mat3} out
 **/
function scale(out, a, v) {
  let x = v[0], y = v[1];

  out[0] = x * a[0];
  out[1] = x * a[1];
  out[2] = x * a[2];

  out[3] = y * a[3];
  out[4] = y * a[4];
  out[5] = y * a[5];

  out[6] = a[6];
  out[7] = a[7];
  out[8] = a[8];
  return out;
}

/**
 * Creates a matrix from a vector translation
 * This is equivalent to (but much faster than):
 *
 *     mat3.identity(dest);
 *     mat3.translate(dest, dest, vec);
 *
 * @param {mat3} out mat3 receiving operation result
 * @param {vec2} v Translation vector
 * @returns {mat3} out
 */
function fromTranslation(out, v) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 1;
  out[5] = 0;
  out[6] = v[0];
  out[7] = v[1];
  out[8] = 1;
  return out;
}

/**
 * Creates a matrix from a given angle
 * This is equivalent to (but much faster than):
 *
 *     mat3.identity(dest);
 *     mat3.rotate(dest, dest, rad);
 *
 * @param {mat3} out mat3 receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat3} out
 */
function fromRotation(out, rad) {
  let s = Math.sin(rad), c = Math.cos(rad);

  out[0] = c;
  out[1] = s;
  out[2] = 0;

  out[3] = -s;
  out[4] = c;
  out[5] = 0;

  out[6] = 0;
  out[7] = 0;
  out[8] = 1;
  return out;
}

/**
 * Creates a matrix from a vector scaling
 * This is equivalent to (but much faster than):
 *
 *     mat3.identity(dest);
 *     mat3.scale(dest, dest, vec);
 *
 * @param {mat3} out mat3 receiving operation result
 * @param {vec2} v Scaling vector
 * @returns {mat3} out
 */
function fromScaling(out, v) {
  out[0] = v[0];
  out[1] = 0;
  out[2] = 0;

  out[3] = 0;
  out[4] = v[1];
  out[5] = 0;

  out[6] = 0;
  out[7] = 0;
  out[8] = 1;
  return out;
}

/**
 * Copies the values from a mat2d into a mat3
 *
 * @param {mat3} out the receiving matrix
 * @param {mat2d} a the matrix to copy
 * @returns {mat3} out
 **/
function fromMat2d(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = 0;

  out[3] = a[2];
  out[4] = a[3];
  out[5] = 0;

  out[6] = a[4];
  out[7] = a[5];
  out[8] = 1;
  return out;
}

/**
* Calculates a 3x3 matrix from the given quaternion
*
* @param {mat3} out mat3 receiving operation result
* @param {quat} q Quaternion to create matrix from
*
* @returns {mat3} out
*/
function fromQuat(out, q) {
  let x = q[0], y = q[1], z = q[2], w = q[3];
  let x2 = x + x;
  let y2 = y + y;
  let z2 = z + z;

  let xx = x * x2;
  let yx = y * x2;
  let yy = y * y2;
  let zx = z * x2;
  let zy = z * y2;
  let zz = z * z2;
  let wx = w * x2;
  let wy = w * y2;
  let wz = w * z2;

  out[0] = 1 - yy - zz;
  out[3] = yx - wz;
  out[6] = zx + wy;

  out[1] = yx + wz;
  out[4] = 1 - xx - zz;
  out[7] = zy - wx;

  out[2] = zx - wy;
  out[5] = zy + wx;
  out[8] = 1 - xx - yy;

  return out;
}

/**
* Calculates a 3x3 normal matrix (transpose inverse) from the 4x4 matrix
*
* @param {mat3} out mat3 receiving operation result
* @param {mat4} a Mat4 to derive the normal matrix from
*
* @returns {mat3} out
*/
function normalFromMat4(out, a) {
  let a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
  let a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
  let a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
  let a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

  let b00 = a00 * a11 - a01 * a10;
  let b01 = a00 * a12 - a02 * a10;
  let b02 = a00 * a13 - a03 * a10;
  let b03 = a01 * a12 - a02 * a11;
  let b04 = a01 * a13 - a03 * a11;
  let b05 = a02 * a13 - a03 * a12;
  let b06 = a20 * a31 - a21 * a30;
  let b07 = a20 * a32 - a22 * a30;
  let b08 = a20 * a33 - a23 * a30;
  let b09 = a21 * a32 - a22 * a31;
  let b10 = a21 * a33 - a23 * a31;
  let b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  let det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  if (!det) {
    return null;
  }
  det = 1.0 / det;

  out[0] = (a11 * b11 - a12 * b10 + a13 * b09) * det;
  out[1] = (a12 * b08 - a10 * b11 - a13 * b07) * det;
  out[2] = (a10 * b10 - a11 * b08 + a13 * b06) * det;

  out[3] = (a02 * b10 - a01 * b11 - a03 * b09) * det;
  out[4] = (a00 * b11 - a02 * b08 + a03 * b07) * det;
  out[5] = (a01 * b08 - a00 * b10 - a03 * b06) * det;

  out[6] = (a31 * b05 - a32 * b04 + a33 * b03) * det;
  out[7] = (a32 * b02 - a30 * b05 - a33 * b01) * det;
  out[8] = (a30 * b04 - a31 * b02 + a33 * b00) * det;

  return out;
}

/**
 * Generates a 2D projection matrix with the given bounds
 *
 * @param {mat3} out mat3 frustum matrix will be written into
 * @param {number} width Width of your gl context
 * @param {number} height Height of gl context
 * @returns {mat3} out
 */
function projection(out, width, height) {
    out[0] = 2 / width;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;
    out[4] = -2 / height;
    out[5] = 0;
    out[6] = -1;
    out[7] = 1;
    out[8] = 1;
    return out;
}

/**
 * Returns a string representation of a mat3
 *
 * @param {mat3} a matrix to represent as a string
 * @returns {String} string representation of the matrix
 */
function str(a) {
  return 'mat3(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' +
          a[3] + ', ' + a[4] + ', ' + a[5] + ', ' +
          a[6] + ', ' + a[7] + ', ' + a[8] + ')';
}

/**
 * Returns Frobenius norm of a mat3
 *
 * @param {mat3} a the matrix to calculate Frobenius norm of
 * @returns {Number} Frobenius norm
 */
function frob(a) {
  return(Math.sqrt(Math.pow(a[0], 2) + Math.pow(a[1], 2) + Math.pow(a[2], 2) + Math.pow(a[3], 2) + Math.pow(a[4], 2) + Math.pow(a[5], 2) + Math.pow(a[6], 2) + Math.pow(a[7], 2) + Math.pow(a[8], 2)))
}

/**
 * Adds two mat3's
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the first operand
 * @param {mat3} b the second operand
 * @returns {mat3} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  out[3] = a[3] + b[3];
  out[4] = a[4] + b[4];
  out[5] = a[5] + b[5];
  out[6] = a[6] + b[6];
  out[7] = a[7] + b[7];
  out[8] = a[8] + b[8];
  return out;
}

/**
 * Subtracts matrix b from matrix a
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the first operand
 * @param {mat3} b the second operand
 * @returns {mat3} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  out[3] = a[3] - b[3];
  out[4] = a[4] - b[4];
  out[5] = a[5] - b[5];
  out[6] = a[6] - b[6];
  out[7] = a[7] - b[7];
  out[8] = a[8] - b[8];
  return out;
}



/**
 * Multiply each element of the matrix by a scalar.
 *
 * @param {mat3} out the receiving matrix
 * @param {mat3} a the matrix to scale
 * @param {Number} b amount to scale the matrix's elements by
 * @returns {mat3} out
 */
function multiplyScalar(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  out[3] = a[3] * b;
  out[4] = a[4] * b;
  out[5] = a[5] * b;
  out[6] = a[6] * b;
  out[7] = a[7] * b;
  out[8] = a[8] * b;
  return out;
}

/**
 * Adds two mat3's after multiplying each element of the second operand by a scalar value.
 *
 * @param {mat3} out the receiving vector
 * @param {mat3} a the first operand
 * @param {mat3} b the second operand
 * @param {Number} scale the amount to scale b's elements by before adding
 * @returns {mat3} out
 */
function multiplyScalarAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  out[2] = a[2] + (b[2] * scale);
  out[3] = a[3] + (b[3] * scale);
  out[4] = a[4] + (b[4] * scale);
  out[5] = a[5] + (b[5] * scale);
  out[6] = a[6] + (b[6] * scale);
  out[7] = a[7] + (b[7] * scale);
  out[8] = a[8] + (b[8] * scale);
  return out;
}

/**
 * Returns whether or not the matrices have exactly the same elements in the same position (when compared with ===)
 *
 * @param {mat3} a The first matrix.
 * @param {mat3} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] &&
         a[3] === b[3] && a[4] === b[4] && a[5] === b[5] &&
         a[6] === b[6] && a[7] === b[7] && a[8] === b[8];
}

/**
 * Returns whether or not the matrices have approximately the same elements in the same position.
 *
 * @param {mat3} a The first matrix.
 * @param {mat3} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5], a6 = a[6], a7 = a[7], a8 = a[8];
  let b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5], b6 = b[6], b7 = b[7], b8 = b[8];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
          Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a2), Math.abs(b2)) &&
          Math.abs(a3 - b3) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a3), Math.abs(b3)) &&
          Math.abs(a4 - b4) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a4), Math.abs(b4)) &&
          Math.abs(a5 - b5) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a5), Math.abs(b5)) &&
          Math.abs(a6 - b6) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a6), Math.abs(b6)) &&
          Math.abs(a7 - b7) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a7), Math.abs(b7)) &&
          Math.abs(a8 - b8) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a8), Math.abs(b8)));
}

/**
 * Alias for {@link mat3.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link mat3.subtract}
 * @function
 */
const sub = subtract;


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/mat4.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/mat4.js ***!
  \******************************************************/
/*! exports provided: create, clone, copy, fromValues, set, identity, transpose, invert, adjoint, determinant, multiply, translate, scale, rotate, rotateX, rotateY, rotateZ, fromTranslation, fromScaling, fromRotation, fromXRotation, fromYRotation, fromZRotation, fromRotationTranslation, fromQuat2, getTranslation, getScaling, getRotation, fromRotationTranslationScale, fromRotationTranslationScaleOrigin, fromQuat, frustum, perspective, perspectiveFromFieldOfView, ortho, lookAt, targetTo, str, frob, add, subtract, multiplyScalar, multiplyScalarAndAdd, exactEquals, equals, mul, sub */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "identity", function() { return identity; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transpose", function() { return transpose; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "invert", function() { return invert; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "adjoint", function() { return adjoint; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "determinant", function() { return determinant; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "translate", function() { return translate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotate", function() { return rotate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateX", function() { return rotateX; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateY", function() { return rotateY; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateZ", function() { return rotateZ; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromTranslation", function() { return fromTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromScaling", function() { return fromScaling; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotation", function() { return fromRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromXRotation", function() { return fromXRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromYRotation", function() { return fromYRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromZRotation", function() { return fromZRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotationTranslation", function() { return fromRotationTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromQuat2", function() { return fromQuat2; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getTranslation", function() { return getTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getScaling", function() { return getScaling; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getRotation", function() { return getRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotationTranslationScale", function() { return fromRotationTranslationScale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotationTranslationScaleOrigin", function() { return fromRotationTranslationScaleOrigin; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromQuat", function() { return fromQuat; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "frustum", function() { return frustum; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "perspective", function() { return perspective; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "perspectiveFromFieldOfView", function() { return perspectiveFromFieldOfView; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ortho", function() { return ortho; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "lookAt", function() { return lookAt; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "targetTo", function() { return targetTo; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "frob", function() { return frob; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalar", function() { return multiplyScalar; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiplyScalarAndAdd", function() { return multiplyScalarAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * @class 4x4 Matrix<br>Format: column-major, when typed out it looks like row-major<br>The matrices are being post multiplied.
 * @name mat4
 */

/**
 * Creates a new identity mat4
 *
 * @returns {mat4} a new 4x4 matrix
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](16);
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = 1;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = 1;
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Creates a new mat4 initialized with values from an existing matrix
 *
 * @param {mat4} a matrix to clone
 * @returns {mat4} a new 4x4 matrix
 */
function clone(a) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](16);
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  out[6] = a[6];
  out[7] = a[7];
  out[8] = a[8];
  out[9] = a[9];
  out[10] = a[10];
  out[11] = a[11];
  out[12] = a[12];
  out[13] = a[13];
  out[14] = a[14];
  out[15] = a[15];
  return out;
}

/**
 * Copy the values from one mat4 to another
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the source matrix
 * @returns {mat4} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  out[6] = a[6];
  out[7] = a[7];
  out[8] = a[8];
  out[9] = a[9];
  out[10] = a[10];
  out[11] = a[11];
  out[12] = a[12];
  out[13] = a[13];
  out[14] = a[14];
  out[15] = a[15];
  return out;
}

/**
 * Create a new mat4 with the given values
 *
 * @param {Number} m00 Component in column 0, row 0 position (index 0)
 * @param {Number} m01 Component in column 0, row 1 position (index 1)
 * @param {Number} m02 Component in column 0, row 2 position (index 2)
 * @param {Number} m03 Component in column 0, row 3 position (index 3)
 * @param {Number} m10 Component in column 1, row 0 position (index 4)
 * @param {Number} m11 Component in column 1, row 1 position (index 5)
 * @param {Number} m12 Component in column 1, row 2 position (index 6)
 * @param {Number} m13 Component in column 1, row 3 position (index 7)
 * @param {Number} m20 Component in column 2, row 0 position (index 8)
 * @param {Number} m21 Component in column 2, row 1 position (index 9)
 * @param {Number} m22 Component in column 2, row 2 position (index 10)
 * @param {Number} m23 Component in column 2, row 3 position (index 11)
 * @param {Number} m30 Component in column 3, row 0 position (index 12)
 * @param {Number} m31 Component in column 3, row 1 position (index 13)
 * @param {Number} m32 Component in column 3, row 2 position (index 14)
 * @param {Number} m33 Component in column 3, row 3 position (index 15)
 * @returns {mat4} A new mat4
 */
function fromValues(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](16);
  out[0] = m00;
  out[1] = m01;
  out[2] = m02;
  out[3] = m03;
  out[4] = m10;
  out[5] = m11;
  out[6] = m12;
  out[7] = m13;
  out[8] = m20;
  out[9] = m21;
  out[10] = m22;
  out[11] = m23;
  out[12] = m30;
  out[13] = m31;
  out[14] = m32;
  out[15] = m33;
  return out;
}

/**
 * Set the components of a mat4 to the given values
 *
 * @param {mat4} out the receiving matrix
 * @param {Number} m00 Component in column 0, row 0 position (index 0)
 * @param {Number} m01 Component in column 0, row 1 position (index 1)
 * @param {Number} m02 Component in column 0, row 2 position (index 2)
 * @param {Number} m03 Component in column 0, row 3 position (index 3)
 * @param {Number} m10 Component in column 1, row 0 position (index 4)
 * @param {Number} m11 Component in column 1, row 1 position (index 5)
 * @param {Number} m12 Component in column 1, row 2 position (index 6)
 * @param {Number} m13 Component in column 1, row 3 position (index 7)
 * @param {Number} m20 Component in column 2, row 0 position (index 8)
 * @param {Number} m21 Component in column 2, row 1 position (index 9)
 * @param {Number} m22 Component in column 2, row 2 position (index 10)
 * @param {Number} m23 Component in column 2, row 3 position (index 11)
 * @param {Number} m30 Component in column 3, row 0 position (index 12)
 * @param {Number} m31 Component in column 3, row 1 position (index 13)
 * @param {Number} m32 Component in column 3, row 2 position (index 14)
 * @param {Number} m33 Component in column 3, row 3 position (index 15)
 * @returns {mat4} out
 */
function set(out, m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33) {
  out[0] = m00;
  out[1] = m01;
  out[2] = m02;
  out[3] = m03;
  out[4] = m10;
  out[5] = m11;
  out[6] = m12;
  out[7] = m13;
  out[8] = m20;
  out[9] = m21;
  out[10] = m22;
  out[11] = m23;
  out[12] = m30;
  out[13] = m31;
  out[14] = m32;
  out[15] = m33;
  return out;
}


/**
 * Set a mat4 to the identity matrix
 *
 * @param {mat4} out the receiving matrix
 * @returns {mat4} out
 */
function identity(out) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = 1;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = 1;
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Transpose the values of a mat4
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the source matrix
 * @returns {mat4} out
 */
function transpose(out, a) {
  // If we are transposing ourselves we can skip a few steps but have to cache some values
  if (out === a) {
    let a01 = a[1], a02 = a[2], a03 = a[3];
    let a12 = a[6], a13 = a[7];
    let a23 = a[11];

    out[1] = a[4];
    out[2] = a[8];
    out[3] = a[12];
    out[4] = a01;
    out[6] = a[9];
    out[7] = a[13];
    out[8] = a02;
    out[9] = a12;
    out[11] = a[14];
    out[12] = a03;
    out[13] = a13;
    out[14] = a23;
  } else {
    out[0] = a[0];
    out[1] = a[4];
    out[2] = a[8];
    out[3] = a[12];
    out[4] = a[1];
    out[5] = a[5];
    out[6] = a[9];
    out[7] = a[13];
    out[8] = a[2];
    out[9] = a[6];
    out[10] = a[10];
    out[11] = a[14];
    out[12] = a[3];
    out[13] = a[7];
    out[14] = a[11];
    out[15] = a[15];
  }

  return out;
}

/**
 * Inverts a mat4
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the source matrix
 * @returns {mat4} out
 */
function invert(out, a) {
  let a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
  let a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
  let a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
  let a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

  let b00 = a00 * a11 - a01 * a10;
  let b01 = a00 * a12 - a02 * a10;
  let b02 = a00 * a13 - a03 * a10;
  let b03 = a01 * a12 - a02 * a11;
  let b04 = a01 * a13 - a03 * a11;
  let b05 = a02 * a13 - a03 * a12;
  let b06 = a20 * a31 - a21 * a30;
  let b07 = a20 * a32 - a22 * a30;
  let b08 = a20 * a33 - a23 * a30;
  let b09 = a21 * a32 - a22 * a31;
  let b10 = a21 * a33 - a23 * a31;
  let b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  let det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  if (!det) {
    return null;
  }
  det = 1.0 / det;

  out[0] = (a11 * b11 - a12 * b10 + a13 * b09) * det;
  out[1] = (a02 * b10 - a01 * b11 - a03 * b09) * det;
  out[2] = (a31 * b05 - a32 * b04 + a33 * b03) * det;
  out[3] = (a22 * b04 - a21 * b05 - a23 * b03) * det;
  out[4] = (a12 * b08 - a10 * b11 - a13 * b07) * det;
  out[5] = (a00 * b11 - a02 * b08 + a03 * b07) * det;
  out[6] = (a32 * b02 - a30 * b05 - a33 * b01) * det;
  out[7] = (a20 * b05 - a22 * b02 + a23 * b01) * det;
  out[8] = (a10 * b10 - a11 * b08 + a13 * b06) * det;
  out[9] = (a01 * b08 - a00 * b10 - a03 * b06) * det;
  out[10] = (a30 * b04 - a31 * b02 + a33 * b00) * det;
  out[11] = (a21 * b02 - a20 * b04 - a23 * b00) * det;
  out[12] = (a11 * b07 - a10 * b09 - a12 * b06) * det;
  out[13] = (a00 * b09 - a01 * b07 + a02 * b06) * det;
  out[14] = (a31 * b01 - a30 * b03 - a32 * b00) * det;
  out[15] = (a20 * b03 - a21 * b01 + a22 * b00) * det;

  return out;
}

/**
 * Calculates the adjugate of a mat4
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the source matrix
 * @returns {mat4} out
 */
function adjoint(out, a) {
  let a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
  let a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
  let a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
  let a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

  out[0]  =  (a11 * (a22 * a33 - a23 * a32) - a21 * (a12 * a33 - a13 * a32) + a31 * (a12 * a23 - a13 * a22));
  out[1]  = -(a01 * (a22 * a33 - a23 * a32) - a21 * (a02 * a33 - a03 * a32) + a31 * (a02 * a23 - a03 * a22));
  out[2]  =  (a01 * (a12 * a33 - a13 * a32) - a11 * (a02 * a33 - a03 * a32) + a31 * (a02 * a13 - a03 * a12));
  out[3]  = -(a01 * (a12 * a23 - a13 * a22) - a11 * (a02 * a23 - a03 * a22) + a21 * (a02 * a13 - a03 * a12));
  out[4]  = -(a10 * (a22 * a33 - a23 * a32) - a20 * (a12 * a33 - a13 * a32) + a30 * (a12 * a23 - a13 * a22));
  out[5]  =  (a00 * (a22 * a33 - a23 * a32) - a20 * (a02 * a33 - a03 * a32) + a30 * (a02 * a23 - a03 * a22));
  out[6]  = -(a00 * (a12 * a33 - a13 * a32) - a10 * (a02 * a33 - a03 * a32) + a30 * (a02 * a13 - a03 * a12));
  out[7]  =  (a00 * (a12 * a23 - a13 * a22) - a10 * (a02 * a23 - a03 * a22) + a20 * (a02 * a13 - a03 * a12));
  out[8]  =  (a10 * (a21 * a33 - a23 * a31) - a20 * (a11 * a33 - a13 * a31) + a30 * (a11 * a23 - a13 * a21));
  out[9]  = -(a00 * (a21 * a33 - a23 * a31) - a20 * (a01 * a33 - a03 * a31) + a30 * (a01 * a23 - a03 * a21));
  out[10] =  (a00 * (a11 * a33 - a13 * a31) - a10 * (a01 * a33 - a03 * a31) + a30 * (a01 * a13 - a03 * a11));
  out[11] = -(a00 * (a11 * a23 - a13 * a21) - a10 * (a01 * a23 - a03 * a21) + a20 * (a01 * a13 - a03 * a11));
  out[12] = -(a10 * (a21 * a32 - a22 * a31) - a20 * (a11 * a32 - a12 * a31) + a30 * (a11 * a22 - a12 * a21));
  out[13] =  (a00 * (a21 * a32 - a22 * a31) - a20 * (a01 * a32 - a02 * a31) + a30 * (a01 * a22 - a02 * a21));
  out[14] = -(a00 * (a11 * a32 - a12 * a31) - a10 * (a01 * a32 - a02 * a31) + a30 * (a01 * a12 - a02 * a11));
  out[15] =  (a00 * (a11 * a22 - a12 * a21) - a10 * (a01 * a22 - a02 * a21) + a20 * (a01 * a12 - a02 * a11));
  return out;
}

/**
 * Calculates the determinant of a mat4
 *
 * @param {mat4} a the source matrix
 * @returns {Number} determinant of a
 */
function determinant(a) {
  let a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
  let a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
  let a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
  let a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

  let b00 = a00 * a11 - a01 * a10;
  let b01 = a00 * a12 - a02 * a10;
  let b02 = a00 * a13 - a03 * a10;
  let b03 = a01 * a12 - a02 * a11;
  let b04 = a01 * a13 - a03 * a11;
  let b05 = a02 * a13 - a03 * a12;
  let b06 = a20 * a31 - a21 * a30;
  let b07 = a20 * a32 - a22 * a30;
  let b08 = a20 * a33 - a23 * a30;
  let b09 = a21 * a32 - a22 * a31;
  let b10 = a21 * a33 - a23 * a31;
  let b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  return b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
}

/**
 * Multiplies two mat4s
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the first operand
 * @param {mat4} b the second operand
 * @returns {mat4} out
 */
function multiply(out, a, b) {
  let a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
  let a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
  let a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
  let a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

  // Cache only the current line of the second matrix
  let b0  = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
  out[0] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
  out[1] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
  out[2] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
  out[3] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

  b0 = b[4]; b1 = b[5]; b2 = b[6]; b3 = b[7];
  out[4] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
  out[5] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
  out[6] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
  out[7] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

  b0 = b[8]; b1 = b[9]; b2 = b[10]; b3 = b[11];
  out[8] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
  out[9] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
  out[10] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
  out[11] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

  b0 = b[12]; b1 = b[13]; b2 = b[14]; b3 = b[15];
  out[12] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
  out[13] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
  out[14] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
  out[15] = b0*a03 + b1*a13 + b2*a23 + b3*a33;
  return out;
}

/**
 * Translate a mat4 by the given vector
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to translate
 * @param {vec3} v vector to translate by
 * @returns {mat4} out
 */
function translate(out, a, v) {
  let x = v[0], y = v[1], z = v[2];
  let a00, a01, a02, a03;
  let a10, a11, a12, a13;
  let a20, a21, a22, a23;

  if (a === out) {
    out[12] = a[0] * x + a[4] * y + a[8] * z + a[12];
    out[13] = a[1] * x + a[5] * y + a[9] * z + a[13];
    out[14] = a[2] * x + a[6] * y + a[10] * z + a[14];
    out[15] = a[3] * x + a[7] * y + a[11] * z + a[15];
  } else {
    a00 = a[0]; a01 = a[1]; a02 = a[2]; a03 = a[3];
    a10 = a[4]; a11 = a[5]; a12 = a[6]; a13 = a[7];
    a20 = a[8]; a21 = a[9]; a22 = a[10]; a23 = a[11];

    out[0] = a00; out[1] = a01; out[2] = a02; out[3] = a03;
    out[4] = a10; out[5] = a11; out[6] = a12; out[7] = a13;
    out[8] = a20; out[9] = a21; out[10] = a22; out[11] = a23;

    out[12] = a00 * x + a10 * y + a20 * z + a[12];
    out[13] = a01 * x + a11 * y + a21 * z + a[13];
    out[14] = a02 * x + a12 * y + a22 * z + a[14];
    out[15] = a03 * x + a13 * y + a23 * z + a[15];
  }

  return out;
}

/**
 * Scales the mat4 by the dimensions in the given vec3 not using vectorization
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to scale
 * @param {vec3} v the vec3 to scale the matrix by
 * @returns {mat4} out
 **/
function scale(out, a, v) {
  let x = v[0], y = v[1], z = v[2];

  out[0] = a[0] * x;
  out[1] = a[1] * x;
  out[2] = a[2] * x;
  out[3] = a[3] * x;
  out[4] = a[4] * y;
  out[5] = a[5] * y;
  out[6] = a[6] * y;
  out[7] = a[7] * y;
  out[8] = a[8] * z;
  out[9] = a[9] * z;
  out[10] = a[10] * z;
  out[11] = a[11] * z;
  out[12] = a[12];
  out[13] = a[13];
  out[14] = a[14];
  out[15] = a[15];
  return out;
}

/**
 * Rotates a mat4 by the given angle around the given axis
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @param {vec3} axis the axis to rotate around
 * @returns {mat4} out
 */
function rotate(out, a, rad, axis) {
  let x = axis[0], y = axis[1], z = axis[2];
  let len = Math.sqrt(x * x + y * y + z * z);
  let s, c, t;
  let a00, a01, a02, a03;
  let a10, a11, a12, a13;
  let a20, a21, a22, a23;
  let b00, b01, b02;
  let b10, b11, b12;
  let b20, b21, b22;

  if (Math.abs(len) < _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]) { return null; }

  len = 1 / len;
  x *= len;
  y *= len;
  z *= len;

  s = Math.sin(rad);
  c = Math.cos(rad);
  t = 1 - c;

  a00 = a[0]; a01 = a[1]; a02 = a[2]; a03 = a[3];
  a10 = a[4]; a11 = a[5]; a12 = a[6]; a13 = a[7];
  a20 = a[8]; a21 = a[9]; a22 = a[10]; a23 = a[11];

  // Construct the elements of the rotation matrix
  b00 = x * x * t + c; b01 = y * x * t + z * s; b02 = z * x * t - y * s;
  b10 = x * y * t - z * s; b11 = y * y * t + c; b12 = z * y * t + x * s;
  b20 = x * z * t + y * s; b21 = y * z * t - x * s; b22 = z * z * t + c;

  // Perform rotation-specific matrix multiplication
  out[0] = a00 * b00 + a10 * b01 + a20 * b02;
  out[1] = a01 * b00 + a11 * b01 + a21 * b02;
  out[2] = a02 * b00 + a12 * b01 + a22 * b02;
  out[3] = a03 * b00 + a13 * b01 + a23 * b02;
  out[4] = a00 * b10 + a10 * b11 + a20 * b12;
  out[5] = a01 * b10 + a11 * b11 + a21 * b12;
  out[6] = a02 * b10 + a12 * b11 + a22 * b12;
  out[7] = a03 * b10 + a13 * b11 + a23 * b12;
  out[8] = a00 * b20 + a10 * b21 + a20 * b22;
  out[9] = a01 * b20 + a11 * b21 + a21 * b22;
  out[10] = a02 * b20 + a12 * b21 + a22 * b22;
  out[11] = a03 * b20 + a13 * b21 + a23 * b22;

  if (a !== out) { // If the source and destination differ, copy the unchanged last row
    out[12] = a[12];
    out[13] = a[13];
    out[14] = a[14];
    out[15] = a[15];
  }
  return out;
}

/**
 * Rotates a matrix by the given angle around the X axis
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat4} out
 */
function rotateX(out, a, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);
  let a10 = a[4];
  let a11 = a[5];
  let a12 = a[6];
  let a13 = a[7];
  let a20 = a[8];
  let a21 = a[9];
  let a22 = a[10];
  let a23 = a[11];

  if (a !== out) { // If the source and destination differ, copy the unchanged rows
    out[0]  = a[0];
    out[1]  = a[1];
    out[2]  = a[2];
    out[3]  = a[3];
    out[12] = a[12];
    out[13] = a[13];
    out[14] = a[14];
    out[15] = a[15];
  }

  // Perform axis-specific matrix multiplication
  out[4] = a10 * c + a20 * s;
  out[5] = a11 * c + a21 * s;
  out[6] = a12 * c + a22 * s;
  out[7] = a13 * c + a23 * s;
  out[8] = a20 * c - a10 * s;
  out[9] = a21 * c - a11 * s;
  out[10] = a22 * c - a12 * s;
  out[11] = a23 * c - a13 * s;
  return out;
}

/**
 * Rotates a matrix by the given angle around the Y axis
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat4} out
 */
function rotateY(out, a, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);
  let a00 = a[0];
  let a01 = a[1];
  let a02 = a[2];
  let a03 = a[3];
  let a20 = a[8];
  let a21 = a[9];
  let a22 = a[10];
  let a23 = a[11];

  if (a !== out) { // If the source and destination differ, copy the unchanged rows
    out[4]  = a[4];
    out[5]  = a[5];
    out[6]  = a[6];
    out[7]  = a[7];
    out[12] = a[12];
    out[13] = a[13];
    out[14] = a[14];
    out[15] = a[15];
  }

  // Perform axis-specific matrix multiplication
  out[0] = a00 * c - a20 * s;
  out[1] = a01 * c - a21 * s;
  out[2] = a02 * c - a22 * s;
  out[3] = a03 * c - a23 * s;
  out[8] = a00 * s + a20 * c;
  out[9] = a01 * s + a21 * c;
  out[10] = a02 * s + a22 * c;
  out[11] = a03 * s + a23 * c;
  return out;
}

/**
 * Rotates a matrix by the given angle around the Z axis
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to rotate
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat4} out
 */
function rotateZ(out, a, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);
  let a00 = a[0];
  let a01 = a[1];
  let a02 = a[2];
  let a03 = a[3];
  let a10 = a[4];
  let a11 = a[5];
  let a12 = a[6];
  let a13 = a[7];

  if (a !== out) { // If the source and destination differ, copy the unchanged last row
    out[8]  = a[8];
    out[9]  = a[9];
    out[10] = a[10];
    out[11] = a[11];
    out[12] = a[12];
    out[13] = a[13];
    out[14] = a[14];
    out[15] = a[15];
  }

  // Perform axis-specific matrix multiplication
  out[0] = a00 * c + a10 * s;
  out[1] = a01 * c + a11 * s;
  out[2] = a02 * c + a12 * s;
  out[3] = a03 * c + a13 * s;
  out[4] = a10 * c - a00 * s;
  out[5] = a11 * c - a01 * s;
  out[6] = a12 * c - a02 * s;
  out[7] = a13 * c - a03 * s;
  return out;
}

/**
 * Creates a matrix from a vector translation
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.translate(dest, dest, vec);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {vec3} v Translation vector
 * @returns {mat4} out
 */
function fromTranslation(out, v) {
  out[0] = 1;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = 1;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = 1;
  out[11] = 0;
  out[12] = v[0];
  out[13] = v[1];
  out[14] = v[2];
  out[15] = 1;
  return out;
}

/**
 * Creates a matrix from a vector scaling
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.scale(dest, dest, vec);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {vec3} v Scaling vector
 * @returns {mat4} out
 */
function fromScaling(out, v) {
  out[0] = v[0];
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = v[1];
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = v[2];
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Creates a matrix from a given angle around a given axis
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.rotate(dest, dest, rad, axis);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @param {vec3} axis the axis to rotate around
 * @returns {mat4} out
 */
function fromRotation(out, rad, axis) {
  let x = axis[0], y = axis[1], z = axis[2];
  let len = Math.sqrt(x * x + y * y + z * z);
  let s, c, t;

  if (Math.abs(len) < _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]) { return null; }

  len = 1 / len;
  x *= len;
  y *= len;
  z *= len;

  s = Math.sin(rad);
  c = Math.cos(rad);
  t = 1 - c;

  // Perform rotation-specific matrix multiplication
  out[0] = x * x * t + c;
  out[1] = y * x * t + z * s;
  out[2] = z * x * t - y * s;
  out[3] = 0;
  out[4] = x * y * t - z * s;
  out[5] = y * y * t + c;
  out[6] = z * y * t + x * s;
  out[7] = 0;
  out[8] = x * z * t + y * s;
  out[9] = y * z * t - x * s;
  out[10] = z * z * t + c;
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Creates a matrix from the given angle around the X axis
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.rotateX(dest, dest, rad);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat4} out
 */
function fromXRotation(out, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);

  // Perform axis-specific matrix multiplication
  out[0]  = 1;
  out[1]  = 0;
  out[2]  = 0;
  out[3]  = 0;
  out[4] = 0;
  out[5] = c;
  out[6] = s;
  out[7] = 0;
  out[8] = 0;
  out[9] = -s;
  out[10] = c;
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Creates a matrix from the given angle around the Y axis
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.rotateY(dest, dest, rad);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat4} out
 */
function fromYRotation(out, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);

  // Perform axis-specific matrix multiplication
  out[0]  = c;
  out[1]  = 0;
  out[2]  = -s;
  out[3]  = 0;
  out[4] = 0;
  out[5] = 1;
  out[6] = 0;
  out[7] = 0;
  out[8] = s;
  out[9] = 0;
  out[10] = c;
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Creates a matrix from the given angle around the Z axis
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.rotateZ(dest, dest, rad);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {Number} rad the angle to rotate the matrix by
 * @returns {mat4} out
 */
function fromZRotation(out, rad) {
  let s = Math.sin(rad);
  let c = Math.cos(rad);

  // Perform axis-specific matrix multiplication
  out[0]  = c;
  out[1]  = s;
  out[2]  = 0;
  out[3]  = 0;
  out[4] = -s;
  out[5] = c;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = 1;
  out[11] = 0;
  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;
  return out;
}

/**
 * Creates a matrix from a quaternion rotation and vector translation
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.translate(dest, vec);
 *     let quatMat = mat4.create();
 *     quat4.toMat4(quat, quatMat);
 *     mat4.multiply(dest, quatMat);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {quat4} q Rotation quaternion
 * @param {vec3} v Translation vector
 * @returns {mat4} out
 */
function fromRotationTranslation(out, q, v) {
  // Quaternion math
  let x = q[0], y = q[1], z = q[2], w = q[3];
  let x2 = x + x;
  let y2 = y + y;
  let z2 = z + z;

  let xx = x * x2;
  let xy = x * y2;
  let xz = x * z2;
  let yy = y * y2;
  let yz = y * z2;
  let zz = z * z2;
  let wx = w * x2;
  let wy = w * y2;
  let wz = w * z2;

  out[0] = 1 - (yy + zz);
  out[1] = xy + wz;
  out[2] = xz - wy;
  out[3] = 0;
  out[4] = xy - wz;
  out[5] = 1 - (xx + zz);
  out[6] = yz + wx;
  out[7] = 0;
  out[8] = xz + wy;
  out[9] = yz - wx;
  out[10] = 1 - (xx + yy);
  out[11] = 0;
  out[12] = v[0];
  out[13] = v[1];
  out[14] = v[2];
  out[15] = 1;

  return out;
}

/**
 * Creates a new mat4 from a dual quat.
 *
 * @param {mat4} out Matrix
 * @param {quat2} a Dual Quaternion
 * @returns {mat4} mat4 receiving operation result
 */
function fromQuat2(out, a) {
  let translation = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](3);
  let bx = -a[0], by = -a[1], bz = -a[2], bw = a[3],
  ax = a[4], ay = a[5], az = a[6], aw = a[7];
  
  let magnitude = bx * bx + by * by + bz * bz + bw * bw;
  //Only scale if it makes sense
  if (magnitude > 0) {
    translation[0] = (ax * bw + aw * bx + ay * bz - az * by) * 2 / magnitude;
    translation[1] = (ay * bw + aw * by + az * bx - ax * bz) * 2 / magnitude;
    translation[2] = (az * bw + aw * bz + ax * by - ay * bx) * 2 / magnitude;
  } else {
    translation[0] = (ax * bw + aw * bx + ay * bz - az * by) * 2;
    translation[1] = (ay * bw + aw * by + az * bx - ax * bz) * 2;
    translation[2] = (az * bw + aw * bz + ax * by - ay * bx) * 2;
  }
  fromRotationTranslation(out, a, translation);
  return out;
}

/**
 * Returns the translation vector component of a transformation
 *  matrix. If a matrix is built with fromRotationTranslation,
 *  the returned vector will be the same as the translation vector
 *  originally supplied.
 * @param  {vec3} out Vector to receive translation component
 * @param  {mat4} mat Matrix to be decomposed (input)
 * @return {vec3} out
 */
function getTranslation(out, mat) {
  out[0] = mat[12];
  out[1] = mat[13];
  out[2] = mat[14];

  return out;
}

/**
 * Returns the scaling factor component of a transformation
 *  matrix. If a matrix is built with fromRotationTranslationScale
 *  with a normalized Quaternion paramter, the returned vector will be
 *  the same as the scaling vector
 *  originally supplied.
 * @param  {vec3} out Vector to receive scaling factor component
 * @param  {mat4} mat Matrix to be decomposed (input)
 * @return {vec3} out
 */
function getScaling(out, mat) {
  let m11 = mat[0];
  let m12 = mat[1];
  let m13 = mat[2];
  let m21 = mat[4];
  let m22 = mat[5];
  let m23 = mat[6];
  let m31 = mat[8];
  let m32 = mat[9];
  let m33 = mat[10];

  out[0] = Math.sqrt(m11 * m11 + m12 * m12 + m13 * m13);
  out[1] = Math.sqrt(m21 * m21 + m22 * m22 + m23 * m23);
  out[2] = Math.sqrt(m31 * m31 + m32 * m32 + m33 * m33);

  return out;
}

/**
 * Returns a quaternion representing the rotational component
 *  of a transformation matrix. If a matrix is built with
 *  fromRotationTranslation, the returned quaternion will be the
 *  same as the quaternion originally supplied.
 * @param {quat} out Quaternion to receive the rotation component
 * @param {mat4} mat Matrix to be decomposed (input)
 * @return {quat} out
 */
function getRotation(out, mat) {
  // Algorithm taken from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
  let trace = mat[0] + mat[5] + mat[10];
  let S = 0;

  if (trace > 0) {
    S = Math.sqrt(trace + 1.0) * 2;
    out[3] = 0.25 * S;
    out[0] = (mat[6] - mat[9]) / S;
    out[1] = (mat[8] - mat[2]) / S;
    out[2] = (mat[1] - mat[4]) / S;
  } else if ((mat[0] > mat[5]) && (mat[0] > mat[10])) {
    S = Math.sqrt(1.0 + mat[0] - mat[5] - mat[10]) * 2;
    out[3] = (mat[6] - mat[9]) / S;
    out[0] = 0.25 * S;
    out[1] = (mat[1] + mat[4]) / S;
    out[2] = (mat[8] + mat[2]) / S;
  } else if (mat[5] > mat[10]) {
    S = Math.sqrt(1.0 + mat[5] - mat[0] - mat[10]) * 2;
    out[3] = (mat[8] - mat[2]) / S;
    out[0] = (mat[1] + mat[4]) / S;
    out[1] = 0.25 * S;
    out[2] = (mat[6] + mat[9]) / S;
  } else {
    S = Math.sqrt(1.0 + mat[10] - mat[0] - mat[5]) * 2;
    out[3] = (mat[1] - mat[4]) / S;
    out[0] = (mat[8] + mat[2]) / S;
    out[1] = (mat[6] + mat[9]) / S;
    out[2] = 0.25 * S;
  }

  return out;
}

/**
 * Creates a matrix from a quaternion rotation, vector translation and vector scale
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.translate(dest, vec);
 *     let quatMat = mat4.create();
 *     quat4.toMat4(quat, quatMat);
 *     mat4.multiply(dest, quatMat);
 *     mat4.scale(dest, scale)
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {quat4} q Rotation quaternion
 * @param {vec3} v Translation vector
 * @param {vec3} s Scaling vector
 * @returns {mat4} out
 */
function fromRotationTranslationScale(out, q, v, s) {
  // Quaternion math
  let x = q[0], y = q[1], z = q[2], w = q[3];
  let x2 = x + x;
  let y2 = y + y;
  let z2 = z + z;

  let xx = x * x2;
  let xy = x * y2;
  let xz = x * z2;
  let yy = y * y2;
  let yz = y * z2;
  let zz = z * z2;
  let wx = w * x2;
  let wy = w * y2;
  let wz = w * z2;
  let sx = s[0];
  let sy = s[1];
  let sz = s[2];

  out[0] = (1 - (yy + zz)) * sx;
  out[1] = (xy + wz) * sx;
  out[2] = (xz - wy) * sx;
  out[3] = 0;
  out[4] = (xy - wz) * sy;
  out[5] = (1 - (xx + zz)) * sy;
  out[6] = (yz + wx) * sy;
  out[7] = 0;
  out[8] = (xz + wy) * sz;
  out[9] = (yz - wx) * sz;
  out[10] = (1 - (xx + yy)) * sz;
  out[11] = 0;
  out[12] = v[0];
  out[13] = v[1];
  out[14] = v[2];
  out[15] = 1;

  return out;
}

/**
 * Creates a matrix from a quaternion rotation, vector translation and vector scale, rotating and scaling around the given origin
 * This is equivalent to (but much faster than):
 *
 *     mat4.identity(dest);
 *     mat4.translate(dest, vec);
 *     mat4.translate(dest, origin);
 *     let quatMat = mat4.create();
 *     quat4.toMat4(quat, quatMat);
 *     mat4.multiply(dest, quatMat);
 *     mat4.scale(dest, scale)
 *     mat4.translate(dest, negativeOrigin);
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {quat4} q Rotation quaternion
 * @param {vec3} v Translation vector
 * @param {vec3} s Scaling vector
 * @param {vec3} o The origin vector around which to scale and rotate
 * @returns {mat4} out
 */
function fromRotationTranslationScaleOrigin(out, q, v, s, o) {
  // Quaternion math
  let x = q[0], y = q[1], z = q[2], w = q[3];
  let x2 = x + x;
  let y2 = y + y;
  let z2 = z + z;

  let xx = x * x2;
  let xy = x * y2;
  let xz = x * z2;
  let yy = y * y2;
  let yz = y * z2;
  let zz = z * z2;
  let wx = w * x2;
  let wy = w * y2;
  let wz = w * z2;

  let sx = s[0];
  let sy = s[1];
  let sz = s[2];

  let ox = o[0];
  let oy = o[1];
  let oz = o[2];

  let out0 = (1 - (yy + zz)) * sx;
  let out1 = (xy + wz) * sx;
  let out2 = (xz - wy) * sx;
  let out4 = (xy - wz) * sy;
  let out5 = (1 - (xx + zz)) * sy;
  let out6 = (yz + wx) * sy;
  let out8 = (xz + wy) * sz;
  let out9 = (yz - wx) * sz;
  let out10 = (1 - (xx + yy)) * sz;

  out[0] = out0;
  out[1] = out1;
  out[2] = out2;
  out[3] = 0;
  out[4] = out4;
  out[5] = out5;
  out[6] = out6;
  out[7] = 0;
  out[8] = out8;
  out[9] = out9;
  out[10] = out10;
  out[11] = 0;
  out[12] = v[0] + ox - (out0 * ox + out4 * oy + out8 * oz);
  out[13] = v[1] + oy - (out1 * ox + out5 * oy + out9 * oz);
  out[14] = v[2] + oz - (out2 * ox + out6 * oy + out10 * oz);
  out[15] = 1;

  return out;
}

/**
 * Calculates a 4x4 matrix from the given quaternion
 *
 * @param {mat4} out mat4 receiving operation result
 * @param {quat} q Quaternion to create matrix from
 *
 * @returns {mat4} out
 */
function fromQuat(out, q) {
  let x = q[0], y = q[1], z = q[2], w = q[3];
  let x2 = x + x;
  let y2 = y + y;
  let z2 = z + z;

  let xx = x * x2;
  let yx = y * x2;
  let yy = y * y2;
  let zx = z * x2;
  let zy = z * y2;
  let zz = z * z2;
  let wx = w * x2;
  let wy = w * y2;
  let wz = w * z2;

  out[0] = 1 - yy - zz;
  out[1] = yx + wz;
  out[2] = zx - wy;
  out[3] = 0;

  out[4] = yx - wz;
  out[5] = 1 - xx - zz;
  out[6] = zy + wx;
  out[7] = 0;

  out[8] = zx + wy;
  out[9] = zy - wx;
  out[10] = 1 - xx - yy;
  out[11] = 0;

  out[12] = 0;
  out[13] = 0;
  out[14] = 0;
  out[15] = 1;

  return out;
}

/**
 * Generates a frustum matrix with the given bounds
 *
 * @param {mat4} out mat4 frustum matrix will be written into
 * @param {Number} left Left bound of the frustum
 * @param {Number} right Right bound of the frustum
 * @param {Number} bottom Bottom bound of the frustum
 * @param {Number} top Top bound of the frustum
 * @param {Number} near Near bound of the frustum
 * @param {Number} far Far bound of the frustum
 * @returns {mat4} out
 */
function frustum(out, left, right, bottom, top, near, far) {
  let rl = 1 / (right - left);
  let tb = 1 / (top - bottom);
  let nf = 1 / (near - far);
  out[0] = (near * 2) * rl;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = (near * 2) * tb;
  out[6] = 0;
  out[7] = 0;
  out[8] = (right + left) * rl;
  out[9] = (top + bottom) * tb;
  out[10] = (far + near) * nf;
  out[11] = -1;
  out[12] = 0;
  out[13] = 0;
  out[14] = (far * near * 2) * nf;
  out[15] = 0;
  return out;
}

/**
 * Generates a perspective projection matrix with the given bounds
 *
 * @param {mat4} out mat4 frustum matrix will be written into
 * @param {number} fovy Vertical field of view in radians
 * @param {number} aspect Aspect ratio. typically viewport width/height
 * @param {number} near Near bound of the frustum
 * @param {number} far Far bound of the frustum
 * @returns {mat4} out
 */
function perspective(out, fovy, aspect, near, far) {
  let f = 1.0 / Math.tan(fovy / 2);
  let nf = 1 / (near - far);
  out[0] = f / aspect;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = f;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = (far + near) * nf;
  out[11] = -1;
  out[12] = 0;
  out[13] = 0;
  out[14] = (2 * far * near) * nf;
  out[15] = 0;
  return out;
}

/**
 * Generates a perspective projection matrix with the given field of view.
 * This is primarily useful for generating projection matrices to be used
 * with the still experiemental WebVR API.
 *
 * @param {mat4} out mat4 frustum matrix will be written into
 * @param {Object} fov Object containing the following values: upDegrees, downDegrees, leftDegrees, rightDegrees
 * @param {number} near Near bound of the frustum
 * @param {number} far Far bound of the frustum
 * @returns {mat4} out
 */
function perspectiveFromFieldOfView(out, fov, near, far) {
  let upTan = Math.tan(fov.upDegrees * Math.PI/180.0);
  let downTan = Math.tan(fov.downDegrees * Math.PI/180.0);
  let leftTan = Math.tan(fov.leftDegrees * Math.PI/180.0);
  let rightTan = Math.tan(fov.rightDegrees * Math.PI/180.0);
  let xScale = 2.0 / (leftTan + rightTan);
  let yScale = 2.0 / (upTan + downTan);

  out[0] = xScale;
  out[1] = 0.0;
  out[2] = 0.0;
  out[3] = 0.0;
  out[4] = 0.0;
  out[5] = yScale;
  out[6] = 0.0;
  out[7] = 0.0;
  out[8] = -((leftTan - rightTan) * xScale * 0.5);
  out[9] = ((upTan - downTan) * yScale * 0.5);
  out[10] = far / (near - far);
  out[11] = -1.0;
  out[12] = 0.0;
  out[13] = 0.0;
  out[14] = (far * near) / (near - far);
  out[15] = 0.0;
  return out;
}

/**
 * Generates a orthogonal projection matrix with the given bounds
 *
 * @param {mat4} out mat4 frustum matrix will be written into
 * @param {number} left Left bound of the frustum
 * @param {number} right Right bound of the frustum
 * @param {number} bottom Bottom bound of the frustum
 * @param {number} top Top bound of the frustum
 * @param {number} near Near bound of the frustum
 * @param {number} far Far bound of the frustum
 * @returns {mat4} out
 */
function ortho(out, left, right, bottom, top, near, far) {
  let lr = 1 / (left - right);
  let bt = 1 / (bottom - top);
  let nf = 1 / (near - far);
  out[0] = -2 * lr;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = -2 * bt;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = 2 * nf;
  out[11] = 0;
  out[12] = (left + right) * lr;
  out[13] = (top + bottom) * bt;
  out[14] = (far + near) * nf;
  out[15] = 1;
  return out;
}

/**
 * Generates a look-at matrix with the given eye position, focal point, and up axis. 
 * If you want a matrix that actually makes an object look at another object, you should use targetTo instead.
 *
 * @param {mat4} out mat4 frustum matrix will be written into
 * @param {vec3} eye Position of the viewer
 * @param {vec3} center Point the viewer is looking at
 * @param {vec3} up vec3 pointing up
 * @returns {mat4} out
 */
function lookAt(out, eye, center, up) {
  let x0, x1, x2, y0, y1, y2, z0, z1, z2, len;
  let eyex = eye[0];
  let eyey = eye[1];
  let eyez = eye[2];
  let upx = up[0];
  let upy = up[1];
  let upz = up[2];
  let centerx = center[0];
  let centery = center[1];
  let centerz = center[2];

  if (Math.abs(eyex - centerx) < _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] &&
      Math.abs(eyey - centery) < _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] &&
      Math.abs(eyez - centerz) < _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]) {
    return identity(out);
  }

  z0 = eyex - centerx;
  z1 = eyey - centery;
  z2 = eyez - centerz;

  len = 1 / Math.sqrt(z0 * z0 + z1 * z1 + z2 * z2);
  z0 *= len;
  z1 *= len;
  z2 *= len;

  x0 = upy * z2 - upz * z1;
  x1 = upz * z0 - upx * z2;
  x2 = upx * z1 - upy * z0;
  len = Math.sqrt(x0 * x0 + x1 * x1 + x2 * x2);
  if (!len) {
    x0 = 0;
    x1 = 0;
    x2 = 0;
  } else {
    len = 1 / len;
    x0 *= len;
    x1 *= len;
    x2 *= len;
  }

  y0 = z1 * x2 - z2 * x1;
  y1 = z2 * x0 - z0 * x2;
  y2 = z0 * x1 - z1 * x0;

  len = Math.sqrt(y0 * y0 + y1 * y1 + y2 * y2);
  if (!len) {
    y0 = 0;
    y1 = 0;
    y2 = 0;
  } else {
    len = 1 / len;
    y0 *= len;
    y1 *= len;
    y2 *= len;
  }

  out[0] = x0;
  out[1] = y0;
  out[2] = z0;
  out[3] = 0;
  out[4] = x1;
  out[5] = y1;
  out[6] = z1;
  out[7] = 0;
  out[8] = x2;
  out[9] = y2;
  out[10] = z2;
  out[11] = 0;
  out[12] = -(x0 * eyex + x1 * eyey + x2 * eyez);
  out[13] = -(y0 * eyex + y1 * eyey + y2 * eyez);
  out[14] = -(z0 * eyex + z1 * eyey + z2 * eyez);
  out[15] = 1;

  return out;
}

/**
 * Generates a matrix that makes something look at something else.
 *
 * @param {mat4} out mat4 frustum matrix will be written into
 * @param {vec3} eye Position of the viewer
 * @param {vec3} center Point the viewer is looking at
 * @param {vec3} up vec3 pointing up
 * @returns {mat4} out
 */
function targetTo(out, eye, target, up) {
  let eyex = eye[0],
      eyey = eye[1],
      eyez = eye[2],
      upx = up[0],
      upy = up[1],
      upz = up[2];

  let z0 = eyex - target[0],
      z1 = eyey - target[1],
      z2 = eyez - target[2];

  let len = z0*z0 + z1*z1 + z2*z2;
  if (len > 0) {
    len = 1 / Math.sqrt(len);
    z0 *= len;
    z1 *= len;
    z2 *= len;
  }

  let x0 = upy * z2 - upz * z1,
      x1 = upz * z0 - upx * z2,
      x2 = upx * z1 - upy * z0;

  len = x0*x0 + x1*x1 + x2*x2;
  if (len > 0) {
    len = 1 / Math.sqrt(len);
    x0 *= len;
    x1 *= len;
    x2 *= len;
  }

  out[0] = x0;
  out[1] = x1;
  out[2] = x2;
  out[3] = 0;
  out[4] = z1 * x2 - z2 * x1;
  out[5] = z2 * x0 - z0 * x2;
  out[6] = z0 * x1 - z1 * x0;
  out[7] = 0;
  out[8] = z0;
  out[9] = z1;
  out[10] = z2;
  out[11] = 0;
  out[12] = eyex;
  out[13] = eyey;
  out[14] = eyez;
  out[15] = 1;
  return out;
};

/**
 * Returns a string representation of a mat4
 *
 * @param {mat4} a matrix to represent as a string
 * @returns {String} string representation of the matrix
 */
function str(a) {
  return 'mat4(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' + a[3] + ', ' +
          a[4] + ', ' + a[5] + ', ' + a[6] + ', ' + a[7] + ', ' +
          a[8] + ', ' + a[9] + ', ' + a[10] + ', ' + a[11] + ', ' +
          a[12] + ', ' + a[13] + ', ' + a[14] + ', ' + a[15] + ')';
}

/**
 * Returns Frobenius norm of a mat4
 *
 * @param {mat4} a the matrix to calculate Frobenius norm of
 * @returns {Number} Frobenius norm
 */
function frob(a) {
  return(Math.sqrt(Math.pow(a[0], 2) + Math.pow(a[1], 2) + Math.pow(a[2], 2) + Math.pow(a[3], 2) + Math.pow(a[4], 2) + Math.pow(a[5], 2) + Math.pow(a[6], 2) + Math.pow(a[7], 2) + Math.pow(a[8], 2) + Math.pow(a[9], 2) + Math.pow(a[10], 2) + Math.pow(a[11], 2) + Math.pow(a[12], 2) + Math.pow(a[13], 2) + Math.pow(a[14], 2) + Math.pow(a[15], 2) ))
}

/**
 * Adds two mat4's
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the first operand
 * @param {mat4} b the second operand
 * @returns {mat4} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  out[3] = a[3] + b[3];
  out[4] = a[4] + b[4];
  out[5] = a[5] + b[5];
  out[6] = a[6] + b[6];
  out[7] = a[7] + b[7];
  out[8] = a[8] + b[8];
  out[9] = a[9] + b[9];
  out[10] = a[10] + b[10];
  out[11] = a[11] + b[11];
  out[12] = a[12] + b[12];
  out[13] = a[13] + b[13];
  out[14] = a[14] + b[14];
  out[15] = a[15] + b[15];
  return out;
}

/**
 * Subtracts matrix b from matrix a
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the first operand
 * @param {mat4} b the second operand
 * @returns {mat4} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  out[3] = a[3] - b[3];
  out[4] = a[4] - b[4];
  out[5] = a[5] - b[5];
  out[6] = a[6] - b[6];
  out[7] = a[7] - b[7];
  out[8] = a[8] - b[8];
  out[9] = a[9] - b[9];
  out[10] = a[10] - b[10];
  out[11] = a[11] - b[11];
  out[12] = a[12] - b[12];
  out[13] = a[13] - b[13];
  out[14] = a[14] - b[14];
  out[15] = a[15] - b[15];
  return out;
}

/**
 * Multiply each element of the matrix by a scalar.
 *
 * @param {mat4} out the receiving matrix
 * @param {mat4} a the matrix to scale
 * @param {Number} b amount to scale the matrix's elements by
 * @returns {mat4} out
 */
function multiplyScalar(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  out[3] = a[3] * b;
  out[4] = a[4] * b;
  out[5] = a[5] * b;
  out[6] = a[6] * b;
  out[7] = a[7] * b;
  out[8] = a[8] * b;
  out[9] = a[9] * b;
  out[10] = a[10] * b;
  out[11] = a[11] * b;
  out[12] = a[12] * b;
  out[13] = a[13] * b;
  out[14] = a[14] * b;
  out[15] = a[15] * b;
  return out;
}

/**
 * Adds two mat4's after multiplying each element of the second operand by a scalar value.
 *
 * @param {mat4} out the receiving vector
 * @param {mat4} a the first operand
 * @param {mat4} b the second operand
 * @param {Number} scale the amount to scale b's elements by before adding
 * @returns {mat4} out
 */
function multiplyScalarAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  out[2] = a[2] + (b[2] * scale);
  out[3] = a[3] + (b[3] * scale);
  out[4] = a[4] + (b[4] * scale);
  out[5] = a[5] + (b[5] * scale);
  out[6] = a[6] + (b[6] * scale);
  out[7] = a[7] + (b[7] * scale);
  out[8] = a[8] + (b[8] * scale);
  out[9] = a[9] + (b[9] * scale);
  out[10] = a[10] + (b[10] * scale);
  out[11] = a[11] + (b[11] * scale);
  out[12] = a[12] + (b[12] * scale);
  out[13] = a[13] + (b[13] * scale);
  out[14] = a[14] + (b[14] * scale);
  out[15] = a[15] + (b[15] * scale);
  return out;
}

/**
 * Returns whether or not the matrices have exactly the same elements in the same position (when compared with ===)
 *
 * @param {mat4} a The first matrix.
 * @param {mat4} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] && a[3] === b[3] &&
         a[4] === b[4] && a[5] === b[5] && a[6] === b[6] && a[7] === b[7] &&
         a[8] === b[8] && a[9] === b[9] && a[10] === b[10] && a[11] === b[11] &&
         a[12] === b[12] && a[13] === b[13] && a[14] === b[14] && a[15] === b[15];
}

/**
 * Returns whether or not the matrices have approximately the same elements in the same position.
 *
 * @param {mat4} a The first matrix.
 * @param {mat4} b The second matrix.
 * @returns {Boolean} True if the matrices are equal, false otherwise.
 */
function equals(a, b) {
  let a0  = a[0],  a1  = a[1],  a2  = a[2],  a3  = a[3];
  let a4  = a[4],  a5  = a[5],  a6  = a[6],  a7  = a[7];
  let a8  = a[8],  a9  = a[9],  a10 = a[10], a11 = a[11];
  let a12 = a[12], a13 = a[13], a14 = a[14], a15 = a[15];

  let b0  = b[0],  b1  = b[1],  b2  = b[2],  b3  = b[3];
  let b4  = b[4],  b5  = b[5],  b6  = b[6],  b7  = b[7];
  let b8  = b[8],  b9  = b[9],  b10 = b[10], b11 = b[11];
  let b12 = b[12], b13 = b[13], b14 = b[14], b15 = b[15];

  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
          Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a2), Math.abs(b2)) &&
          Math.abs(a3 - b3) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a3), Math.abs(b3)) &&
          Math.abs(a4 - b4) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a4), Math.abs(b4)) &&
          Math.abs(a5 - b5) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a5), Math.abs(b5)) &&
          Math.abs(a6 - b6) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a6), Math.abs(b6)) &&
          Math.abs(a7 - b7) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a7), Math.abs(b7)) &&
          Math.abs(a8 - b8) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a8), Math.abs(b8)) &&
          Math.abs(a9 - b9) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a9), Math.abs(b9)) &&
          Math.abs(a10 - b10) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a10), Math.abs(b10)) &&
          Math.abs(a11 - b11) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a11), Math.abs(b11)) &&
          Math.abs(a12 - b12) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a12), Math.abs(b12)) &&
          Math.abs(a13 - b13) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a13), Math.abs(b13)) &&
          Math.abs(a14 - b14) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a14), Math.abs(b14)) &&
          Math.abs(a15 - b15) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a15), Math.abs(b15)));
}

/**
 * Alias for {@link mat4.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link mat4.subtract}
 * @function
 */
const sub = subtract;


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/quat.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/quat.js ***!
  \******************************************************/
/*! exports provided: create, identity, setAxisAngle, getAxisAngle, multiply, rotateX, rotateY, rotateZ, calculateW, slerp, invert, conjugate, fromMat3, fromEuler, str, clone, fromValues, copy, set, add, mul, scale, dot, lerp, length, len, squaredLength, sqrLen, normalize, exactEquals, equals, rotationTo, sqlerp, setAxes */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "identity", function() { return identity; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "setAxisAngle", function() { return setAxisAngle; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getAxisAngle", function() { return getAxisAngle; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateX", function() { return rotateX; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateY", function() { return rotateY; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateZ", function() { return rotateZ; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "calculateW", function() { return calculateW; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "slerp", function() { return slerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "invert", function() { return invert; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "conjugate", function() { return conjugate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromMat3", function() { return fromMat3; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromEuler", function() { return fromEuler; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dot", function() { return dot; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "lerp", function() { return lerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "length", function() { return length; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "len", function() { return len; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredLength", function() { return squaredLength; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrLen", function() { return sqrLen; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "normalize", function() { return normalize; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotationTo", function() { return rotationTo; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqlerp", function() { return sqlerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "setAxes", function() { return setAxes; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* harmony import */ var _mat3_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./mat3.js */ "./node_modules/gl-matrix/src/gl-matrix/mat3.js");
/* harmony import */ var _vec3_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./vec3.js */ "./node_modules/gl-matrix/src/gl-matrix/vec3.js");
/* harmony import */ var _vec4_js__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./vec4.js */ "./node_modules/gl-matrix/src/gl-matrix/vec4.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */






/**
 * Quaternion
 * @module quat
 */

/**
 * Creates a new identity quat
 *
 * @returns {quat} a new quaternion
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  return out;
}

/**
 * Set a quat to the identity quaternion
 *
 * @param {quat} out the receiving quaternion
 * @returns {quat} out
 */
function identity(out) {
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  return out;
}

/**
 * Sets a quat from the given angle and rotation axis,
 * then returns it.
 *
 * @param {quat} out the receiving quaternion
 * @param {vec3} axis the axis around which to rotate
 * @param {Number} rad the angle in radians
 * @returns {quat} out
 **/
function setAxisAngle(out, axis, rad) {
  rad = rad * 0.5;
  let s = Math.sin(rad);
  out[0] = s * axis[0];
  out[1] = s * axis[1];
  out[2] = s * axis[2];
  out[3] = Math.cos(rad);
  return out;
}

/**
 * Gets the rotation axis and angle for a given
 *  quaternion. If a quaternion is created with
 *  setAxisAngle, this method will return the same
 *  values as providied in the original parameter list
 *  OR functionally equivalent values.
 * Example: The quaternion formed by axis [0, 0, 1] and
 *  angle -90 is the same as the quaternion formed by
 *  [0, 0, 1] and 270. This method favors the latter.
 * @param  {vec3} out_axis  Vector receiving the axis of rotation
 * @param  {quat} q     Quaternion to be decomposed
 * @return {Number}     Angle, in radians, of the rotation
 */
function getAxisAngle(out_axis, q) {
  let rad = Math.acos(q[3]) * 2.0;
  let s = Math.sin(rad / 2.0);
  if (s != 0.0) {
    out_axis[0] = q[0] / s;
    out_axis[1] = q[1] / s;
    out_axis[2] = q[2] / s;
  } else {
    // If s is zero, return any axis (no rotation - axis does not matter)
    out_axis[0] = 1;
    out_axis[1] = 0;
    out_axis[2] = 0;
  }
  return rad;
}

/**
 * Multiplies two quat's
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a the first operand
 * @param {quat} b the second operand
 * @returns {quat} out
 */
function multiply(out, a, b) {
  let ax = a[0], ay = a[1], az = a[2], aw = a[3];
  let bx = b[0], by = b[1], bz = b[2], bw = b[3];

  out[0] = ax * bw + aw * bx + ay * bz - az * by;
  out[1] = ay * bw + aw * by + az * bx - ax * bz;
  out[2] = az * bw + aw * bz + ax * by - ay * bx;
  out[3] = aw * bw - ax * bx - ay * by - az * bz;
  return out;
}

/**
 * Rotates a quaternion by the given angle about the X axis
 *
 * @param {quat} out quat receiving operation result
 * @param {quat} a quat to rotate
 * @param {number} rad angle (in radians) to rotate
 * @returns {quat} out
 */
function rotateX(out, a, rad) {
  rad *= 0.5;

  let ax = a[0], ay = a[1], az = a[2], aw = a[3];
  let bx = Math.sin(rad), bw = Math.cos(rad);

  out[0] = ax * bw + aw * bx;
  out[1] = ay * bw + az * bx;
  out[2] = az * bw - ay * bx;
  out[3] = aw * bw - ax * bx;
  return out;
}

/**
 * Rotates a quaternion by the given angle about the Y axis
 *
 * @param {quat} out quat receiving operation result
 * @param {quat} a quat to rotate
 * @param {number} rad angle (in radians) to rotate
 * @returns {quat} out
 */
function rotateY(out, a, rad) {
  rad *= 0.5;

  let ax = a[0], ay = a[1], az = a[2], aw = a[3];
  let by = Math.sin(rad), bw = Math.cos(rad);

  out[0] = ax * bw - az * by;
  out[1] = ay * bw + aw * by;
  out[2] = az * bw + ax * by;
  out[3] = aw * bw - ay * by;
  return out;
}

/**
 * Rotates a quaternion by the given angle about the Z axis
 *
 * @param {quat} out quat receiving operation result
 * @param {quat} a quat to rotate
 * @param {number} rad angle (in radians) to rotate
 * @returns {quat} out
 */
function rotateZ(out, a, rad) {
  rad *= 0.5;

  let ax = a[0], ay = a[1], az = a[2], aw = a[3];
  let bz = Math.sin(rad), bw = Math.cos(rad);

  out[0] = ax * bw + ay * bz;
  out[1] = ay * bw - ax * bz;
  out[2] = az * bw + aw * bz;
  out[3] = aw * bw - az * bz;
  return out;
}

/**
 * Calculates the W component of a quat from the X, Y, and Z components.
 * Assumes that quaternion is 1 unit in length.
 * Any existing W component will be ignored.
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a quat to calculate W component of
 * @returns {quat} out
 */
function calculateW(out, a) {
  let x = a[0], y = a[1], z = a[2];

  out[0] = x;
  out[1] = y;
  out[2] = z;
  out[3] = Math.sqrt(Math.abs(1.0 - x * x - y * y - z * z));
  return out;
}

/**
 * Performs a spherical linear interpolation between two quat
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a the first operand
 * @param {quat} b the second operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {quat} out
 */
function slerp(out, a, b, t) {
  // benchmarks:
  //    http://jsperf.com/quaternion-slerp-implementations
  let ax = a[0], ay = a[1], az = a[2], aw = a[3];
  let bx = b[0], by = b[1], bz = b[2], bw = b[3];

  let omega, cosom, sinom, scale0, scale1;

  // calc cosine
  cosom = ax * bx + ay * by + az * bz + aw * bw;
  // adjust signs (if necessary)
  if ( cosom < 0.0 ) {
    cosom = -cosom;
    bx = - bx;
    by = - by;
    bz = - bz;
    bw = - bw;
  }
  // calculate coefficients
  if ( (1.0 - cosom) > 0.000001 ) {
    // standard case (slerp)
    omega  = Math.acos(cosom);
    sinom  = Math.sin(omega);
    scale0 = Math.sin((1.0 - t) * omega) / sinom;
    scale1 = Math.sin(t * omega) / sinom;
  } else {
    // "from" and "to" quaternions are very close
    //  ... so we can do a linear interpolation
    scale0 = 1.0 - t;
    scale1 = t;
  }
  // calculate final values
  out[0] = scale0 * ax + scale1 * bx;
  out[1] = scale0 * ay + scale1 * by;
  out[2] = scale0 * az + scale1 * bz;
  out[3] = scale0 * aw + scale1 * bw;

  return out;
}

/**
 * Calculates the inverse of a quat
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a quat to calculate inverse of
 * @returns {quat} out
 */
function invert(out, a) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
  let dot = a0*a0 + a1*a1 + a2*a2 + a3*a3;
  let invDot = dot ? 1.0/dot : 0;

  // TODO: Would be faster to return [0,0,0,0] immediately if dot == 0

  out[0] = -a0*invDot;
  out[1] = -a1*invDot;
  out[2] = -a2*invDot;
  out[3] = a3*invDot;
  return out;
}

/**
 * Calculates the conjugate of a quat
 * If the quaternion is normalized, this function is faster than quat.inverse and produces the same result.
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a quat to calculate conjugate of
 * @returns {quat} out
 */
function conjugate(out, a) {
  out[0] = -a[0];
  out[1] = -a[1];
  out[2] = -a[2];
  out[3] = a[3];
  return out;
}

/**
 * Creates a quaternion from the given 3x3 rotation matrix.
 *
 * NOTE: The resultant quaternion is not normalized, so you should be sure
 * to renormalize the quaternion yourself where necessary.
 *
 * @param {quat} out the receiving quaternion
 * @param {mat3} m rotation matrix
 * @returns {quat} out
 * @function
 */
function fromMat3(out, m) {
  // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
  // article "Quaternion Calculus and Fast Animation".
  let fTrace = m[0] + m[4] + m[8];
  let fRoot;

  if ( fTrace > 0.0 ) {
    // |w| > 1/2, may as well choose w > 1/2
    fRoot = Math.sqrt(fTrace + 1.0);  // 2w
    out[3] = 0.5 * fRoot;
    fRoot = 0.5/fRoot;  // 1/(4w)
    out[0] = (m[5]-m[7])*fRoot;
    out[1] = (m[6]-m[2])*fRoot;
    out[2] = (m[1]-m[3])*fRoot;
  } else {
    // |w| <= 1/2
    let i = 0;
    if ( m[4] > m[0] )
      i = 1;
    if ( m[8] > m[i*3+i] )
      i = 2;
    let j = (i+1)%3;
    let k = (i+2)%3;

    fRoot = Math.sqrt(m[i*3+i]-m[j*3+j]-m[k*3+k] + 1.0);
    out[i] = 0.5 * fRoot;
    fRoot = 0.5 / fRoot;
    out[3] = (m[j*3+k] - m[k*3+j]) * fRoot;
    out[j] = (m[j*3+i] + m[i*3+j]) * fRoot;
    out[k] = (m[k*3+i] + m[i*3+k]) * fRoot;
  }

  return out;
}

/**
 * Creates a quaternion from the given euler angle x, y, z.
 *
 * @param {quat} out the receiving quaternion
 * @param {x} Angle to rotate around X axis in degrees.
 * @param {y} Angle to rotate around Y axis in degrees.
 * @param {z} Angle to rotate around Z axis in degrees.
 * @returns {quat} out
 * @function
 */
function fromEuler(out, x, y, z) {
    let halfToRad = 0.5 * Math.PI / 180.0;
    x *= halfToRad;
    y *= halfToRad;
    z *= halfToRad;

    let sx = Math.sin(x);
    let cx = Math.cos(x);
    let sy = Math.sin(y);
    let cy = Math.cos(y);
    let sz = Math.sin(z);
    let cz = Math.cos(z);

    out[0] = sx * cy * cz - cx * sy * sz;
    out[1] = cx * sy * cz + sx * cy * sz;
    out[2] = cx * cy * sz - sx * sy * cz;
    out[3] = cx * cy * cz + sx * sy * sz;

    return out;
}

/**
 * Returns a string representation of a quatenion
 *
 * @param {quat} a vector to represent as a string
 * @returns {String} string representation of the vector
 */
function str(a) {
  return 'quat(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' + a[3] + ')';
}

/**
 * Creates a new quat initialized with values from an existing quaternion
 *
 * @param {quat} a quaternion to clone
 * @returns {quat} a new quaternion
 * @function
 */
const clone = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["clone"];

/**
 * Creates a new quat initialized with the given values
 *
 * @param {Number} x X component
 * @param {Number} y Y component
 * @param {Number} z Z component
 * @param {Number} w W component
 * @returns {quat} a new quaternion
 * @function
 */
const fromValues = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["fromValues"];

/**
 * Copy the values from one quat to another
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a the source quaternion
 * @returns {quat} out
 * @function
 */
const copy = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["copy"];

/**
 * Set the components of a quat to the given values
 *
 * @param {quat} out the receiving quaternion
 * @param {Number} x X component
 * @param {Number} y Y component
 * @param {Number} z Z component
 * @param {Number} w W component
 * @returns {quat} out
 * @function
 */
const set = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["set"];

/**
 * Adds two quat's
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a the first operand
 * @param {quat} b the second operand
 * @returns {quat} out
 * @function
 */
const add = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["add"];

/**
 * Alias for {@link quat.multiply}
 * @function
 */
const mul = multiply;

/**
 * Scales a quat by a scalar number
 *
 * @param {quat} out the receiving vector
 * @param {quat} a the vector to scale
 * @param {Number} b amount to scale the vector by
 * @returns {quat} out
 * @function
 */
const scale = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["scale"];

/**
 * Calculates the dot product of two quat's
 *
 * @param {quat} a the first operand
 * @param {quat} b the second operand
 * @returns {Number} dot product of a and b
 * @function
 */
const dot = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["dot"];

/**
 * Performs a linear interpolation between two quat's
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a the first operand
 * @param {quat} b the second operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {quat} out
 * @function
 */
const lerp = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["lerp"];

/**
 * Calculates the length of a quat
 *
 * @param {quat} a vector to calculate length of
 * @returns {Number} length of a
 */
const length = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["length"];

/**
 * Alias for {@link quat.length}
 * @function
 */
const len = length;

/**
 * Calculates the squared length of a quat
 *
 * @param {quat} a vector to calculate squared length of
 * @returns {Number} squared length of a
 * @function
 */
const squaredLength = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["squaredLength"];

/**
 * Alias for {@link quat.squaredLength}
 * @function
 */
const sqrLen = squaredLength;

/**
 * Normalize a quat
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a quaternion to normalize
 * @returns {quat} out
 * @function
 */
const normalize = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["normalize"];

/**
 * Returns whether or not the quaternions have exactly the same elements in the same position (when compared with ===)
 *
 * @param {quat} a The first quaternion.
 * @param {quat} b The second quaternion.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
const exactEquals = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["exactEquals"];

/**
 * Returns whether or not the quaternions have approximately the same elements in the same position.
 *
 * @param {quat} a The first vector.
 * @param {quat} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
const equals = _vec4_js__WEBPACK_IMPORTED_MODULE_3__["equals"];

/**
 * Sets a quaternion to represent the shortest rotation from one
 * vector to another.
 *
 * Both vectors are assumed to be unit length.
 *
 * @param {quat} out the receiving quaternion.
 * @param {vec3} a the initial vector
 * @param {vec3} b the destination vector
 * @returns {quat} out
 */
const rotationTo = (function() {
  let tmpvec3 = _vec3_js__WEBPACK_IMPORTED_MODULE_2__["create"]();
  let xUnitVec3 = _vec3_js__WEBPACK_IMPORTED_MODULE_2__["fromValues"](1,0,0);
  let yUnitVec3 = _vec3_js__WEBPACK_IMPORTED_MODULE_2__["fromValues"](0,1,0);

  return function(out, a, b) {
    let dot = _vec3_js__WEBPACK_IMPORTED_MODULE_2__["dot"](a, b);
    if (dot < -0.999999) {
      _vec3_js__WEBPACK_IMPORTED_MODULE_2__["cross"](tmpvec3, xUnitVec3, a);
      if (_vec3_js__WEBPACK_IMPORTED_MODULE_2__["len"](tmpvec3) < 0.000001)
        _vec3_js__WEBPACK_IMPORTED_MODULE_2__["cross"](tmpvec3, yUnitVec3, a);
      _vec3_js__WEBPACK_IMPORTED_MODULE_2__["normalize"](tmpvec3, tmpvec3);
      setAxisAngle(out, tmpvec3, Math.PI);
      return out;
    } else if (dot > 0.999999) {
      out[0] = 0;
      out[1] = 0;
      out[2] = 0;
      out[3] = 1;
      return out;
    } else {
      _vec3_js__WEBPACK_IMPORTED_MODULE_2__["cross"](tmpvec3, a, b);
      out[0] = tmpvec3[0];
      out[1] = tmpvec3[1];
      out[2] = tmpvec3[2];
      out[3] = 1 + dot;
      return normalize(out, out);
    }
  };
})();

/**
 * Performs a spherical linear interpolation with two control points
 *
 * @param {quat} out the receiving quaternion
 * @param {quat} a the first operand
 * @param {quat} b the second operand
 * @param {quat} c the third operand
 * @param {quat} d the fourth operand
 * @param {Number} t interpolation amount
 * @returns {quat} out
 */
const sqlerp = (function () {
  let temp1 = create();
  let temp2 = create();

  return function (out, a, b, c, d, t) {
    slerp(temp1, a, d, t);
    slerp(temp2, b, c, t);
    slerp(out, temp1, temp2, 2 * t * (1 - t));

    return out;
  };
}());

/**
 * Sets the specified quaternion with values corresponding to the given
 * axes. Each axis is a vec3 and is expected to be unit length and
 * perpendicular to all other specified axes.
 *
 * @param {vec3} view  the vector representing the viewing direction
 * @param {vec3} right the vector representing the local "right" direction
 * @param {vec3} up    the vector representing the local "up" direction
 * @returns {quat} out
 */
const setAxes = (function() {
  let matr = _mat3_js__WEBPACK_IMPORTED_MODULE_1__["create"]();

  return function(out, view, right, up) {
    matr[0] = right[0];
    matr[3] = right[1];
    matr[6] = right[2];

    matr[1] = up[0];
    matr[4] = up[1];
    matr[7] = up[2];

    matr[2] = -view[0];
    matr[5] = -view[1];
    matr[8] = -view[2];

    return normalize(out, fromMat3(out, matr));
  };
})();


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/quat2.js":
/*!*******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/quat2.js ***!
  \*******************************************************/
/*! exports provided: create, clone, fromValues, fromRotationTranslationValues, fromRotationTranslation, fromTranslation, fromRotation, fromMat4, copy, identity, set, getReal, getDual, setReal, setDual, getTranslation, translate, rotateX, rotateY, rotateZ, rotateByQuatAppend, rotateByQuatPrepend, rotateAroundAxis, add, multiply, mul, scale, dot, lerp, invert, conjugate, length, len, squaredLength, sqrLen, normalize, str, exactEquals, equals */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotationTranslationValues", function() { return fromRotationTranslationValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotationTranslation", function() { return fromRotationTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromTranslation", function() { return fromTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromRotation", function() { return fromRotation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromMat4", function() { return fromMat4; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "identity", function() { return identity; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getReal", function() { return getReal; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getDual", function() { return getDual; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "setReal", function() { return setReal; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "setDual", function() { return setDual; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "getTranslation", function() { return getTranslation; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "translate", function() { return translate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateX", function() { return rotateX; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateY", function() { return rotateY; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateZ", function() { return rotateZ; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateByQuatAppend", function() { return rotateByQuatAppend; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateByQuatPrepend", function() { return rotateByQuatPrepend; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateAroundAxis", function() { return rotateAroundAxis; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dot", function() { return dot; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "lerp", function() { return lerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "invert", function() { return invert; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "conjugate", function() { return conjugate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "length", function() { return length; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "len", function() { return len; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredLength", function() { return squaredLength; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrLen", function() { return sqrLen; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "normalize", function() { return normalize; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* harmony import */ var _quat_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./quat.js */ "./node_modules/gl-matrix/src/gl-matrix/quat.js");
/* harmony import */ var _mat4_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./mat4.js */ "./node_modules/gl-matrix/src/gl-matrix/mat4.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */






/**
 * Dual Quaternion<br>
 * Format: [real, dual]<br>
 * Quaternion format: XYZW<br>
 * Make sure to have normalized dual quaternions, otherwise the functions may not work as intended.<br>
 * @module quat2
 */


/**
 * Creates a new identity dual quat
 *
 * @returns {quat2} a new dual quaternion [real -> rotation, dual -> translation]
 */
function create() {
  let dq = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](8);
  dq[0] = 0;
  dq[1] = 0;
  dq[2] = 0;
  dq[3] = 1;
  dq[4] = 0;
  dq[5] = 0;
  dq[6] = 0;
  dq[7] = 0;
  return dq;
}

/**
 * Creates a new quat initialized with values from an existing quaternion
 *
 * @param {quat2} a dual quaternion to clone
 * @returns {quat2} new dual quaternion
 * @function
 */
function clone(a) {
  let dq = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](8);
  dq[0] = a[0];
  dq[1] = a[1];
  dq[2] = a[2];
  dq[3] = a[3];
  dq[4] = a[4];
  dq[5] = a[5];
  dq[6] = a[6];
  dq[7] = a[7];
  return dq;
}

/**
 * Creates a new dual quat initialized with the given values
 *
 * @param {Number} x1 X component
 * @param {Number} y1 Y component
 * @param {Number} z1 Z component
 * @param {Number} w1 W component
 * @param {Number} x2 X component
 * @param {Number} y2 Y component
 * @param {Number} z2 Z component
 * @param {Number} w2 W component
 * @returns {quat2} new dual quaternion
 * @function
 */
function fromValues(x1, y1, z1, w1, x2, y2, z2, w2) {
  let dq = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](8);
  dq[0] = x1;
  dq[1] = y1;
  dq[2] = z1;
  dq[3] = w1;
  dq[4] = x2;
  dq[5] = y2;
  dq[6] = z2;
  dq[7] = w2;
  return dq;
}

/**
 * Creates a new dual quat from the given values (quat and translation)
 *
 * @param {Number} x1 X component
 * @param {Number} y1 Y component
 * @param {Number} z1 Z component
 * @param {Number} w1 W component
 * @param {Number} x2 X component (translation)
 * @param {Number} y2 Y component (translation)
 * @param {Number} z2 Z component (translation)
 * @returns {quat2} new dual quaternion
 * @function
 */
function fromRotationTranslationValues(x1, y1, z1, w1, x2, y2, z2) {
  let dq = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](8);
  dq[0] = x1;
  dq[1] = y1;
  dq[2] = z1;
  dq[3] = w1;
  let ax = x2 * 0.5,
    ay = y2 * 0.5,
    az = z2 * 0.5;
  dq[4] = ax * w1 + ay * z1 - az * y1;
  dq[5] = ay * w1 + az * x1 - ax * z1;
  dq[6] = az * w1 + ax * y1 - ay * x1;
  dq[7] = -ax * x1 - ay * y1 - az * z1;
  return dq;
}

/**
 * Creates a dual quat from a quaternion and a translation
 *
 * @param {quat2} dual quaternion receiving operation result
 * @param {quat} q quaternion
 * @param {vec3} t tranlation vector
 * @returns {quat2} dual quaternion receiving operation result
 * @function
 */
function fromRotationTranslation(out, q, t) {
  let ax = t[0] * 0.5,
    ay = t[1] * 0.5,
    az = t[2] * 0.5,
    bx = q[0],
    by = q[1],
    bz = q[2],
    bw = q[3];
  out[0] = bx;
  out[1] = by;
  out[2] = bz;
  out[3] = bw;
  out[4] = ax * bw + ay * bz - az * by;
  out[5] = ay * bw + az * bx - ax * bz;
  out[6] = az * bw + ax * by - ay * bx;
  out[7] = -ax * bx - ay * by - az * bz;
  return out;
}

/**
 * Creates a dual quat from a translation
 *
 * @param {quat2} dual quaternion receiving operation result
 * @param {vec3} t translation vector
 * @returns {quat2} dual quaternion receiving operation result
 * @function
 */
function fromTranslation(out, t) {
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  out[4] = t[0] * 0.5;
  out[5] = t[1] * 0.5;
  out[6] = t[2] * 0.5;
  out[7] = 0;
  return out;
}

/**
 * Creates a dual quat from a quaternion
 *
 * @param {quat2} dual quaternion receiving operation result
 * @param {quat} q the quaternion
 * @returns {quat2} dual quaternion receiving operation result
 * @function
 */
function fromRotation(out, q) {
  out[0] = q[0];
  out[1] = q[1];
  out[2] = q[2];
  out[3] = q[3];
  out[4] = 0;
  out[5] = 0;
  out[6] = 0;
  out[7] = 0;
  return out;
}

/**
 * Creates a new dual quat from a matrix (4x4)
 * 
 * @param {quat2} out the dual quaternion
 * @param {mat4} a the matrix
 * @returns {quat2} dual quat receiving operation result
 * @function
 */
function fromMat4(out, a) {
  //TODO Optimize this 
  let outer = _quat_js__WEBPACK_IMPORTED_MODULE_1__["create"]();
  _mat4_js__WEBPACK_IMPORTED_MODULE_2__["getRotation"](outer, a);
  let t = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](3);
  _mat4_js__WEBPACK_IMPORTED_MODULE_2__["getTranslation"](t, a);
  fromRotationTranslation(out, outer, t);
  return out;
}

/**
 * Copy the values from one dual quat to another
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the source dual quaternion
 * @returns {quat2} out
 * @function
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  out[4] = a[4];
  out[5] = a[5];
  out[6] = a[6];
  out[7] = a[7];
  return out;
}

/**
 * Set a dual quat to the identity dual quaternion
 *
 * @param {quat2} out the receiving quaternion
 * @returns {quat2} out
 */
function identity(out) {
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1;
  out[4] = 0;
  out[5] = 0;
  out[6] = 0;
  out[7] = 0;
  return out;
}

/**
 * Set the components of a dual quat to the given values
 *
 * @param {quat2} out the receiving quaternion
 * @param {Number} x1 X component
 * @param {Number} y1 Y component
 * @param {Number} z1 Z component
 * @param {Number} w1 W component
 * @param {Number} x2 X component
 * @param {Number} y2 Y component
 * @param {Number} z2 Z component
 * @param {Number} w2 W component
 * @returns {quat2} out
 * @function
 */
function set(out, x1, y1, z1, w1, x2, y2, z2, w2) {
  out[0] = x1;
  out[1] = y1;
  out[2] = z1;
  out[3] = w1;

  out[4] = x2;
  out[5] = y2;
  out[6] = z2;
  out[7] = w2;
  return out;
}

/**
 * Gets the real part of a dual quat
 * @param  {quat} out real part
 * @param  {quat2} a Dual Quaternion
 * @return {quat} real part
 */
const getReal = _quat_js__WEBPACK_IMPORTED_MODULE_1__["copy"];

/**
 * Gets the dual part of a dual quat
 * @param  {quat} out dual part
 * @param  {quat2} a Dual Quaternion
 * @return {quat} dual part
 */
function getDual(out, a) {
  out[0] = a[4];
  out[1] = a[5];
  out[2] = a[6];
  out[3] = a[7];
  return out;
}

/**
 * Set the real component of a dual quat to the given quaternion
 *
 * @param {quat2} out the receiving quaternion
 * @param {quat} q a quaternion representing the real part
 * @returns {quat2} out
 * @function
 */
const setReal = _quat_js__WEBPACK_IMPORTED_MODULE_1__["copy"];

/**
 * Set the dual component of a dual quat to the given quaternion
 *
 * @param {quat2} out the receiving quaternion
 * @param {quat} q a quaternion representing the dual part
 * @returns {quat2} out
 * @function
 */
function setDual(out, q) {
  out[4] = q[0];
  out[5] = q[1];
  out[6] = q[2];
  out[7] = q[3];
  return out;
}

/**
 * Gets the translation of a normalized dual quat
 * @param  {vec3} out translation
 * @param  {quat2} a Dual Quaternion to be decomposed
 * @return {vec3} translation
 */
function getTranslation(out, a) {
  let ax = a[4],
    ay = a[5],
    az = a[6],
    aw = a[7],
    bx = -a[0],
    by = -a[1],
    bz = -a[2],
    bw = a[3];
  out[0] = (ax * bw + aw * bx + ay * bz - az * by) * 2;
  out[1] = (ay * bw + aw * by + az * bx - ax * bz) * 2;
  out[2] = (az * bw + aw * bz + ax * by - ay * bx) * 2;
  return out;
}

/**
 * Translates a dual quat by the given vector
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the dual quaternion to translate
 * @param {vec3} v vector to translate by
 * @returns {quat2} out
 */
function translate(out, a, v) {
  let ax1 = a[0],
    ay1 = a[1],
    az1 = a[2],
    aw1 = a[3],
    bx1 = v[0] * 0.5,
    by1 = v[1] * 0.5,
    bz1 = v[2] * 0.5,
    ax2 = a[4],
    ay2 = a[5],
    az2 = a[6],
    aw2 = a[7];
  out[0] = ax1;
  out[1] = ay1;
  out[2] = az1;
  out[3] = aw1;
  out[4] = aw1 * bx1 + ay1 * bz1 - az1 * by1 + ax2;
  out[5] = aw1 * by1 + az1 * bx1 - ax1 * bz1 + ay2;
  out[6] = aw1 * bz1 + ax1 * by1 - ay1 * bx1 + az2;
  out[7] = -ax1 * bx1 - ay1 * by1 - az1 * bz1 + aw2;
  return out;
}

/**
 * Rotates a dual quat around the X axis
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the dual quaternion to rotate
 * @param {number} rad how far should the rotation be
 * @returns {quat2} out
 */
function rotateX(out, a, rad) {
  let bx = -a[0],
    by = -a[1],
    bz = -a[2],
    bw = a[3],
    ax = a[4],
    ay = a[5],
    az = a[6],
    aw = a[7],
    ax1 = ax * bw + aw * bx + ay * bz - az * by,
    ay1 = ay * bw + aw * by + az * bx - ax * bz,
    az1 = az * bw + aw * bz + ax * by - ay * bx,
    aw1 = aw * bw - ax * bx - ay * by - az * bz;
  _quat_js__WEBPACK_IMPORTED_MODULE_1__["rotateX"](out, a, rad);
  bx = out[0];
  by = out[1];
  bz = out[2];
  bw = out[3];
  out[4] = ax1 * bw + aw1 * bx + ay1 * bz - az1 * by;
  out[5] = ay1 * bw + aw1 * by + az1 * bx - ax1 * bz;
  out[6] = az1 * bw + aw1 * bz + ax1 * by - ay1 * bx;
  out[7] = aw1 * bw - ax1 * bx - ay1 * by - az1 * bz;
  return out;
}

/**
 * Rotates a dual quat around the Y axis
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the dual quaternion to rotate
 * @param {number} rad how far should the rotation be
 * @returns {quat2} out
 */
function rotateY(out, a, rad) {
  let bx = -a[0],
    by = -a[1],
    bz = -a[2],
    bw = a[3],
    ax = a[4],
    ay = a[5],
    az = a[6],
    aw = a[7],
    ax1 = ax * bw + aw * bx + ay * bz - az * by,
    ay1 = ay * bw + aw * by + az * bx - ax * bz,
    az1 = az * bw + aw * bz + ax * by - ay * bx,
    aw1 = aw * bw - ax * bx - ay * by - az * bz;
  _quat_js__WEBPACK_IMPORTED_MODULE_1__["rotateY"](out, a, rad);
  bx = out[0];
  by = out[1];
  bz = out[2];
  bw = out[3];
  out[4] = ax1 * bw + aw1 * bx + ay1 * bz - az1 * by;
  out[5] = ay1 * bw + aw1 * by + az1 * bx - ax1 * bz;
  out[6] = az1 * bw + aw1 * bz + ax1 * by - ay1 * bx;
  out[7] = aw1 * bw - ax1 * bx - ay1 * by - az1 * bz;
  return out;
}

/**
 * Rotates a dual quat around the Z axis
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the dual quaternion to rotate
 * @param {number} rad how far should the rotation be
 * @returns {quat2} out
 */
function rotateZ(out, a, rad) {
  let bx = -a[0],
    by = -a[1],
    bz = -a[2],
    bw = a[3],
    ax = a[4],
    ay = a[5],
    az = a[6],
    aw = a[7],
    ax1 = ax * bw + aw * bx + ay * bz - az * by,
    ay1 = ay * bw + aw * by + az * bx - ax * bz,
    az1 = az * bw + aw * bz + ax * by - ay * bx,
    aw1 = aw * bw - ax * bx - ay * by - az * bz;
  _quat_js__WEBPACK_IMPORTED_MODULE_1__["rotateZ"](out, a, rad);
  bx = out[0];
  by = out[1];
  bz = out[2];
  bw = out[3];
  out[4] = ax1 * bw + aw1 * bx + ay1 * bz - az1 * by;
  out[5] = ay1 * bw + aw1 * by + az1 * bx - ax1 * bz;
  out[6] = az1 * bw + aw1 * bz + ax1 * by - ay1 * bx;
  out[7] = aw1 * bw - ax1 * bx - ay1 * by - az1 * bz;
  return out;
}

/**
 * Rotates a dual quat by a given quaternion (a * q)
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the dual quaternion to rotate
 * @param {quat} q quaternion to rotate by
 * @returns {quat2} out
 */
function rotateByQuatAppend(out, a, q) {
  let qx = q[0],
    qy = q[1],
    qz = q[2],
    qw = q[3],
    ax = a[0],
    ay = a[1],
    az = a[2],
    aw = a[3];

  out[0] = ax * qw + aw * qx + ay * qz - az * qy;
  out[1] = ay * qw + aw * qy + az * qx - ax * qz;
  out[2] = az * qw + aw * qz + ax * qy - ay * qx;
  out[3] = aw * qw - ax * qx - ay * qy - az * qz;
  ax = a[4];
  ay = a[5];
  az = a[6];
  aw = a[7];
  out[4] = ax * qw + aw * qx + ay * qz - az * qy;
  out[5] = ay * qw + aw * qy + az * qx - ax * qz;
  out[6] = az * qw + aw * qz + ax * qy - ay * qx;
  out[7] = aw * qw - ax * qx - ay * qy - az * qz;
  return out;
}

/**
 * Rotates a dual quat by a given quaternion (q * a)
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat} q quaternion to rotate by
 * @param {quat2} a the dual quaternion to rotate
 * @returns {quat2} out
 */
function rotateByQuatPrepend(out, q, a) {
  let qx = q[0],
    qy = q[1],
    qz = q[2],
    qw = q[3],
    bx = a[0],
    by = a[1],
    bz = a[2],
    bw = a[3];

  out[0] = qx * bw + qw * bx + qy * bz - qz * by;
  out[1] = qy * bw + qw * by + qz * bx - qx * bz;
  out[2] = qz * bw + qw * bz + qx * by - qy * bx;
  out[3] = qw * bw - qx * bx - qy * by - qz * bz;
  bx = a[4];
  by = a[5];
  bz = a[6];
  bw = a[7];
  out[4] = qx * bw + qw * bx + qy * bz - qz * by;
  out[5] = qy * bw + qw * by + qz * bx - qx * bz;
  out[6] = qz * bw + qw * bz + qx * by - qy * bx;
  out[7] = qw * bw - qx * bx - qy * by - qz * bz;
  return out;
}

/**
 * Rotates a dual quat around a given axis. Does the normalisation automatically
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the dual quaternion to rotate
 * @param {vec3} axis the axis to rotate around
 * @param {Number} rad how far the rotation should be
 * @returns {quat2} out
 */
function rotateAroundAxis(out, a, axis, rad) {
  //Special case for rad = 0
  if (Math.abs(rad) < _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]) {
    return copy(out, a);
  }
  let axisLength = Math.sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);

  rad = rad * 0.5;
  let s = Math.sin(rad);
  let bx = s * axis[0] / axisLength;
  let by = s * axis[1] / axisLength;
  let bz = s * axis[2] / axisLength;
  let bw = Math.cos(rad);

  let ax1 = a[0],
    ay1 = a[1],
    az1 = a[2],
    aw1 = a[3];
  out[0] = ax1 * bw + aw1 * bx + ay1 * bz - az1 * by;
  out[1] = ay1 * bw + aw1 * by + az1 * bx - ax1 * bz;
  out[2] = az1 * bw + aw1 * bz + ax1 * by - ay1 * bx;
  out[3] = aw1 * bw - ax1 * bx - ay1 * by - az1 * bz;

  let ax = a[4],
    ay = a[5],
    az = a[6],
    aw = a[7];
  out[4] = ax * bw + aw * bx + ay * bz - az * by;
  out[5] = ay * bw + aw * by + az * bx - ax * bz;
  out[6] = az * bw + aw * bz + ax * by - ay * bx;
  out[7] = aw * bw - ax * bx - ay * by - az * bz;

  return out;
}

/**
 * Adds two dual quat's
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the first operand
 * @param {quat2} b the second operand
 * @returns {quat2} out
 * @function
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  out[3] = a[3] + b[3];
  out[4] = a[4] + b[4];
  out[5] = a[5] + b[5];
  out[6] = a[6] + b[6];
  out[7] = a[7] + b[7];
  return out;
}

/**
 * Multiplies two dual quat's
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a the first operand
 * @param {quat2} b the second operand
 * @returns {quat2} out
 */
function multiply(out, a, b) {
  let ax0 = a[0],
    ay0 = a[1],
    az0 = a[2],
    aw0 = a[3],
    bx1 = b[4],
    by1 = b[5],
    bz1 = b[6],
    bw1 = b[7],
    ax1 = a[4],
    ay1 = a[5],
    az1 = a[6],
    aw1 = a[7],
    bx0 = b[0],
    by0 = b[1],
    bz0 = b[2],
    bw0 = b[3];
  out[0] = ax0 * bw0 + aw0 * bx0 + ay0 * bz0 - az0 * by0;
  out[1] = ay0 * bw0 + aw0 * by0 + az0 * bx0 - ax0 * bz0;
  out[2] = az0 * bw0 + aw0 * bz0 + ax0 * by0 - ay0 * bx0;
  out[3] = aw0 * bw0 - ax0 * bx0 - ay0 * by0 - az0 * bz0;
  out[4] = ax0 * bw1 + aw0 * bx1 + ay0 * bz1 - az0 * by1 + ax1 * bw0 + aw1 * bx0 + ay1 * bz0 - az1 * by0;
  out[5] = ay0 * bw1 + aw0 * by1 + az0 * bx1 - ax0 * bz1 + ay1 * bw0 + aw1 * by0 + az1 * bx0 - ax1 * bz0;
  out[6] = az0 * bw1 + aw0 * bz1 + ax0 * by1 - ay0 * bx1 + az1 * bw0 + aw1 * bz0 + ax1 * by0 - ay1 * bx0;
  out[7] = aw0 * bw1 - ax0 * bx1 - ay0 * by1 - az0 * bz1 + aw1 * bw0 - ax1 * bx0 - ay1 * by0 - az1 * bz0;
  return out;
}

/**
 * Alias for {@link quat2.multiply}
 * @function
 */
const mul = multiply;

/**
 * Scales a dual quat by a scalar number
 *
 * @param {quat2} out the receiving dual quat
 * @param {quat2} a the dual quat to scale
 * @param {Number} b amount to scale the dual quat by
 * @returns {quat2} out
 * @function
 */
function scale(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  out[3] = a[3] * b;
  out[4] = a[4] * b;
  out[5] = a[5] * b;
  out[6] = a[6] * b;
  out[7] = a[7] * b;
  return out;
}

/**
 * Calculates the dot product of two dual quat's (The dot product of the real parts)
 *
 * @param {quat2} a the first operand
 * @param {quat2} b the second operand
 * @returns {Number} dot product of a and b
 * @function
 */
const dot = _quat_js__WEBPACK_IMPORTED_MODULE_1__["dot"];

/**
 * Performs a linear interpolation between two dual quats's
 * NOTE: The resulting dual quaternions won't always be normalized (The error is most noticeable when t = 0.5)
 *
 * @param {quat2} out the receiving dual quat
 * @param {quat2} a the first operand
 * @param {quat2} b the second operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {quat2} out
 */
function lerp(out, a, b, t) {
  let mt = 1 - t;
  if (dot(a, b) < 0) t = -t;

  out[0] = a[0] * mt + b[0] * t;
  out[1] = a[1] * mt + b[1] * t;
  out[2] = a[2] * mt + b[2] * t;
  out[3] = a[3] * mt + b[3] * t;
  out[4] = a[4] * mt + b[4] * t;
  out[5] = a[5] * mt + b[5] * t;
  out[6] = a[6] * mt + b[6] * t;
  out[7] = a[7] * mt + b[7] * t;

  return out;
}

/**
 * Calculates the inverse of a dual quat. If they are normalized, conjugate is cheaper
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a dual quat to calculate inverse of
 * @returns {quat2} out
 */
function invert(out, a) {
  let sqlen = squaredLength(a);
  out[0] = -a[0] / sqlen;
  out[1] = -a[1] / sqlen;
  out[2] = -a[2] / sqlen;
  out[3] = a[3] / sqlen;
  out[4] = -a[4] / sqlen;
  out[5] = -a[5] / sqlen;
  out[6] = -a[6] / sqlen;
  out[7] = a[7] / sqlen;
  return out;
}

/**
 * Calculates the conjugate of a dual quat
 * If the dual quaternion is normalized, this function is faster than quat2.inverse and produces the same result.
 *
 * @param {quat2} out the receiving quaternion
 * @param {quat2} a quat to calculate conjugate of
 * @returns {quat2} out
 */
function conjugate(out, a) {
  out[0] = -a[0];
  out[1] = -a[1];
  out[2] = -a[2];
  out[3] = a[3];
  out[4] = -a[4];
  out[5] = -a[5];
  out[6] = -a[6];
  out[7] = a[7];
  return out;
}

/**
 * Calculates the length of a dual quat
 *
 * @param {quat2} a dual quat to calculate length of
 * @returns {Number} length of a
 * @function
 */
const length = _quat_js__WEBPACK_IMPORTED_MODULE_1__["length"];

/**
 * Alias for {@link quat2.length}
 * @function
 */
const len = length;

/**
 * Calculates the squared length of a dual quat
 *
 * @param {quat2} a dual quat to calculate squared length of
 * @returns {Number} squared length of a
 * @function
 */
const squaredLength = _quat_js__WEBPACK_IMPORTED_MODULE_1__["squaredLength"];

/**
 * Alias for {@link quat2.squaredLength}
 * @function
 */
const sqrLen = squaredLength;

/**
 * Normalize a dual quat
 *
 * @param {quat2} out the receiving dual quaternion
 * @param {quat2} a dual quaternion to normalize
 * @returns {quat2} out
 * @function
 */
function normalize(out, a) {
  let magnitude = squaredLength(a);
  if (magnitude > 0) {
    magnitude = Math.sqrt(magnitude);
    out[0] = a[0] / magnitude;
    out[1] = a[1] / magnitude;
    out[2] = a[2] / magnitude;
    out[3] = a[3] / magnitude;
    out[4] = a[4] / magnitude;
    out[5] = a[5] / magnitude;
    out[6] = a[6] / magnitude;
    out[7] = a[7] / magnitude;
  }
  return out;
}

/**
 * Returns a string representation of a dual quatenion
 *
 * @param {quat2} a dual quaternion to represent as a string
 * @returns {String} string representation of the dual quat
 */
function str(a) {
  return 'quat2(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' + a[3] + ', ' +
    a[4] + ', ' + a[5] + ', ' + a[6] + ', ' + a[7] + ')';
}

/**
 * Returns whether or not the dual quaternions have exactly the same elements in the same position (when compared with ===)
 *
 * @param {quat2} a the first dual quaternion.
 * @param {quat2} b the second dual quaternion.
 * @returns {Boolean} true if the dual quaternions are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] && a[3] === b[3] &&
    a[4] === b[4] && a[5] === b[5] && a[6] === b[6] && a[7] === b[7];
}

/**
 * Returns whether or not the dual quaternions have approximately the same elements in the same position.
 *
 * @param {quat2} a the first dual quat.
 * @param {quat2} b the second dual quat.
 * @returns {Boolean} true if the dual quats are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0],
    a1 = a[1],
    a2 = a[2],
    a3 = a[3],
    a4 = a[4],
    a5 = a[5],
    a6 = a[6],
    a7 = a[7];
  let b0 = b[0],
    b1 = b[1],
    b2 = b[2],
    b3 = b[3],
    b4 = b[4],
    b5 = b[5],
    b6 = b[6],
    b7 = b[7];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
    Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
    Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a2), Math.abs(b2)) &&
    Math.abs(a3 - b3) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a3), Math.abs(b3)) &&
    Math.abs(a4 - b4) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a4), Math.abs(b4)) &&
    Math.abs(a5 - b5) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a5), Math.abs(b5)) &&
    Math.abs(a6 - b6) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a6), Math.abs(b6)) &&
    Math.abs(a7 - b7) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"] * Math.max(1.0, Math.abs(a7), Math.abs(b7)));
}


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/vec2.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/vec2.js ***!
  \******************************************************/
/*! exports provided: create, clone, fromValues, copy, set, add, subtract, multiply, divide, ceil, floor, min, max, round, scale, scaleAndAdd, distance, squaredDistance, length, squaredLength, negate, inverse, normalize, dot, cross, lerp, random, transformMat2, transformMat2d, transformMat3, transformMat4, str, exactEquals, equals, len, sub, mul, div, dist, sqrDist, sqrLen, forEach */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "divide", function() { return divide; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ceil", function() { return ceil; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "floor", function() { return floor; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "min", function() { return min; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "max", function() { return max; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "round", function() { return round; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scaleAndAdd", function() { return scaleAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "distance", function() { return distance; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredDistance", function() { return squaredDistance; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "length", function() { return length; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredLength", function() { return squaredLength; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "negate", function() { return negate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "inverse", function() { return inverse; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "normalize", function() { return normalize; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dot", function() { return dot; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "cross", function() { return cross; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "lerp", function() { return lerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "random", function() { return random; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat2", function() { return transformMat2; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat2d", function() { return transformMat2d; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat3", function() { return transformMat3; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat4", function() { return transformMat4; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "len", function() { return len; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "div", function() { return div; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dist", function() { return dist; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrDist", function() { return sqrDist; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrLen", function() { return sqrLen; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "forEach", function() { return forEach; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * 2 Dimensional Vector
 * @module vec2
 */

/**
 * Creates a new, empty vec2
 *
 * @returns {vec2} a new 2D vector
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](2);
  out[0] = 0;
  out[1] = 0;
  return out;
}

/**
 * Creates a new vec2 initialized with values from an existing vector
 *
 * @param {vec2} a vector to clone
 * @returns {vec2} a new 2D vector
 */
function clone(a) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](2);
  out[0] = a[0];
  out[1] = a[1];
  return out;
}

/**
 * Creates a new vec2 initialized with the given values
 *
 * @param {Number} x X component
 * @param {Number} y Y component
 * @returns {vec2} a new 2D vector
 */
function fromValues(x, y) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](2);
  out[0] = x;
  out[1] = y;
  return out;
}

/**
 * Copy the values from one vec2 to another
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the source vector
 * @returns {vec2} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  return out;
}

/**
 * Set the components of a vec2 to the given values
 *
 * @param {vec2} out the receiving vector
 * @param {Number} x X component
 * @param {Number} y Y component
 * @returns {vec2} out
 */
function set(out, x, y) {
  out[0] = x;
  out[1] = y;
  return out;
}

/**
 * Adds two vec2's
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec2} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  return out;
}

/**
 * Subtracts vector b from vector a
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec2} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  return out;
}

/**
 * Multiplies two vec2's
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec2} out
 */
function multiply(out, a, b) {
  out[0] = a[0] * b[0];
  out[1] = a[1] * b[1];
  return out;
};

/**
 * Divides two vec2's
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec2} out
 */
function divide(out, a, b) {
  out[0] = a[0] / b[0];
  out[1] = a[1] / b[1];
  return out;
};

/**
 * Math.ceil the components of a vec2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a vector to ceil
 * @returns {vec2} out
 */
function ceil(out, a) {
  out[0] = Math.ceil(a[0]);
  out[1] = Math.ceil(a[1]);
  return out;
};

/**
 * Math.floor the components of a vec2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a vector to floor
 * @returns {vec2} out
 */
function floor(out, a) {
  out[0] = Math.floor(a[0]);
  out[1] = Math.floor(a[1]);
  return out;
};

/**
 * Returns the minimum of two vec2's
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec2} out
 */
function min(out, a, b) {
  out[0] = Math.min(a[0], b[0]);
  out[1] = Math.min(a[1], b[1]);
  return out;
};

/**
 * Returns the maximum of two vec2's
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec2} out
 */
function max(out, a, b) {
  out[0] = Math.max(a[0], b[0]);
  out[1] = Math.max(a[1], b[1]);
  return out;
};

/**
 * Math.round the components of a vec2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a vector to round
 * @returns {vec2} out
 */
function round (out, a) {
  out[0] = Math.round(a[0]);
  out[1] = Math.round(a[1]);
  return out;
};

/**
 * Scales a vec2 by a scalar number
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the vector to scale
 * @param {Number} b amount to scale the vector by
 * @returns {vec2} out
 */
function scale(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  return out;
};

/**
 * Adds two vec2's after scaling the second operand by a scalar value
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @param {Number} scale the amount to scale b by before adding
 * @returns {vec2} out
 */
function scaleAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  return out;
};

/**
 * Calculates the euclidian distance between two vec2's
 *
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {Number} distance between a and b
 */
function distance(a, b) {
  var x = b[0] - a[0],
    y = b[1] - a[1];
  return Math.sqrt(x*x + y*y);
};

/**
 * Calculates the squared euclidian distance between two vec2's
 *
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {Number} squared distance between a and b
 */
function squaredDistance(a, b) {
  var x = b[0] - a[0],
    y = b[1] - a[1];
  return x*x + y*y;
};

/**
 * Calculates the length of a vec2
 *
 * @param {vec2} a vector to calculate length of
 * @returns {Number} length of a
 */
function length(a) {
  var x = a[0],
    y = a[1];
  return Math.sqrt(x*x + y*y);
};

/**
 * Calculates the squared length of a vec2
 *
 * @param {vec2} a vector to calculate squared length of
 * @returns {Number} squared length of a
 */
function squaredLength (a) {
  var x = a[0],
    y = a[1];
  return x*x + y*y;
};

/**
 * Negates the components of a vec2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a vector to negate
 * @returns {vec2} out
 */
function negate(out, a) {
  out[0] = -a[0];
  out[1] = -a[1];
  return out;
};

/**
 * Returns the inverse of the components of a vec2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a vector to invert
 * @returns {vec2} out
 */
function inverse(out, a) {
  out[0] = 1.0 / a[0];
  out[1] = 1.0 / a[1];
  return out;
};

/**
 * Normalize a vec2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a vector to normalize
 * @returns {vec2} out
 */
function normalize(out, a) {
  var x = a[0],
    y = a[1];
  var len = x*x + y*y;
  if (len > 0) {
    //TODO: evaluate use of glm_invsqrt here?
    len = 1 / Math.sqrt(len);
    out[0] = a[0] * len;
    out[1] = a[1] * len;
  }
  return out;
};

/**
 * Calculates the dot product of two vec2's
 *
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {Number} dot product of a and b
 */
function dot(a, b) {
  return a[0] * b[0] + a[1] * b[1];
};

/**
 * Computes the cross product of two vec2's
 * Note that the cross product must by definition produce a 3D vector
 *
 * @param {vec3} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @returns {vec3} out
 */
function cross(out, a, b) {
  var z = a[0] * b[1] - a[1] * b[0];
  out[0] = out[1] = 0;
  out[2] = z;
  return out;
};

/**
 * Performs a linear interpolation between two vec2's
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the first operand
 * @param {vec2} b the second operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {vec2} out
 */
function lerp(out, a, b, t) {
  var ax = a[0],
    ay = a[1];
  out[0] = ax + t * (b[0] - ax);
  out[1] = ay + t * (b[1] - ay);
  return out;
};

/**
 * Generates a random vector with the given scale
 *
 * @param {vec2} out the receiving vector
 * @param {Number} [scale] Length of the resulting vector. If ommitted, a unit vector will be returned
 * @returns {vec2} out
 */
function random(out, scale) {
  scale = scale || 1.0;
  var r = _common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]() * 2.0 * Math.PI;
  out[0] = Math.cos(r) * scale;
  out[1] = Math.sin(r) * scale;
  return out;
};

/**
 * Transforms the vec2 with a mat2
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the vector to transform
 * @param {mat2} m matrix to transform with
 * @returns {vec2} out
 */
function transformMat2(out, a, m) {
  var x = a[0],
    y = a[1];
  out[0] = m[0] * x + m[2] * y;
  out[1] = m[1] * x + m[3] * y;
  return out;
};

/**
 * Transforms the vec2 with a mat2d
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the vector to transform
 * @param {mat2d} m matrix to transform with
 * @returns {vec2} out
 */
function transformMat2d(out, a, m) {
  var x = a[0],
    y = a[1];
  out[0] = m[0] * x + m[2] * y + m[4];
  out[1] = m[1] * x + m[3] * y + m[5];
  return out;
};

/**
 * Transforms the vec2 with a mat3
 * 3rd vector component is implicitly '1'
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the vector to transform
 * @param {mat3} m matrix to transform with
 * @returns {vec2} out
 */
function transformMat3(out, a, m) {
  var x = a[0],
    y = a[1];
  out[0] = m[0] * x + m[3] * y + m[6];
  out[1] = m[1] * x + m[4] * y + m[7];
  return out;
};

/**
 * Transforms the vec2 with a mat4
 * 3rd vector component is implicitly '0'
 * 4th vector component is implicitly '1'
 *
 * @param {vec2} out the receiving vector
 * @param {vec2} a the vector to transform
 * @param {mat4} m matrix to transform with
 * @returns {vec2} out
 */
function transformMat4(out, a, m) {
  let x = a[0];
  let y = a[1];
  out[0] = m[0] * x + m[4] * y + m[12];
  out[1] = m[1] * x + m[5] * y + m[13];
  return out;
}

/**
 * Returns a string representation of a vector
 *
 * @param {vec2} a vector to represent as a string
 * @returns {String} string representation of the vector
 */
function str(a) {
  return 'vec2(' + a[0] + ', ' + a[1] + ')';
}

/**
 * Returns whether or not the vectors exactly have the same elements in the same position (when compared with ===)
 *
 * @param {vec2} a The first vector.
 * @param {vec2} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1];
}

/**
 * Returns whether or not the vectors have approximately the same elements in the same position.
 *
 * @param {vec2} a The first vector.
 * @param {vec2} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0], a1 = a[1];
  let b0 = b[0], b1 = b[1];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)));
}

/**
 * Alias for {@link vec2.length}
 * @function
 */
const len = length;

/**
 * Alias for {@link vec2.subtract}
 * @function
 */
const sub = subtract;

/**
 * Alias for {@link vec2.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link vec2.divide}
 * @function
 */
const div = divide;

/**
 * Alias for {@link vec2.distance}
 * @function
 */
const dist = distance;

/**
 * Alias for {@link vec2.squaredDistance}
 * @function
 */
const sqrDist = squaredDistance;

/**
 * Alias for {@link vec2.squaredLength}
 * @function
 */
const sqrLen = squaredLength;

/**
 * Perform some operation over an array of vec2s.
 *
 * @param {Array} a the array of vectors to iterate over
 * @param {Number} stride Number of elements between the start of each vec2. If 0 assumes tightly packed
 * @param {Number} offset Number of elements to skip at the beginning of the array
 * @param {Number} count Number of vec2s to iterate over. If 0 iterates over entire array
 * @param {Function} fn Function to call for each vector in the array
 * @param {Object} [arg] additional argument to pass to fn
 * @returns {Array} a
 * @function
 */
const forEach = (function() {
  let vec = create();

  return function(a, stride, offset, count, fn, arg) {
    let i, l;
    if(!stride) {
      stride = 2;
    }

    if(!offset) {
      offset = 0;
    }

    if(count) {
      l = Math.min((count * stride) + offset, a.length);
    } else {
      l = a.length;
    }

    for(i = offset; i < l; i += stride) {
      vec[0] = a[i]; vec[1] = a[i+1];
      fn(vec, vec, arg);
      a[i] = vec[0]; a[i+1] = vec[1];
    }

    return a;
  };
})();


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/vec3.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/vec3.js ***!
  \******************************************************/
/*! exports provided: create, clone, length, fromValues, copy, set, add, subtract, multiply, divide, ceil, floor, min, max, round, scale, scaleAndAdd, distance, squaredDistance, squaredLength, negate, inverse, normalize, dot, cross, lerp, hermite, bezier, random, transformMat4, transformMat3, transformQuat, rotateX, rotateY, rotateZ, angle, str, exactEquals, equals, sub, mul, div, dist, sqrDist, len, sqrLen, forEach */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "length", function() { return length; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "divide", function() { return divide; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ceil", function() { return ceil; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "floor", function() { return floor; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "min", function() { return min; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "max", function() { return max; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "round", function() { return round; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scaleAndAdd", function() { return scaleAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "distance", function() { return distance; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredDistance", function() { return squaredDistance; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredLength", function() { return squaredLength; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "negate", function() { return negate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "inverse", function() { return inverse; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "normalize", function() { return normalize; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dot", function() { return dot; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "cross", function() { return cross; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "lerp", function() { return lerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "hermite", function() { return hermite; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "bezier", function() { return bezier; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "random", function() { return random; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat4", function() { return transformMat4; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat3", function() { return transformMat3; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformQuat", function() { return transformQuat; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateX", function() { return rotateX; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateY", function() { return rotateY; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "rotateZ", function() { return rotateZ; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "angle", function() { return angle; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "div", function() { return div; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dist", function() { return dist; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrDist", function() { return sqrDist; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "len", function() { return len; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrLen", function() { return sqrLen; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "forEach", function() { return forEach; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * 3 Dimensional Vector
 * @module vec3
 */

/**
 * Creates a new, empty vec3
 *
 * @returns {vec3} a new 3D vector
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](3);
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  return out;
}

/**
 * Creates a new vec3 initialized with values from an existing vector
 *
 * @param {vec3} a vector to clone
 * @returns {vec3} a new 3D vector
 */
function clone(a) {
  var out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](3);
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  return out;
}

/**
 * Calculates the length of a vec3
 *
 * @param {vec3} a vector to calculate length of
 * @returns {Number} length of a
 */
function length(a) {
  let x = a[0];
  let y = a[1];
  let z = a[2];
  return Math.sqrt(x*x + y*y + z*z);
}

/**
 * Creates a new vec3 initialized with the given values
 *
 * @param {Number} x X component
 * @param {Number} y Y component
 * @param {Number} z Z component
 * @returns {vec3} a new 3D vector
 */
function fromValues(x, y, z) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](3);
  out[0] = x;
  out[1] = y;
  out[2] = z;
  return out;
}

/**
 * Copy the values from one vec3 to another
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the source vector
 * @returns {vec3} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  return out;
}

/**
 * Set the components of a vec3 to the given values
 *
 * @param {vec3} out the receiving vector
 * @param {Number} x X component
 * @param {Number} y Y component
 * @param {Number} z Z component
 * @returns {vec3} out
 */
function set(out, x, y, z) {
  out[0] = x;
  out[1] = y;
  out[2] = z;
  return out;
}

/**
 * Adds two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  return out;
}

/**
 * Subtracts vector b from vector a
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  return out;
}

/**
 * Multiplies two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function multiply(out, a, b) {
  out[0] = a[0] * b[0];
  out[1] = a[1] * b[1];
  out[2] = a[2] * b[2];
  return out;
}

/**
 * Divides two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function divide(out, a, b) {
  out[0] = a[0] / b[0];
  out[1] = a[1] / b[1];
  out[2] = a[2] / b[2];
  return out;
}

/**
 * Math.ceil the components of a vec3
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a vector to ceil
 * @returns {vec3} out
 */
function ceil(out, a) {
  out[0] = Math.ceil(a[0]);
  out[1] = Math.ceil(a[1]);
  out[2] = Math.ceil(a[2]);
  return out;
}

/**
 * Math.floor the components of a vec3
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a vector to floor
 * @returns {vec3} out
 */
function floor(out, a) {
  out[0] = Math.floor(a[0]);
  out[1] = Math.floor(a[1]);
  out[2] = Math.floor(a[2]);
  return out;
}

/**
 * Returns the minimum of two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function min(out, a, b) {
  out[0] = Math.min(a[0], b[0]);
  out[1] = Math.min(a[1], b[1]);
  out[2] = Math.min(a[2], b[2]);
  return out;
}

/**
 * Returns the maximum of two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function max(out, a, b) {
  out[0] = Math.max(a[0], b[0]);
  out[1] = Math.max(a[1], b[1]);
  out[2] = Math.max(a[2], b[2]);
  return out;
}

/**
 * Math.round the components of a vec3
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a vector to round
 * @returns {vec3} out
 */
function round(out, a) {
  out[0] = Math.round(a[0]);
  out[1] = Math.round(a[1]);
  out[2] = Math.round(a[2]);
  return out;
}

/**
 * Scales a vec3 by a scalar number
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the vector to scale
 * @param {Number} b amount to scale the vector by
 * @returns {vec3} out
 */
function scale(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  return out;
}

/**
 * Adds two vec3's after scaling the second operand by a scalar value
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @param {Number} scale the amount to scale b by before adding
 * @returns {vec3} out
 */
function scaleAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  out[2] = a[2] + (b[2] * scale);
  return out;
}

/**
 * Calculates the euclidian distance between two vec3's
 *
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {Number} distance between a and b
 */
function distance(a, b) {
  let x = b[0] - a[0];
  let y = b[1] - a[1];
  let z = b[2] - a[2];
  return Math.sqrt(x*x + y*y + z*z);
}

/**
 * Calculates the squared euclidian distance between two vec3's
 *
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {Number} squared distance between a and b
 */
function squaredDistance(a, b) {
  let x = b[0] - a[0];
  let y = b[1] - a[1];
  let z = b[2] - a[2];
  return x*x + y*y + z*z;
}

/**
 * Calculates the squared length of a vec3
 *
 * @param {vec3} a vector to calculate squared length of
 * @returns {Number} squared length of a
 */
function squaredLength(a) {
  let x = a[0];
  let y = a[1];
  let z = a[2];
  return x*x + y*y + z*z;
}

/**
 * Negates the components of a vec3
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a vector to negate
 * @returns {vec3} out
 */
function negate(out, a) {
  out[0] = -a[0];
  out[1] = -a[1];
  out[2] = -a[2];
  return out;
}

/**
 * Returns the inverse of the components of a vec3
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a vector to invert
 * @returns {vec3} out
 */
function inverse(out, a) {
  out[0] = 1.0 / a[0];
  out[1] = 1.0 / a[1];
  out[2] = 1.0 / a[2];
  return out;
}

/**
 * Normalize a vec3
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a vector to normalize
 * @returns {vec3} out
 */
function normalize(out, a) {
  let x = a[0];
  let y = a[1];
  let z = a[2];
  let len = x*x + y*y + z*z;
  if (len > 0) {
    //TODO: evaluate use of glm_invsqrt here?
    len = 1 / Math.sqrt(len);
    out[0] = a[0] * len;
    out[1] = a[1] * len;
    out[2] = a[2] * len;
  }
  return out;
}

/**
 * Calculates the dot product of two vec3's
 *
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {Number} dot product of a and b
 */
function dot(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/**
 * Computes the cross product of two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @returns {vec3} out
 */
function cross(out, a, b) {
  let ax = a[0], ay = a[1], az = a[2];
  let bx = b[0], by = b[1], bz = b[2];

  out[0] = ay * bz - az * by;
  out[1] = az * bx - ax * bz;
  out[2] = ax * by - ay * bx;
  return out;
}

/**
 * Performs a linear interpolation between two vec3's
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {vec3} out
 */
function lerp(out, a, b, t) {
  let ax = a[0];
  let ay = a[1];
  let az = a[2];
  out[0] = ax + t * (b[0] - ax);
  out[1] = ay + t * (b[1] - ay);
  out[2] = az + t * (b[2] - az);
  return out;
}

/**
 * Performs a hermite interpolation with two control points
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @param {vec3} c the third operand
 * @param {vec3} d the fourth operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {vec3} out
 */
function hermite(out, a, b, c, d, t) {
  let factorTimes2 = t * t;
  let factor1 = factorTimes2 * (2 * t - 3) + 1;
  let factor2 = factorTimes2 * (t - 2) + t;
  let factor3 = factorTimes2 * (t - 1);
  let factor4 = factorTimes2 * (3 - 2 * t);

  out[0] = a[0] * factor1 + b[0] * factor2 + c[0] * factor3 + d[0] * factor4;
  out[1] = a[1] * factor1 + b[1] * factor2 + c[1] * factor3 + d[1] * factor4;
  out[2] = a[2] * factor1 + b[2] * factor2 + c[2] * factor3 + d[2] * factor4;

  return out;
}

/**
 * Performs a bezier interpolation with two control points
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the first operand
 * @param {vec3} b the second operand
 * @param {vec3} c the third operand
 * @param {vec3} d the fourth operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {vec3} out
 */
function bezier(out, a, b, c, d, t) {
  let inverseFactor = 1 - t;
  let inverseFactorTimesTwo = inverseFactor * inverseFactor;
  let factorTimes2 = t * t;
  let factor1 = inverseFactorTimesTwo * inverseFactor;
  let factor2 = 3 * t * inverseFactorTimesTwo;
  let factor3 = 3 * factorTimes2 * inverseFactor;
  let factor4 = factorTimes2 * t;

  out[0] = a[0] * factor1 + b[0] * factor2 + c[0] * factor3 + d[0] * factor4;
  out[1] = a[1] * factor1 + b[1] * factor2 + c[1] * factor3 + d[1] * factor4;
  out[2] = a[2] * factor1 + b[2] * factor2 + c[2] * factor3 + d[2] * factor4;

  return out;
}

/**
 * Generates a random vector with the given scale
 *
 * @param {vec3} out the receiving vector
 * @param {Number} [scale] Length of the resulting vector. If ommitted, a unit vector will be returned
 * @returns {vec3} out
 */
function random(out, scale) {
  scale = scale || 1.0;

  let r = _common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]() * 2.0 * Math.PI;
  let z = (_common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]() * 2.0) - 1.0;
  let zScale = Math.sqrt(1.0-z*z) * scale;

  out[0] = Math.cos(r) * zScale;
  out[1] = Math.sin(r) * zScale;
  out[2] = z * scale;
  return out;
}

/**
 * Transforms the vec3 with a mat4.
 * 4th vector component is implicitly '1'
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the vector to transform
 * @param {mat4} m matrix to transform with
 * @returns {vec3} out
 */
function transformMat4(out, a, m) {
  let x = a[0], y = a[1], z = a[2];
  let w = m[3] * x + m[7] * y + m[11] * z + m[15];
  w = w || 1.0;
  out[0] = (m[0] * x + m[4] * y + m[8] * z + m[12]) / w;
  out[1] = (m[1] * x + m[5] * y + m[9] * z + m[13]) / w;
  out[2] = (m[2] * x + m[6] * y + m[10] * z + m[14]) / w;
  return out;
}

/**
 * Transforms the vec3 with a mat3.
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the vector to transform
 * @param {mat3} m the 3x3 matrix to transform with
 * @returns {vec3} out
 */
function transformMat3(out, a, m) {
  let x = a[0], y = a[1], z = a[2];
  out[0] = x * m[0] + y * m[3] + z * m[6];
  out[1] = x * m[1] + y * m[4] + z * m[7];
  out[2] = x * m[2] + y * m[5] + z * m[8];
  return out;
}

/**
 * Transforms the vec3 with a quat
 * Can also be used for dual quaternions. (Multiply it with the real part)
 *
 * @param {vec3} out the receiving vector
 * @param {vec3} a the vector to transform
 * @param {quat} q quaternion to transform with
 * @returns {vec3} out
 */
function transformQuat(out, a, q) {
    // benchmarks: https://jsperf.com/quaternion-transform-vec3-implementations-fixed
    let qx = q[0], qy = q[1], qz = q[2], qw = q[3];
    let x = a[0], y = a[1], z = a[2];
    // var qvec = [qx, qy, qz];
    // var uv = vec3.cross([], qvec, a);
    let uvx = qy * z - qz * y,
        uvy = qz * x - qx * z,
        uvz = qx * y - qy * x;
    // var uuv = vec3.cross([], qvec, uv);
    let uuvx = qy * uvz - qz * uvy,
        uuvy = qz * uvx - qx * uvz,
        uuvz = qx * uvy - qy * uvx;
    // vec3.scale(uv, uv, 2 * w);
    let w2 = qw * 2;
    uvx *= w2;
    uvy *= w2;
    uvz *= w2;
    // vec3.scale(uuv, uuv, 2);
    uuvx *= 2;
    uuvy *= 2;
    uuvz *= 2;
    // return vec3.add(out, a, vec3.add(out, uv, uuv));
    out[0] = x + uvx + uuvx;
    out[1] = y + uvy + uuvy;
    out[2] = z + uvz + uuvz;
    return out;
}

/**
 * Rotate a 3D vector around the x-axis
 * @param {vec3} out The receiving vec3
 * @param {vec3} a The vec3 point to rotate
 * @param {vec3} b The origin of the rotation
 * @param {Number} c The angle of rotation
 * @returns {vec3} out
 */
function rotateX(out, a, b, c){
  let p = [], r=[];
  //Translate point to the origin
  p[0] = a[0] - b[0];
  p[1] = a[1] - b[1];
  p[2] = a[2] - b[2];

  //perform rotation
  r[0] = p[0];
  r[1] = p[1]*Math.cos(c) - p[2]*Math.sin(c);
  r[2] = p[1]*Math.sin(c) + p[2]*Math.cos(c);

  //translate to correct position
  out[0] = r[0] + b[0];
  out[1] = r[1] + b[1];
  out[2] = r[2] + b[2];

  return out;
}

/**
 * Rotate a 3D vector around the y-axis
 * @param {vec3} out The receiving vec3
 * @param {vec3} a The vec3 point to rotate
 * @param {vec3} b The origin of the rotation
 * @param {Number} c The angle of rotation
 * @returns {vec3} out
 */
function rotateY(out, a, b, c){
  let p = [], r=[];
  //Translate point to the origin
  p[0] = a[0] - b[0];
  p[1] = a[1] - b[1];
  p[2] = a[2] - b[2];

  //perform rotation
  r[0] = p[2]*Math.sin(c) + p[0]*Math.cos(c);
  r[1] = p[1];
  r[2] = p[2]*Math.cos(c) - p[0]*Math.sin(c);

  //translate to correct position
  out[0] = r[0] + b[0];
  out[1] = r[1] + b[1];
  out[2] = r[2] + b[2];

  return out;
}

/**
 * Rotate a 3D vector around the z-axis
 * @param {vec3} out The receiving vec3
 * @param {vec3} a The vec3 point to rotate
 * @param {vec3} b The origin of the rotation
 * @param {Number} c The angle of rotation
 * @returns {vec3} out
 */
function rotateZ(out, a, b, c){
  let p = [], r=[];
  //Translate point to the origin
  p[0] = a[0] - b[0];
  p[1] = a[1] - b[1];
  p[2] = a[2] - b[2];

  //perform rotation
  r[0] = p[0]*Math.cos(c) - p[1]*Math.sin(c);
  r[1] = p[0]*Math.sin(c) + p[1]*Math.cos(c);
  r[2] = p[2];

  //translate to correct position
  out[0] = r[0] + b[0];
  out[1] = r[1] + b[1];
  out[2] = r[2] + b[2];

  return out;
}

/**
 * Get the angle between two 3D vectors
 * @param {vec3} a The first operand
 * @param {vec3} b The second operand
 * @returns {Number} The angle in radians
 */
function angle(a, b) {
  let tempA = fromValues(a[0], a[1], a[2]);
  let tempB = fromValues(b[0], b[1], b[2]);

  normalize(tempA, tempA);
  normalize(tempB, tempB);

  let cosine = dot(tempA, tempB);

  if(cosine > 1.0) {
    return 0;
  }
  else if(cosine < -1.0) {
    return Math.PI;
  } else {
    return Math.acos(cosine);
  }
}

/**
 * Returns a string representation of a vector
 *
 * @param {vec3} a vector to represent as a string
 * @returns {String} string representation of the vector
 */
function str(a) {
  return 'vec3(' + a[0] + ', ' + a[1] + ', ' + a[2] + ')';
}

/**
 * Returns whether or not the vectors have exactly the same elements in the same position (when compared with ===)
 *
 * @param {vec3} a The first vector.
 * @param {vec3} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2];
}

/**
 * Returns whether or not the vectors have approximately the same elements in the same position.
 *
 * @param {vec3} a The first vector.
 * @param {vec3} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2];
  let b0 = b[0], b1 = b[1], b2 = b[2];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
          Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a2), Math.abs(b2)));
}

/**
 * Alias for {@link vec3.subtract}
 * @function
 */
const sub = subtract;

/**
 * Alias for {@link vec3.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link vec3.divide}
 * @function
 */
const div = divide;

/**
 * Alias for {@link vec3.distance}
 * @function
 */
const dist = distance;

/**
 * Alias for {@link vec3.squaredDistance}
 * @function
 */
const sqrDist = squaredDistance;

/**
 * Alias for {@link vec3.length}
 * @function
 */
const len = length;

/**
 * Alias for {@link vec3.squaredLength}
 * @function
 */
const sqrLen = squaredLength;

/**
 * Perform some operation over an array of vec3s.
 *
 * @param {Array} a the array of vectors to iterate over
 * @param {Number} stride Number of elements between the start of each vec3. If 0 assumes tightly packed
 * @param {Number} offset Number of elements to skip at the beginning of the array
 * @param {Number} count Number of vec3s to iterate over. If 0 iterates over entire array
 * @param {Function} fn Function to call for each vector in the array
 * @param {Object} [arg] additional argument to pass to fn
 * @returns {Array} a
 * @function
 */
const forEach = (function() {
  let vec = create();

  return function(a, stride, offset, count, fn, arg) {
    let i, l;
    if(!stride) {
      stride = 3;
    }

    if(!offset) {
      offset = 0;
    }

    if(count) {
      l = Math.min((count * stride) + offset, a.length);
    } else {
      l = a.length;
    }

    for(i = offset; i < l; i += stride) {
      vec[0] = a[i]; vec[1] = a[i+1]; vec[2] = a[i+2];
      fn(vec, vec, arg);
      a[i] = vec[0]; a[i+1] = vec[1]; a[i+2] = vec[2];
    }

    return a;
  };
})();


/***/ }),

/***/ "./node_modules/gl-matrix/src/gl-matrix/vec4.js":
/*!******************************************************!*\
  !*** ./node_modules/gl-matrix/src/gl-matrix/vec4.js ***!
  \******************************************************/
/*! exports provided: create, clone, fromValues, copy, set, add, subtract, multiply, divide, ceil, floor, min, max, round, scale, scaleAndAdd, distance, squaredDistance, length, squaredLength, negate, inverse, normalize, dot, lerp, random, transformMat4, transformQuat, str, exactEquals, equals, sub, mul, div, dist, sqrDist, len, sqrLen, forEach */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "create", function() { return create; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "clone", function() { return clone; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fromValues", function() { return fromValues; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "copy", function() { return copy; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "set", function() { return set; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "add", function() { return add; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "subtract", function() { return subtract; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "multiply", function() { return multiply; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "divide", function() { return divide; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ceil", function() { return ceil; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "floor", function() { return floor; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "min", function() { return min; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "max", function() { return max; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "round", function() { return round; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scale", function() { return scale; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "scaleAndAdd", function() { return scaleAndAdd; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "distance", function() { return distance; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredDistance", function() { return squaredDistance; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "length", function() { return length; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "squaredLength", function() { return squaredLength; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "negate", function() { return negate; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "inverse", function() { return inverse; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "normalize", function() { return normalize; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dot", function() { return dot; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "lerp", function() { return lerp; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "random", function() { return random; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformMat4", function() { return transformMat4; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "transformQuat", function() { return transformQuat; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "str", function() { return str; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "exactEquals", function() { return exactEquals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "equals", function() { return equals; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sub", function() { return sub; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "mul", function() { return mul; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "div", function() { return div; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "dist", function() { return dist; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrDist", function() { return sqrDist; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "len", function() { return len; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "sqrLen", function() { return sqrLen; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "forEach", function() { return forEach; });
/* harmony import */ var _common_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");
/* Copyright (c) 2015, Brandon Jones, Colin MacKenzie IV.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */



/**
 * 4 Dimensional Vector
 * @module vec4
 */

/**
 * Creates a new, empty vec4
 *
 * @returns {vec4} a new 4D vector
 */
function create() {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  return out;
}

/**
 * Creates a new vec4 initialized with values from an existing vector
 *
 * @param {vec4} a vector to clone
 * @returns {vec4} a new 4D vector
 */
function clone(a) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  return out;
}

/**
 * Creates a new vec4 initialized with the given values
 *
 * @param {Number} x X component
 * @param {Number} y Y component
 * @param {Number} z Z component
 * @param {Number} w W component
 * @returns {vec4} a new 4D vector
 */
function fromValues(x, y, z, w) {
  let out = new _common_js__WEBPACK_IMPORTED_MODULE_0__["ARRAY_TYPE"](4);
  out[0] = x;
  out[1] = y;
  out[2] = z;
  out[3] = w;
  return out;
}

/**
 * Copy the values from one vec4 to another
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the source vector
 * @returns {vec4} out
 */
function copy(out, a) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
  return out;
}

/**
 * Set the components of a vec4 to the given values
 *
 * @param {vec4} out the receiving vector
 * @param {Number} x X component
 * @param {Number} y Y component
 * @param {Number} z Z component
 * @param {Number} w W component
 * @returns {vec4} out
 */
function set(out, x, y, z, w) {
  out[0] = x;
  out[1] = y;
  out[2] = z;
  out[3] = w;
  return out;
}

/**
 * Adds two vec4's
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {vec4} out
 */
function add(out, a, b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  out[3] = a[3] + b[3];
  return out;
}

/**
 * Subtracts vector b from vector a
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {vec4} out
 */
function subtract(out, a, b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  out[3] = a[3] - b[3];
  return out;
}

/**
 * Multiplies two vec4's
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {vec4} out
 */
function multiply(out, a, b) {
  out[0] = a[0] * b[0];
  out[1] = a[1] * b[1];
  out[2] = a[2] * b[2];
  out[3] = a[3] * b[3];
  return out;
}

/**
 * Divides two vec4's
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {vec4} out
 */
function divide(out, a, b) {
  out[0] = a[0] / b[0];
  out[1] = a[1] / b[1];
  out[2] = a[2] / b[2];
  out[3] = a[3] / b[3];
  return out;
}

/**
 * Math.ceil the components of a vec4
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a vector to ceil
 * @returns {vec4} out
 */
function ceil(out, a) {
  out[0] = Math.ceil(a[0]);
  out[1] = Math.ceil(a[1]);
  out[2] = Math.ceil(a[2]);
  out[3] = Math.ceil(a[3]);
  return out;
}

/**
 * Math.floor the components of a vec4
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a vector to floor
 * @returns {vec4} out
 */
function floor(out, a) {
  out[0] = Math.floor(a[0]);
  out[1] = Math.floor(a[1]);
  out[2] = Math.floor(a[2]);
  out[3] = Math.floor(a[3]);
  return out;
}

/**
 * Returns the minimum of two vec4's
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {vec4} out
 */
function min(out, a, b) {
  out[0] = Math.min(a[0], b[0]);
  out[1] = Math.min(a[1], b[1]);
  out[2] = Math.min(a[2], b[2]);
  out[3] = Math.min(a[3], b[3]);
  return out;
}

/**
 * Returns the maximum of two vec4's
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {vec4} out
 */
function max(out, a, b) {
  out[0] = Math.max(a[0], b[0]);
  out[1] = Math.max(a[1], b[1]);
  out[2] = Math.max(a[2], b[2]);
  out[3] = Math.max(a[3], b[3]);
  return out;
}

/**
 * Math.round the components of a vec4
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a vector to round
 * @returns {vec4} out
 */
function round(out, a) {
  out[0] = Math.round(a[0]);
  out[1] = Math.round(a[1]);
  out[2] = Math.round(a[2]);
  out[3] = Math.round(a[3]);
  return out;
}

/**
 * Scales a vec4 by a scalar number
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the vector to scale
 * @param {Number} b amount to scale the vector by
 * @returns {vec4} out
 */
function scale(out, a, b) {
  out[0] = a[0] * b;
  out[1] = a[1] * b;
  out[2] = a[2] * b;
  out[3] = a[3] * b;
  return out;
}

/**
 * Adds two vec4's after scaling the second operand by a scalar value
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @param {Number} scale the amount to scale b by before adding
 * @returns {vec4} out
 */
function scaleAndAdd(out, a, b, scale) {
  out[0] = a[0] + (b[0] * scale);
  out[1] = a[1] + (b[1] * scale);
  out[2] = a[2] + (b[2] * scale);
  out[3] = a[3] + (b[3] * scale);
  return out;
}

/**
 * Calculates the euclidian distance between two vec4's
 *
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {Number} distance between a and b
 */
function distance(a, b) {
  let x = b[0] - a[0];
  let y = b[1] - a[1];
  let z = b[2] - a[2];
  let w = b[3] - a[3];
  return Math.sqrt(x*x + y*y + z*z + w*w);
}

/**
 * Calculates the squared euclidian distance between two vec4's
 *
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {Number} squared distance between a and b
 */
function squaredDistance(a, b) {
  let x = b[0] - a[0];
  let y = b[1] - a[1];
  let z = b[2] - a[2];
  let w = b[3] - a[3];
  return x*x + y*y + z*z + w*w;
}

/**
 * Calculates the length of a vec4
 *
 * @param {vec4} a vector to calculate length of
 * @returns {Number} length of a
 */
function length(a) {
  let x = a[0];
  let y = a[1];
  let z = a[2];
  let w = a[3];
  return Math.sqrt(x*x + y*y + z*z + w*w);
}

/**
 * Calculates the squared length of a vec4
 *
 * @param {vec4} a vector to calculate squared length of
 * @returns {Number} squared length of a
 */
function squaredLength(a) {
  let x = a[0];
  let y = a[1];
  let z = a[2];
  let w = a[3];
  return x*x + y*y + z*z + w*w;
}

/**
 * Negates the components of a vec4
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a vector to negate
 * @returns {vec4} out
 */
function negate(out, a) {
  out[0] = -a[0];
  out[1] = -a[1];
  out[2] = -a[2];
  out[3] = -a[3];
  return out;
}

/**
 * Returns the inverse of the components of a vec4
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a vector to invert
 * @returns {vec4} out
 */
function inverse(out, a) {
  out[0] = 1.0 / a[0];
  out[1] = 1.0 / a[1];
  out[2] = 1.0 / a[2];
  out[3] = 1.0 / a[3];
  return out;
}

/**
 * Normalize a vec4
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a vector to normalize
 * @returns {vec4} out
 */
function normalize(out, a) {
  let x = a[0];
  let y = a[1];
  let z = a[2];
  let w = a[3];
  let len = x*x + y*y + z*z + w*w;
  if (len > 0) {
    len = 1 / Math.sqrt(len);
    out[0] = x * len;
    out[1] = y * len;
    out[2] = z * len;
    out[3] = w * len;
  }
  return out;
}

/**
 * Calculates the dot product of two vec4's
 *
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @returns {Number} dot product of a and b
 */
function dot(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

/**
 * Performs a linear interpolation between two vec4's
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the first operand
 * @param {vec4} b the second operand
 * @param {Number} t interpolation amount between the two inputs
 * @returns {vec4} out
 */
function lerp(out, a, b, t) {
  let ax = a[0];
  let ay = a[1];
  let az = a[2];
  let aw = a[3];
  out[0] = ax + t * (b[0] - ax);
  out[1] = ay + t * (b[1] - ay);
  out[2] = az + t * (b[2] - az);
  out[3] = aw + t * (b[3] - aw);
  return out;
}

/**
 * Generates a random vector with the given scale
 *
 * @param {vec4} out the receiving vector
 * @param {Number} [scale] Length of the resulting vector. If ommitted, a unit vector will be returned
 * @returns {vec4} out
 */
function random(out, vectorScale) {
  vectorScale = vectorScale || 1.0;

  //TODO: This is a pretty awful way of doing this. Find something better.
  out[0] = _common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]();
  out[1] = _common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]();
  out[2] = _common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]();
  out[3] = _common_js__WEBPACK_IMPORTED_MODULE_0__["RANDOM"]();
  normalize(out, out);
  scale(out, out, vectorScale);
  return out;
}

/**
 * Transforms the vec4 with a mat4.
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the vector to transform
 * @param {mat4} m matrix to transform with
 * @returns {vec4} out
 */
function transformMat4(out, a, m) {
  let x = a[0], y = a[1], z = a[2], w = a[3];
  out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
  out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
  out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
  out[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
  return out;
}

/**
 * Transforms the vec4 with a quat
 *
 * @param {vec4} out the receiving vector
 * @param {vec4} a the vector to transform
 * @param {quat} q quaternion to transform with
 * @returns {vec4} out
 */
function transformQuat(out, a, q) {
  let x = a[0], y = a[1], z = a[2];
  let qx = q[0], qy = q[1], qz = q[2], qw = q[3];

  // calculate quat * vec
  let ix = qw * x + qy * z - qz * y;
  let iy = qw * y + qz * x - qx * z;
  let iz = qw * z + qx * y - qy * x;
  let iw = -qx * x - qy * y - qz * z;

  // calculate result * inverse quat
  out[0] = ix * qw + iw * -qx + iy * -qz - iz * -qy;
  out[1] = iy * qw + iw * -qy + iz * -qx - ix * -qz;
  out[2] = iz * qw + iw * -qz + ix * -qy - iy * -qx;
  out[3] = a[3];
  return out;
}

/**
 * Returns a string representation of a vector
 *
 * @param {vec4} a vector to represent as a string
 * @returns {String} string representation of the vector
 */
function str(a) {
  return 'vec4(' + a[0] + ', ' + a[1] + ', ' + a[2] + ', ' + a[3] + ')';
}

/**
 * Returns whether or not the vectors have exactly the same elements in the same position (when compared with ===)
 *
 * @param {vec4} a The first vector.
 * @param {vec4} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
function exactEquals(a, b) {
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] && a[3] === b[3];
}

/**
 * Returns whether or not the vectors have approximately the same elements in the same position.
 *
 * @param {vec4} a The first vector.
 * @param {vec4} b The second vector.
 * @returns {Boolean} True if the vectors are equal, false otherwise.
 */
function equals(a, b) {
  let a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
  let b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
  return (Math.abs(a0 - b0) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a0), Math.abs(b0)) &&
          Math.abs(a1 - b1) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a1), Math.abs(b1)) &&
          Math.abs(a2 - b2) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a2), Math.abs(b2)) &&
          Math.abs(a3 - b3) <= _common_js__WEBPACK_IMPORTED_MODULE_0__["EPSILON"]*Math.max(1.0, Math.abs(a3), Math.abs(b3)));
}

/**
 * Alias for {@link vec4.subtract}
 * @function
 */
const sub = subtract;

/**
 * Alias for {@link vec4.multiply}
 * @function
 */
const mul = multiply;

/**
 * Alias for {@link vec4.divide}
 * @function
 */
const div = divide;

/**
 * Alias for {@link vec4.distance}
 * @function
 */
const dist = distance;

/**
 * Alias for {@link vec4.squaredDistance}
 * @function
 */
const sqrDist = squaredDistance;

/**
 * Alias for {@link vec4.length}
 * @function
 */
const len = length;

/**
 * Alias for {@link vec4.squaredLength}
 * @function
 */
const sqrLen = squaredLength;

/**
 * Perform some operation over an array of vec4s.
 *
 * @param {Array} a the array of vectors to iterate over
 * @param {Number} stride Number of elements between the start of each vec4. If 0 assumes tightly packed
 * @param {Number} offset Number of elements to skip at the beginning of the array
 * @param {Number} count Number of vec4s to iterate over. If 0 iterates over entire array
 * @param {Function} fn Function to call for each vector in the array
 * @param {Object} [arg] additional argument to pass to fn
 * @returns {Array} a
 * @function
 */
const forEach = (function() {
  let vec = create();

  return function(a, stride, offset, count, fn, arg) {
    let i, l;
    if(!stride) {
      stride = 4;
    }

    if(!offset) {
      offset = 0;
    }

    if(count) {
      l = Math.min((count * stride) + offset, a.length);
    } else {
      l = a.length;
    }

    for(i = offset; i < l; i += stride) {
      vec[0] = a[i]; vec[1] = a[i+1]; vec[2] = a[i+2]; vec[3] = a[i+3];
      fn(vec, vec, arg);
      a[i] = vec[0]; a[i+1] = vec[1]; a[i+2] = vec[2]; a[i+3] = vec[3];
    }

    return a;
  };
})();


/***/ }),

/***/ "./src/core/material.js":
/*!******************************!*\
  !*** ./src/core/material.js ***!
  \******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

exports.stateToBlendFunc = stateToBlendFunc;

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

// Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var GL = WebGLRenderingContext; // For enums

var CAP = exports.CAP = {
  // Enable caps
  CULL_FACE: 0x001,
  BLEND: 0x002,
  DEPTH_TEST: 0x004,
  STENCIL_TEST: 0x008,
  COLOR_MASK: 0x010,
  DEPTH_MASK: 0x020,
  STENCIL_MASK: 0x040
};

var MAT_STATE = exports.MAT_STATE = {
  CAPS_RANGE: 0x000000FF,
  BLEND_SRC_SHIFT: 8,
  BLEND_SRC_RANGE: 0x00000F00,
  BLEND_DST_SHIFT: 12,
  BLEND_DST_RANGE: 0x0000F000,
  BLEND_FUNC_RANGE: 0x0000FF00,
  DEPTH_FUNC_SHIFT: 16,
  DEPTH_FUNC_RANGE: 0x000F0000
};

var RENDER_ORDER = exports.RENDER_ORDER = {
  // Render opaque objects first.
  OPAQUE: 0,

  // Render the sky after all opaque object to save fill rate.
  SKY: 1,

  // Render transparent objects next so that the opaqe objects show through.
  TRANSPARENT: 2,

  // Finally render purely additive effects like pointer rays so that they
  // can render without depth mask.
  ADDITIVE: 3,

  // Render order will be picked based on the material properties.
  DEFAULT: 4
};

function stateToBlendFunc(state, mask, shift) {
  var value = (state & mask) >> shift;
  switch (value) {
    case 0:
    case 1:
      return value;
    default:
      return value - 2 + GL.SRC_COLOR;
  }
}

var MaterialState = exports.MaterialState = function () {
  function MaterialState() {
    _classCallCheck(this, MaterialState);

    this._state = CAP.CULL_FACE | CAP.DEPTH_TEST | CAP.COLOR_MASK | CAP.DEPTH_MASK;

    // Use a fairly commonly desired blend func as the default.
    this.blendFuncSrc = GL.SRC_ALPHA;
    this.blendFuncDst = GL.ONE_MINUS_SRC_ALPHA;

    this.depthFunc = GL.LESS;
  }

  _createClass(MaterialState, [{
    key: "cullFace",
    get: function get() {
      return !!(this._state & CAP.CULL_FACE);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.CULL_FACE;
      } else {
        this._state &= ~CAP.CULL_FACE;
      }
    }
  }, {
    key: "blend",
    get: function get() {
      return !!(this._state & CAP.BLEND);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.BLEND;
      } else {
        this._state &= ~CAP.BLEND;
      }
    }
  }, {
    key: "depthTest",
    get: function get() {
      return !!(this._state & CAP.DEPTH_TEST);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.DEPTH_TEST;
      } else {
        this._state &= ~CAP.DEPTH_TEST;
      }
    }
  }, {
    key: "stencilTest",
    get: function get() {
      return !!(this._state & CAP.STENCIL_TEST);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.STENCIL_TEST;
      } else {
        this._state &= ~CAP.STENCIL_TEST;
      }
    }
  }, {
    key: "colorMask",
    get: function get() {
      return !!(this._state & CAP.COLOR_MASK);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.COLOR_MASK;
      } else {
        this._state &= ~CAP.COLOR_MASK;
      }
    }
  }, {
    key: "depthMask",
    get: function get() {
      return !!(this._state & CAP.DEPTH_MASK);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.DEPTH_MASK;
      } else {
        this._state &= ~CAP.DEPTH_MASK;
      }
    }
  }, {
    key: "depthFunc",
    get: function get() {
      return ((this._state & MAT_STATE.DEPTH_FUNC_RANGE) >> MAT_STATE.DEPTH_FUNC_SHIFT) + GL.NEVER;
    },
    set: function set(value) {
      value = value - GL.NEVER;
      this._state &= ~MAT_STATE.DEPTH_FUNC_RANGE;
      this._state |= value << MAT_STATE.DEPTH_FUNC_SHIFT;
    }
  }, {
    key: "stencilMask",
    get: function get() {
      return !!(this._state & CAP.STENCIL_MASK);
    },
    set: function set(value) {
      if (value) {
        this._state |= CAP.STENCIL_MASK;
      } else {
        this._state &= ~CAP.STENCIL_MASK;
      }
    }
  }, {
    key: "blendFuncSrc",
    get: function get() {
      return stateToBlendFunc(this._state, MAT_STATE.BLEND_SRC_RANGE, MAT_STATE.BLEND_SRC_SHIFT);
    },
    set: function set(value) {
      switch (value) {
        case 0:
        case 1:
          break;
        default:
          value = value - GL.SRC_COLOR + 2;
      }
      this._state &= ~MAT_STATE.BLEND_SRC_RANGE;
      this._state |= value << MAT_STATE.BLEND_SRC_SHIFT;
    }
  }, {
    key: "blendFuncDst",
    get: function get() {
      return stateToBlendFunc(this._state, MAT_STATE.BLEND_DST_RANGE, MAT_STATE.BLEND_DST_SHIFT);
    },
    set: function set(value) {
      switch (value) {
        case 0:
        case 1:
          break;
        default:
          value = value - GL.SRC_COLOR + 2;
      }
      this._state &= ~MAT_STATE.BLEND_DST_RANGE;
      this._state |= value << MAT_STATE.BLEND_DST_SHIFT;
    }
  }]);

  return MaterialState;
}();

var MaterialSampler = function () {
  function MaterialSampler(uniformName) {
    _classCallCheck(this, MaterialSampler);

    this._uniformName = uniformName;
    this._texture = null;
  }

  _createClass(MaterialSampler, [{
    key: "texture",
    get: function get() {
      return this._texture;
    },
    set: function set(value) {
      this._texture = value;
    }
  }]);

  return MaterialSampler;
}();

var MaterialUniform = function () {
  function MaterialUniform(uniformName, defaultValue, length) {
    _classCallCheck(this, MaterialUniform);

    this._uniformName = uniformName;
    this._value = defaultValue;
    this._length = length;
    if (!this._length) {
      if (defaultValue instanceof Array) {
        this._length = defaultValue.length;
      } else {
        this._length = 1;
      }
    }
  }

  _createClass(MaterialUniform, [{
    key: "value",
    get: function get() {
      return this._value;
    },
    set: function set(value) {
      this._value = value;
    }
  }]);

  return MaterialUniform;
}();

var Material = exports.Material = function () {
  function Material() {
    _classCallCheck(this, Material);

    this.state = new MaterialState();
    this.renderOrder = RENDER_ORDER.DEFAULT;
    this._samplers = [];
    this._uniforms = [];
  }

  _createClass(Material, [{
    key: "defineSampler",
    value: function defineSampler(uniformName) {
      var sampler = new MaterialSampler(uniformName);
      this._samplers.push(sampler);
      return sampler;
    }
  }, {
    key: "defineUniform",
    value: function defineUniform(uniformName) {
      var defaultValue = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : null;
      var length = arguments.length > 2 && arguments[2] !== undefined ? arguments[2] : 0;

      var uniform = new MaterialUniform(uniformName, defaultValue, length);
      this._uniforms.push(uniform);
      return uniform;
    }
  }, {
    key: "getProgramDefines",
    value: function getProgramDefines(renderPrimitive) {
      return {};
    }
  }, {
    key: "materialName",
    get: function get() {
      return null;
    }
  }, {
    key: "vertexSource",
    get: function get() {
      return null;
    }
  }, {
    key: "fragmentSource",
    get: function get() {
      return null;
    }
  }]);

  return Material;
}();

/***/ }),

/***/ "./src/core/node.js":
/*!**************************!*\
  !*** ./src/core/node.js ***!
  \**************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Node = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }(); // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var _ray = __webpack_require__(/*! ./ray.js */ "./src/core/ray.js");

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var DEFAULT_TRANSLATION = new Float32Array([0, 0, 0]);
var DEFAULT_ROTATION = new Float32Array([0, 0, 0, 1]);
var DEFAULT_SCALE = new Float32Array([1, 1, 1]);

var tmpRayMatrix = _glMatrix.mat4.create();
var tmpRayOrigin = _glMatrix.vec3.create();

var Node = exports.Node = function () {
  function Node() {
    _classCallCheck(this, Node);

    this.name = null; // Only for debugging
    this.children = [];
    this.parent = null;
    this.visible = true;
    this.selectable = false;

    this._matrix = null;

    this._dirtyTRS = false;
    this._translation = null;
    this._rotation = null;
    this._scale = null;

    this._dirtyWorldMatrix = false;
    this._worldMatrix = null;

    this._activeFrameId = -1;
    this._hoverFrameId = -1;
    this._renderPrimitives = null;
    this._renderer = null;

    this._selectHandler = null;
  }

  _createClass(Node, [{
    key: '_setRenderer',
    value: function _setRenderer(renderer) {
      if (this._renderer == renderer) {
        return;
      }

      if (this._renderer) {
        // Changing the renderer removes any previously attached renderPrimitives
        // from a different renderer.
        this.clearRenderPrimitives();
      }

      this._renderer = renderer;
      if (renderer) {
        this.onRendererChanged(renderer);

        var _iteratorNormalCompletion = true;
        var _didIteratorError = false;
        var _iteratorError = undefined;

        try {
          for (var _iterator = this.children[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
            var child = _step.value;

            child._setRenderer(renderer);
          }
        } catch (err) {
          _didIteratorError = true;
          _iteratorError = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion && _iterator.return) {
              _iterator.return();
            }
          } finally {
            if (_didIteratorError) {
              throw _iteratorError;
            }
          }
        }
      }
    }
  }, {
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {}
    // Override in other node types to respond to changes in the renderer.


    // Create a clone of this node and all of it's children. Does not duplicate
    // RenderPrimitives, the cloned nodes will be treated as new instances of the
    // geometry.

  }, {
    key: 'clone',
    value: function clone() {
      var _this = this;

      var cloneNode = new Node();
      cloneNode.name = this.name;
      cloneNode.visible = this.visible;
      cloneNode._renderer = this._renderer;

      cloneNode._dirtyTRS = this._dirtyTRS;

      if (this._translation) {
        cloneNode._translation = _glMatrix.vec3.create();
        _glMatrix.vec3.copy(cloneNode._translation, this._translation);
      }

      if (this._rotation) {
        cloneNode._rotation = _glMatrix.quat.create();
        _glMatrix.quat.copy(cloneNode._rotation, this._rotation);
      }

      if (this._scale) {
        cloneNode._scale = _glMatrix.vec3.create();
        _glMatrix.vec3.copy(cloneNode._scale, this._scale);
      }

      // Only copy the matrices if they're not already dirty.
      if (!cloneNode._dirtyTRS && this._matrix) {
        cloneNode._matrix = _glMatrix.mat4.create();
        _glMatrix.mat4.copy(cloneNode._matrix, this._matrix);
      }

      cloneNode._dirtyWorldMatrix = this._dirtyWorldMatrix;
      if (!cloneNode._dirtyWorldMatrix && this._worldMatrix) {
        cloneNode._worldMatrix = _glMatrix.mat4.create();
        _glMatrix.mat4.copy(cloneNode._worldMatrix, this._worldMatrix);
      }

      this.waitForComplete().then(function () {
        if (_this._renderPrimitives) {
          var _iteratorNormalCompletion2 = true;
          var _didIteratorError2 = false;
          var _iteratorError2 = undefined;

          try {
            for (var _iterator2 = _this._renderPrimitives[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
              var primitive = _step2.value;

              cloneNode.addRenderPrimitive(primitive);
            }
          } catch (err) {
            _didIteratorError2 = true;
            _iteratorError2 = err;
          } finally {
            try {
              if (!_iteratorNormalCompletion2 && _iterator2.return) {
                _iterator2.return();
              }
            } finally {
              if (_didIteratorError2) {
                throw _iteratorError2;
              }
            }
          }
        }

        var _iteratorNormalCompletion3 = true;
        var _didIteratorError3 = false;
        var _iteratorError3 = undefined;

        try {
          for (var _iterator3 = _this.children[Symbol.iterator](), _step3; !(_iteratorNormalCompletion3 = (_step3 = _iterator3.next()).done); _iteratorNormalCompletion3 = true) {
            var child = _step3.value;

            cloneNode.addNode(child.clone());
          }
        } catch (err) {
          _didIteratorError3 = true;
          _iteratorError3 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion3 && _iterator3.return) {
              _iterator3.return();
            }
          } finally {
            if (_didIteratorError3) {
              throw _iteratorError3;
            }
          }
        }
      });

      return cloneNode;
    }
  }, {
    key: 'markActive',
    value: function markActive(frameId) {
      if (this.visible && this._renderPrimitives) {
        this._activeFrameId = frameId;
        var _iteratorNormalCompletion4 = true;
        var _didIteratorError4 = false;
        var _iteratorError4 = undefined;

        try {
          for (var _iterator4 = this._renderPrimitives[Symbol.iterator](), _step4; !(_iteratorNormalCompletion4 = (_step4 = _iterator4.next()).done); _iteratorNormalCompletion4 = true) {
            var primitive = _step4.value;

            primitive.markActive(frameId);
          }
        } catch (err) {
          _didIteratorError4 = true;
          _iteratorError4 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion4 && _iterator4.return) {
              _iterator4.return();
            }
          } finally {
            if (_didIteratorError4) {
              throw _iteratorError4;
            }
          }
        }
      }

      var _iteratorNormalCompletion5 = true;
      var _didIteratorError5 = false;
      var _iteratorError5 = undefined;

      try {
        for (var _iterator5 = this.children[Symbol.iterator](), _step5; !(_iteratorNormalCompletion5 = (_step5 = _iterator5.next()).done); _iteratorNormalCompletion5 = true) {
          var child = _step5.value;

          if (child.visible) {
            child.markActive(frameId);
          }
        }
      } catch (err) {
        _didIteratorError5 = true;
        _iteratorError5 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion5 && _iterator5.return) {
            _iterator5.return();
          }
        } finally {
          if (_didIteratorError5) {
            throw _iteratorError5;
          }
        }
      }
    }
  }, {
    key: 'addNode',
    value: function addNode(value) {
      if (!value || value.parent == this) {
        return;
      }

      if (value.parent) {
        value.parent.removeNode(value);
      }
      value.parent = this;

      this.children.push(value);

      if (this._renderer) {
        value._setRenderer(this._renderer);
      }
    }
  }, {
    key: 'removeNode',
    value: function removeNode(value) {
      var i = this.children.indexOf(value);
      if (i > -1) {
        this.children.splice(i, 1);
        value.parent = null;
      }
    }
  }, {
    key: 'clearNodes',
    value: function clearNodes() {
      var _iteratorNormalCompletion6 = true;
      var _didIteratorError6 = false;
      var _iteratorError6 = undefined;

      try {
        for (var _iterator6 = this.children[Symbol.iterator](), _step6; !(_iteratorNormalCompletion6 = (_step6 = _iterator6.next()).done); _iteratorNormalCompletion6 = true) {
          var child = _step6.value;

          child.parent = null;
        }
      } catch (err) {
        _didIteratorError6 = true;
        _iteratorError6 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion6 && _iterator6.return) {
            _iterator6.return();
          }
        } finally {
          if (_didIteratorError6) {
            throw _iteratorError6;
          }
        }
      }

      this.children = [];
    }
  }, {
    key: 'setMatrixDirty',
    value: function setMatrixDirty() {
      if (!this._dirtyWorldMatrix) {
        this._dirtyWorldMatrix = true;
        var _iteratorNormalCompletion7 = true;
        var _didIteratorError7 = false;
        var _iteratorError7 = undefined;

        try {
          for (var _iterator7 = this.children[Symbol.iterator](), _step7; !(_iteratorNormalCompletion7 = (_step7 = _iterator7.next()).done); _iteratorNormalCompletion7 = true) {
            var child = _step7.value;

            child.setMatrixDirty();
          }
        } catch (err) {
          _didIteratorError7 = true;
          _iteratorError7 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion7 && _iterator7.return) {
              _iterator7.return();
            }
          } finally {
            if (_didIteratorError7) {
              throw _iteratorError7;
            }
          }
        }
      }
    }
  }, {
    key: '_updateLocalMatrix',
    value: function _updateLocalMatrix() {
      if (!this._matrix) {
        this._matrix = _glMatrix.mat4.create();
      }

      if (this._dirtyTRS) {
        this._dirtyTRS = false;
        _glMatrix.mat4.fromRotationTranslationScale(this._matrix, this._rotation || DEFAULT_ROTATION, this._translation || DEFAULT_TRANSLATION, this._scale || DEFAULT_SCALE);
      }

      return this._matrix;
    }
  }, {
    key: 'waitForComplete',
    value: function waitForComplete() {
      var _this2 = this;

      var childPromises = [];
      var _iteratorNormalCompletion8 = true;
      var _didIteratorError8 = false;
      var _iteratorError8 = undefined;

      try {
        for (var _iterator8 = this.children[Symbol.iterator](), _step8; !(_iteratorNormalCompletion8 = (_step8 = _iterator8.next()).done); _iteratorNormalCompletion8 = true) {
          var child = _step8.value;

          childPromises.push(child.waitForComplete());
        }
      } catch (err) {
        _didIteratorError8 = true;
        _iteratorError8 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion8 && _iterator8.return) {
            _iterator8.return();
          }
        } finally {
          if (_didIteratorError8) {
            throw _iteratorError8;
          }
        }
      }

      if (this._renderPrimitives) {
        var _iteratorNormalCompletion9 = true;
        var _didIteratorError9 = false;
        var _iteratorError9 = undefined;

        try {
          for (var _iterator9 = this._renderPrimitives[Symbol.iterator](), _step9; !(_iteratorNormalCompletion9 = (_step9 = _iterator9.next()).done); _iteratorNormalCompletion9 = true) {
            var primitive = _step9.value;

            childPromises.push(primitive.waitForComplete());
          }
        } catch (err) {
          _didIteratorError9 = true;
          _iteratorError9 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion9 && _iterator9.return) {
              _iterator9.return();
            }
          } finally {
            if (_didIteratorError9) {
              throw _iteratorError9;
            }
          }
        }
      }
      return Promise.all(childPromises).then(function () {
        return _this2;
      });
    }
  }, {
    key: 'addRenderPrimitive',
    value: function addRenderPrimitive(primitive) {
      if (!this._renderPrimitives) {
        this._renderPrimitives = [primitive];
      } else {
        this._renderPrimitives.push(primitive);
      }
      primitive._instances.push(this);
    }
  }, {
    key: 'removeRenderPrimitive',
    value: function removeRenderPrimitive(primitive) {
      if (!this._renderPrimitives) {
        return;
      }

      var index = this._renderPrimitives._instances.indexOf(primitive);
      if (index > -1) {
        this._renderPrimitives._instances.splice(index, 1);

        index = primitive._instances.indexOf(this);
        if (index > -1) {
          primitive._instances.splice(index, 1);
        }

        if (!this._renderPrimitives.length) {
          this._renderPrimitives = null;
        }
      }
    }
  }, {
    key: 'clearRenderPrimitives',
    value: function clearRenderPrimitives() {
      if (this._renderPrimitives) {
        var _iteratorNormalCompletion10 = true;
        var _didIteratorError10 = false;
        var _iteratorError10 = undefined;

        try {
          for (var _iterator10 = this._renderPrimitives[Symbol.iterator](), _step10; !(_iteratorNormalCompletion10 = (_step10 = _iterator10.next()).done); _iteratorNormalCompletion10 = true) {
            var primitive = _step10.value;

            var index = primitive._instances.indexOf(this);
            if (index > -1) {
              primitive._instances.splice(index, 1);
            }
          }
        } catch (err) {
          _didIteratorError10 = true;
          _iteratorError10 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion10 && _iterator10.return) {
              _iterator10.return();
            }
          } finally {
            if (_didIteratorError10) {
              throw _iteratorError10;
            }
          }
        }

        this._renderPrimitives = null;
      }
    }
  }, {
    key: '_hitTestSelectableNode',
    value: function _hitTestSelectableNode(rayMatrix) {
      if (this._renderPrimitives) {
        var ray = null;
        var _iteratorNormalCompletion11 = true;
        var _didIteratorError11 = false;
        var _iteratorError11 = undefined;

        try {
          for (var _iterator11 = this._renderPrimitives[Symbol.iterator](), _step11; !(_iteratorNormalCompletion11 = (_step11 = _iterator11.next()).done); _iteratorNormalCompletion11 = true) {
            var primitive = _step11.value;

            if (primitive._min) {
              if (!ray) {
                _glMatrix.mat4.invert(tmpRayMatrix, this.worldMatrix);
                _glMatrix.mat4.multiply(tmpRayMatrix, tmpRayMatrix, rayMatrix);
                ray = new _ray.Ray(tmpRayMatrix);
              }
              var intersection = ray.intersectsAABB(primitive._min, primitive._max);
              if (intersection) {
                _glMatrix.vec3.transformMat4(intersection, intersection, this.worldMatrix);
                return intersection;
              }
            }
          }
        } catch (err) {
          _didIteratorError11 = true;
          _iteratorError11 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion11 && _iterator11.return) {
              _iterator11.return();
            }
          } finally {
            if (_didIteratorError11) {
              throw _iteratorError11;
            }
          }
        }
      }
      var _iteratorNormalCompletion12 = true;
      var _didIteratorError12 = false;
      var _iteratorError12 = undefined;

      try {
        for (var _iterator12 = this.children[Symbol.iterator](), _step12; !(_iteratorNormalCompletion12 = (_step12 = _iterator12.next()).done); _iteratorNormalCompletion12 = true) {
          var child = _step12.value;

          var _intersection = child._hitTestSelectableNode(rayMatrix);
          if (_intersection) {
            return _intersection;
          }
        }
      } catch (err) {
        _didIteratorError12 = true;
        _iteratorError12 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion12 && _iterator12.return) {
            _iterator12.return();
          }
        } finally {
          if (_didIteratorError12) {
            throw _iteratorError12;
          }
        }
      }

      return null;
    }
  }, {
    key: 'hitTest',
    value: function hitTest(rayMatrix) {
      var rayOrigin = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : null;

      if (!rayOrigin) {
        rayOrigin = tmpRayOrigin;
        _glMatrix.vec3.set(rayOrigin, 0, 0, 0);
        _glMatrix.vec3.transformMat4(rayOrigin, rayOrigin, rayMatrix);
      }

      if (this.selectable && this.visible) {
        var intersection = this._hitTestSelectableNode(rayMatrix);
        if (intersection) {
          return {
            node: this,
            intersection: intersection,
            distance: _glMatrix.vec3.distance(rayOrigin, intersection)
          };
        }
        return null;
      }

      var result = null;
      var _iteratorNormalCompletion13 = true;
      var _didIteratorError13 = false;
      var _iteratorError13 = undefined;

      try {
        for (var _iterator13 = this.children[Symbol.iterator](), _step13; !(_iteratorNormalCompletion13 = (_step13 = _iterator13.next()).done); _iteratorNormalCompletion13 = true) {
          var child = _step13.value;

          var childResult = child.hitTest(rayMatrix, rayOrigin);
          if (childResult) {
            if (!result || result.distance > childResult.distance) {
              result = childResult;
            }
          }
        }
      } catch (err) {
        _didIteratorError13 = true;
        _iteratorError13 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion13 && _iterator13.return) {
            _iterator13.return();
          }
        } finally {
          if (_didIteratorError13) {
            throw _iteratorError13;
          }
        }
      }

      return result;
    }
  }, {
    key: 'onSelect',
    value: function onSelect(value) {
      this._selectHandler = value;
    }
  }, {
    key: 'handleSelect',


    // Called when a selectable node is selected.
    value: function handleSelect() {
      if (this._selectHandler) {
        this._selectHandler();
      }
    }

    // Called when a selectable element is pointed at.

  }, {
    key: 'onHoverStart',
    value: function onHoverStart() {}

    // Called when a selectable element is no longer pointed at.

  }, {
    key: 'onHoverEnd',
    value: function onHoverEnd() {}
  }, {
    key: '_update',
    value: function _update(timestamp, frameDelta) {
      this.onUpdate(timestamp, frameDelta);

      var _iteratorNormalCompletion14 = true;
      var _didIteratorError14 = false;
      var _iteratorError14 = undefined;

      try {
        for (var _iterator14 = this.children[Symbol.iterator](), _step14; !(_iteratorNormalCompletion14 = (_step14 = _iterator14.next()).done); _iteratorNormalCompletion14 = true) {
          var child = _step14.value;

          child._update(timestamp, frameDelta);
        }
      } catch (err) {
        _didIteratorError14 = true;
        _iteratorError14 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion14 && _iterator14.return) {
            _iterator14.return();
          }
        } finally {
          if (_didIteratorError14) {
            throw _iteratorError14;
          }
        }
      }
    }

    // Called every frame so that the nodes can animate themselves

  }, {
    key: 'onUpdate',
    value: function onUpdate(timestamp, frameDelta) {}
  }, {
    key: 'matrix',
    set: function set(value) {
      this._matrix = value;
      this.setMatrixDirty();
      this._dirtyTRS = false;
      this._translation = null;
      this._rotation = null;
      this._scale = null;
    },
    get: function get() {
      this.setMatrixDirty();

      return this._updateLocalMatrix();
    }
  }, {
    key: 'worldMatrix',
    get: function get() {
      if (!this._worldMatrix) {
        this._dirtyWorldMatrix = true;
        this._worldMatrix = _glMatrix.mat4.create();
      }

      if (this._dirtyWorldMatrix || this._dirtyTRS) {
        if (this.parent) {
          // TODO: Some optimizations that could be done here if the node matrix
          // is an identity matrix.
          _glMatrix.mat4.mul(this._worldMatrix, this.parent.worldMatrix, this._updateLocalMatrix());
        } else {
          _glMatrix.mat4.copy(this._worldMatrix, this._updateLocalMatrix());
        }
        this._dirtyWorldMatrix = false;
      }

      return this._worldMatrix;
    }

    // TODO: Decompose matrix when fetching these?

  }, {
    key: 'translation',
    set: function set(value) {
      if (value != null) {
        this._dirtyTRS = true;
        this.setMatrixDirty();
      }
      this._translation = value;
    },
    get: function get() {
      this._dirtyTRS = true;
      this.setMatrixDirty();
      if (!this._translation) {
        this._translation = _glMatrix.vec3.clone(DEFAULT_TRANSLATION);
      }
      return this._translation;
    }
  }, {
    key: 'rotation',
    set: function set(value) {
      if (value != null) {
        this._dirtyTRS = true;
        this.setMatrixDirty();
      }
      this._rotation = value;
    },
    get: function get() {
      this._dirtyTRS = true;
      this.setMatrixDirty();
      if (!this._rotation) {
        this._rotation = _glMatrix.quat.clone(DEFAULT_ROTATION);
      }
      return this._rotation;
    }
  }, {
    key: 'scale',
    set: function set(value) {
      if (value != null) {
        this._dirtyTRS = true;
        this.setMatrixDirty();
      }
      this._scale = value;
    },
    get: function get() {
      this._dirtyTRS = true;
      this.setMatrixDirty();
      if (!this._scale) {
        this._scale = _glMatrix.vec3.clone(DEFAULT_SCALE);
      }
      return this._scale;
    }
  }, {
    key: 'renderPrimitives',
    get: function get() {
      return this._renderPrimitives;
    }
  }, {
    key: 'selectHandler',
    get: function get() {
      return this._selectHandler;
    }
  }]);

  return Node;
}();

/***/ }),

/***/ "./src/core/primitive.js":
/*!*******************************!*\
  !*** ./src/core/primitive.js ***!
  \*******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Primitive = exports.PrimitiveAttribute = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var PrimitiveAttribute = exports.PrimitiveAttribute = function PrimitiveAttribute(name, buffer, componentCount, componentType, stride, byteOffset) {
  _classCallCheck(this, PrimitiveAttribute);

  this.name = name;
  this.buffer = buffer;
  this.componentCount = componentCount || 3;
  this.componentType = componentType || 5126; // gl.FLOAT;
  this.stride = stride || 0;
  this.byteOffset = byteOffset || 0;
  this.normalized = false;
};

var Primitive = exports.Primitive = function () {
  function Primitive(attributes, elementCount, mode) {
    _classCallCheck(this, Primitive);

    this.attributes = attributes || [];
    this.elementCount = elementCount || 0;
    this.mode = mode || 4; // gl.TRIANGLES;
    this.indexBuffer = null;
    this.indexByteOffset = 0;
    this.indexType = 0;
    this._min = null;
    this._max = null;
  }

  _createClass(Primitive, [{
    key: 'setIndexBuffer',
    value: function setIndexBuffer(indexBuffer, byteOffset, indexType) {
      this.indexBuffer = indexBuffer;
      this.indexByteOffset = byteOffset || 0;
      this.indexType = indexType || 5123; // gl.UNSIGNED_SHORT;
    }
  }, {
    key: 'setBounds',
    value: function setBounds(min, max) {
      this._min = _glMatrix.vec3.clone(min);
      this._max = _glMatrix.vec3.clone(max);
    }
  }]);

  return Primitive;
}();

/***/ }),

/***/ "./src/core/program.js":
/*!*****************************!*\
  !*** ./src/core/program.js ***!
  \*****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

// Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var Program = exports.Program = function () {
  function Program(gl, vertSrc, fragSrc, attribMap, defines) {
    _classCallCheck(this, Program);

    this._gl = gl;
    this.program = gl.createProgram();
    this.attrib = null;
    this.uniform = null;
    this.defines = {};

    this._firstUse = true;
    this._nextUseCallbacks = [];

    var definesString = '';
    if (defines) {
      for (var define in defines) {
        this.defines[define] = defines[define];
        definesString += '#define ' + define + ' ' + defines[define] + '\n';
      }
    }

    this._vertShader = gl.createShader(gl.VERTEX_SHADER);
    gl.attachShader(this.program, this._vertShader);
    gl.shaderSource(this._vertShader, definesString + vertSrc);
    gl.compileShader(this._vertShader);

    this._fragShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.attachShader(this.program, this._fragShader);
    gl.shaderSource(this._fragShader, definesString + fragSrc);
    gl.compileShader(this._fragShader);

    if (attribMap) {
      this.attrib = {};
      for (var attribName in attribMap) {
        gl.bindAttribLocation(this.program, attribMap[attribName], attribName);
        this.attrib[attribName] = attribMap[attribName];
      }
    }

    gl.linkProgram(this.program);
  }

  _createClass(Program, [{
    key: 'onNextUse',
    value: function onNextUse(callback) {
      this._nextUseCallbacks.push(callback);
    }
  }, {
    key: 'use',
    value: function use() {
      var gl = this._gl;

      // If this is the first time the program has been used do all the error checking and
      // attrib/uniform querying needed.
      if (this._firstUse) {
        this._firstUse = false;
        if (!gl.getProgramParameter(this.program, gl.LINK_STATUS)) {
          if (!gl.getShaderParameter(this._vertShader, gl.COMPILE_STATUS)) {
            console.error('Vertex shader compile error: ' + gl.getShaderInfoLog(this._vertShader));
          } else if (!gl.getShaderParameter(this._fragShader, gl.COMPILE_STATUS)) {
            console.error('Fragment shader compile error: ' + gl.getShaderInfoLog(this._fragShader));
          } else {
            console.error('Program link error: ' + gl.getProgramInfoLog(this.program));
          }
          gl.deleteProgram(this.program);
          this.program = null;
        } else {
          if (!this.attrib) {
            this.attrib = {};
            var attribCount = gl.getProgramParameter(this.program, gl.ACTIVE_ATTRIBUTES);
            for (var i = 0; i < attribCount; i++) {
              var attribInfo = gl.getActiveAttrib(this.program, i);
              this.attrib[attribInfo.name] = gl.getAttribLocation(this.program, attribInfo.name);
            }
          }

          this.uniform = {};
          var uniformCount = gl.getProgramParameter(this.program, gl.ACTIVE_UNIFORMS);
          var uniformName = '';
          for (var _i = 0; _i < uniformCount; _i++) {
            var uniformInfo = gl.getActiveUniform(this.program, _i);
            uniformName = uniformInfo.name.replace('[0]', '');
            this.uniform[uniformName] = gl.getUniformLocation(this.program, uniformName);
          }
        }
        gl.deleteShader(this._vertShader);
        gl.deleteShader(this._fragShader);
      }

      gl.useProgram(this.program);

      if (this._nextUseCallbacks.length) {
        var _iteratorNormalCompletion = true;
        var _didIteratorError = false;
        var _iteratorError = undefined;

        try {
          for (var _iterator = this._nextUseCallbacks[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
            var callback = _step.value;

            callback(this);
          }
        } catch (err) {
          _didIteratorError = true;
          _iteratorError = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion && _iterator.return) {
              _iterator.return();
            }
          } finally {
            if (_didIteratorError) {
              throw _iteratorError;
            }
          }
        }

        this._nextUseCallbacks = [];
      }
    }
  }]);

  return Program;
}();

/***/ }),

/***/ "./src/core/ray.js":
/*!*************************!*\
  !*** ./src/core/ray.js ***!
  \*************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Ray = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }(); // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var normalMat = _glMatrix.mat3.create();

var RAY_INTERSECTION_OFFSET = 0.02;

var Ray = exports.Ray = function () {
  function Ray() {
    var matrix = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : null;

    _classCallCheck(this, Ray);

    this.origin = _glMatrix.vec3.create();

    this._dir = _glMatrix.vec3.create();
    this._dir[2] = -1.0;

    if (matrix) {
      _glMatrix.vec3.transformMat4(this.origin, this.origin, matrix);
      _glMatrix.mat3.fromMat4(normalMat, matrix);
      _glMatrix.vec3.transformMat3(this._dir, this._dir, normalMat);
    }

    // To force the inverse and sign calculations.
    this.dir = this._dir;
  }

  _createClass(Ray, [{
    key: 'intersectsAABB',


    // Borrowed from:
    // eslint-disable-next-line max-len
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
    value: function intersectsAABB(min, max) {
      var r = this;

      var bounds = [min, max];

      var tmin = (bounds[r.sign[0]][0] - r.origin[0]) * r.inv_dir[0];
      var tmax = (bounds[1 - r.sign[0]][0] - r.origin[0]) * r.inv_dir[0];
      var tymin = (bounds[r.sign[1]][1] - r.origin[1]) * r.inv_dir[1];
      var tymax = (bounds[1 - r.sign[1]][1] - r.origin[1]) * r.inv_dir[1];

      if (tmin > tymax || tymin > tmax) {
        return null;
      }
      if (tymin > tmin) {
        tmin = tymin;
      }
      if (tymax < tmax) {
        tmax = tymax;
      }

      var tzmin = (bounds[r.sign[2]][2] - r.origin[2]) * r.inv_dir[2];
      var tzmax = (bounds[1 - r.sign[2]][2] - r.origin[2]) * r.inv_dir[2];

      if (tmin > tzmax || tzmin > tmax) {
        return null;
      }
      if (tzmin > tmin) {
        tmin = tzmin;
      }
      if (tzmax < tmax) {
        tmax = tzmax;
      }

      var t = -1;
      if (tmin > 0 && tmax > 0) {
        t = Math.min(tmin, tmax);
      } else if (tmin > 0) {
        t = tmin;
      } else if (tmax > 0) {
        t = tmax;
      } else {
        // Intersection is behind the ray origin.
        return null;
      }

      // Push ray intersection point back along the ray a bit so that cursors
      // don't accidentally intersect with the hit surface.
      t -= RAY_INTERSECTION_OFFSET;

      // Return the point where the ray first intersected with the AABB.
      var intersectionPoint = _glMatrix.vec3.clone(this._dir);
      _glMatrix.vec3.scale(intersectionPoint, intersectionPoint, t);
      _glMatrix.vec3.add(intersectionPoint, intersectionPoint, this.origin);
      return intersectionPoint;
    }
  }, {
    key: 'dir',
    get: function get() {
      return this._dir;
    },
    set: function set(value) {
      this._dir = _glMatrix.vec3.copy(this._dir, value);
      _glMatrix.vec3.normalize(this._dir, this._dir);

      this.inv_dir = _glMatrix.vec3.fromValues(1.0 / this._dir[0], 1.0 / this._dir[1], 1.0 / this._dir[2]);

      this.sign = [this.inv_dir[0] < 0 ? 1 : 0, this.inv_dir[1] < 0 ? 1 : 0, this.inv_dir[2] < 0 ? 1 : 0];
    }
  }]);

  return Ray;
}();

/***/ }),

/***/ "./src/core/renderer.js":
/*!******************************!*\
  !*** ./src/core/renderer.js ***!
  \******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Renderer = exports.RenderTexture = exports.RenderView = exports.ATTRIB_MASK = exports.ATTRIB = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }(); // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

exports.createWebGLContext = createWebGLContext;

var _material = __webpack_require__(/*! ./material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ./node.js */ "./src/core/node.js");

var _program = __webpack_require__(/*! ./program.js */ "./src/core/program.js");

var _texture = __webpack_require__(/*! ./texture.js */ "./src/core/texture.js");

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var ATTRIB = exports.ATTRIB = {
  POSITION: 1,
  NORMAL: 2,
  TANGENT: 3,
  TEXCOORD_0: 4,
  TEXCOORD_1: 5,
  COLOR_0: 6
};

var ATTRIB_MASK = exports.ATTRIB_MASK = {
  POSITION: 0x0001,
  NORMAL: 0x0002,
  TANGENT: 0x0004,
  TEXCOORD_0: 0x0008,
  TEXCOORD_1: 0x0010,
  COLOR_0: 0x0020
};

var GL = WebGLRenderingContext; // For enums

var DEF_LIGHT_DIR = new Float32Array([-0.1, -1.0, -0.2]);
var DEF_LIGHT_COLOR = new Float32Array([1.0, 1.0, 0.9]);

var PRECISION_REGEX = new RegExp('precision (lowp|mediump|highp) float;');

var VERTEX_SHADER_SINGLE_ENTRY = '\nuniform mat4 PROJECTION_MATRIX, VIEW_MATRIX, MODEL_MATRIX;\n\nvoid main() {\n  gl_Position = vertex_main(PROJECTION_MATRIX, VIEW_MATRIX, MODEL_MATRIX);\n}\n';

var VERTEX_SHADER_MULTI_ENTRY = '\n#ERROR Multiview rendering is not implemented\nvoid main() {\n  gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n}\n';

var FRAGMENT_SHADER_ENTRY = '\nvoid main() {\n  gl_FragColor = fragment_main();\n}\n';

function isPowerOfTwo(n) {
  return (n & n - 1) === 0;
}

// Creates a WebGL context and initializes it with some common default state.
function createWebGLContext(glAttribs) {
  glAttribs = glAttribs || { alpha: false };

  var webglCanvas = document.createElement('canvas');
  var contextTypes = glAttribs.webgl2 ? ['webgl2'] : ['webgl', 'experimental-webgl'];
  var context = null;

  var _iteratorNormalCompletion = true;
  var _didIteratorError = false;
  var _iteratorError = undefined;

  try {
    for (var _iterator = contextTypes[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
      var contextType = _step.value;

      context = webglCanvas.getContext(contextType, glAttribs);
      if (context) {
        break;
      }
    }
  } catch (err) {
    _didIteratorError = true;
    _iteratorError = err;
  } finally {
    try {
      if (!_iteratorNormalCompletion && _iterator.return) {
        _iterator.return();
      }
    } finally {
      if (_didIteratorError) {
        throw _iteratorError;
      }
    }
  }

  if (!context) {
    var webglType = glAttribs.webgl2 ? 'WebGL 2' : 'WebGL';
    console.error('This browser does not support ' + webglType + '.');
    return null;
  }

  return context;
}

var RenderView = exports.RenderView = function () {
  function RenderView(projectionMatrix, viewMatrix) {
    var viewport = arguments.length > 2 && arguments[2] !== undefined ? arguments[2] : null;
    var eye = arguments.length > 3 && arguments[3] !== undefined ? arguments[3] : 'left';

    _classCallCheck(this, RenderView);

    this.projectionMatrix = projectionMatrix;
    this.viewMatrix = viewMatrix;
    this.viewport = viewport;
    // If an eye isn't given the left eye is assumed.
    this._eye = eye;
    this._eyeIndex = eye == 'left' ? 0 : 1;
  }

  _createClass(RenderView, [{
    key: 'eye',
    get: function get() {
      return this._eye;
    },
    set: function set(value) {
      this._eye = value;
      this._eyeIndex = value == 'left' ? 0 : 1;
    }
  }, {
    key: 'eyeIndex',
    get: function get() {
      return this._eyeIndex;
    }
  }]);

  return RenderView;
}();

var RenderBuffer = function () {
  function RenderBuffer(target, usage, buffer) {
    var _this = this;

    var length = arguments.length > 3 && arguments[3] !== undefined ? arguments[3] : 0;

    _classCallCheck(this, RenderBuffer);

    this._target = target;
    this._usage = usage;
    this._length = length;
    if (buffer instanceof Promise) {
      this._buffer = null;
      this._promise = buffer.then(function (buffer) {
        _this._buffer = buffer;
        return _this;
      });
    } else {
      this._buffer = buffer;
      this._promise = Promise.resolve(this);
    }
  }

  _createClass(RenderBuffer, [{
    key: 'waitForComplete',
    value: function waitForComplete() {
      return this._promise;
    }
  }]);

  return RenderBuffer;
}();

var RenderPrimitiveAttribute = function RenderPrimitiveAttribute(primitiveAttribute) {
  _classCallCheck(this, RenderPrimitiveAttribute);

  this._attrib_index = ATTRIB[primitiveAttribute.name];
  this._componentCount = primitiveAttribute.componentCount;
  this._componentType = primitiveAttribute.componentType;
  this._stride = primitiveAttribute.stride;
  this._byteOffset = primitiveAttribute.byteOffset;
  this._normalized = primitiveAttribute.normalized;
};

var RenderPrimitiveAttributeBuffer = function RenderPrimitiveAttributeBuffer(buffer) {
  _classCallCheck(this, RenderPrimitiveAttributeBuffer);

  this._buffer = buffer;
  this._attributes = [];
};

var RenderPrimitive = function () {
  function RenderPrimitive(primitive) {
    _classCallCheck(this, RenderPrimitive);

    this._activeFrameId = 0;
    this._instances = [];
    this._material = null;

    this.setPrimitive(primitive);
  }

  _createClass(RenderPrimitive, [{
    key: 'setPrimitive',
    value: function setPrimitive(primitive) {
      this._mode = primitive.mode;
      this._elementCount = primitive.elementCount;
      this._promise = null;
      this._vao = null;
      this._complete = false;
      this._attributeBuffers = [];
      this._attributeMask = 0;

      var _iteratorNormalCompletion2 = true;
      var _didIteratorError2 = false;
      var _iteratorError2 = undefined;

      try {
        for (var _iterator2 = primitive.attributes[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
          var attribute = _step2.value;

          this._attributeMask |= ATTRIB_MASK[attribute.name];
          var renderAttribute = new RenderPrimitiveAttribute(attribute);
          var foundBuffer = false;
          var _iteratorNormalCompletion3 = true;
          var _didIteratorError3 = false;
          var _iteratorError3 = undefined;

          try {
            for (var _iterator3 = this._attributeBuffers[Symbol.iterator](), _step3; !(_iteratorNormalCompletion3 = (_step3 = _iterator3.next()).done); _iteratorNormalCompletion3 = true) {
              var attributeBuffer = _step3.value;

              if (attributeBuffer._buffer == attribute.buffer) {
                attributeBuffer._attributes.push(renderAttribute);
                foundBuffer = true;
                break;
              }
            }
          } catch (err) {
            _didIteratorError3 = true;
            _iteratorError3 = err;
          } finally {
            try {
              if (!_iteratorNormalCompletion3 && _iterator3.return) {
                _iterator3.return();
              }
            } finally {
              if (_didIteratorError3) {
                throw _iteratorError3;
              }
            }
          }

          if (!foundBuffer) {
            var _attributeBuffer = new RenderPrimitiveAttributeBuffer(attribute.buffer);
            _attributeBuffer._attributes.push(renderAttribute);
            this._attributeBuffers.push(_attributeBuffer);
          }
        }
      } catch (err) {
        _didIteratorError2 = true;
        _iteratorError2 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion2 && _iterator2.return) {
            _iterator2.return();
          }
        } finally {
          if (_didIteratorError2) {
            throw _iteratorError2;
          }
        }
      }

      this._indexBuffer = null;
      this._indexByteOffset = 0;
      this._indexType = 0;

      if (primitive.indexBuffer) {
        this._indexByteOffset = primitive.indexByteOffset;
        this._indexType = primitive.indexType;
        this._indexBuffer = primitive.indexBuffer;
      }

      if (primitive._min) {
        this._min = _glMatrix.vec3.clone(primitive._min);
        this._max = _glMatrix.vec3.clone(primitive._max);
      } else {
        this._min = null;
        this._max = null;
      }

      if (this._material != null) {
        this.waitForComplete(); // To flip the _complete flag.
      }
    }
  }, {
    key: 'setRenderMaterial',
    value: function setRenderMaterial(material) {
      this._material = material;
      this._promise = null;
      this._complete = false;

      if (this._material != null) {
        this.waitForComplete(); // To flip the _complete flag.
      }
    }
  }, {
    key: 'markActive',
    value: function markActive(frameId) {
      if (this._complete && this._activeFrameId != frameId) {
        if (this._material) {
          if (!this._material.markActive(frameId)) {
            return;
          }
        }
        this._activeFrameId = frameId;
      }
    }
  }, {
    key: 'waitForComplete',
    value: function waitForComplete() {
      var _this2 = this;

      if (!this._promise) {
        if (!this._material) {
          return Promise.reject('RenderPrimitive does not have a material');
        }

        var completionPromises = [];

        var _iteratorNormalCompletion4 = true;
        var _didIteratorError4 = false;
        var _iteratorError4 = undefined;

        try {
          for (var _iterator4 = this._attributeBuffers[Symbol.iterator](), _step4; !(_iteratorNormalCompletion4 = (_step4 = _iterator4.next()).done); _iteratorNormalCompletion4 = true) {
            var attributeBuffer = _step4.value;

            if (!attributeBuffer._buffer._buffer) {
              completionPromises.push(attributeBuffer._buffer._promise);
            }
          }
        } catch (err) {
          _didIteratorError4 = true;
          _iteratorError4 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion4 && _iterator4.return) {
              _iterator4.return();
            }
          } finally {
            if (_didIteratorError4) {
              throw _iteratorError4;
            }
          }
        }

        if (this._indexBuffer && !this._indexBuffer._buffer) {
          completionPromises.push(this._indexBuffer._promise);
        }

        this._promise = Promise.all(completionPromises).then(function () {
          _this2._complete = true;
          return _this2;
        });
      }
      return this._promise;
    }
  }, {
    key: 'samplers',
    get: function get() {
      return this._material._samplerDictionary;
    }
  }, {
    key: 'uniforms',
    get: function get() {
      return this._material._uniform_dictionary;
    }
  }]);

  return RenderPrimitive;
}();

var RenderTexture = exports.RenderTexture = function () {
  function RenderTexture(texture) {
    _classCallCheck(this, RenderTexture);

    this._texture = texture;
    this._complete = false;
    this._activeFrameId = 0;
    this._activeCallback = null;
  }

  _createClass(RenderTexture, [{
    key: 'markActive',
    value: function markActive(frameId) {
      if (this._activeCallback && this._activeFrameId != frameId) {
        this._activeFrameId = frameId;
        this._activeCallback(this);
      }
    }
  }]);

  return RenderTexture;
}();

var inverseMatrix = _glMatrix.mat4.create();

function setCap(gl, glEnum, cap, prevState, state) {
  var change = (state & cap) - (prevState & cap);
  if (!change) {
    return;
  }

  if (change > 0) {
    gl.enable(glEnum);
  } else {
    gl.disable(glEnum);
  }
}

var RenderMaterialSampler = function () {
  function RenderMaterialSampler(renderer, materialSampler, index) {
    _classCallCheck(this, RenderMaterialSampler);

    this._renderer = renderer;
    this._uniformName = materialSampler._uniformName;
    this._renderTexture = renderer._getRenderTexture(materialSampler._texture);
    this._index = index;
  }

  _createClass(RenderMaterialSampler, [{
    key: 'texture',
    set: function set(value) {
      this._renderTexture = this._renderer._getRenderTexture(value);
    }
  }]);

  return RenderMaterialSampler;
}();

var RenderMaterialUniform = function () {
  function RenderMaterialUniform(materialUniform) {
    _classCallCheck(this, RenderMaterialUniform);

    this._uniformName = materialUniform._uniformName;
    this._uniform = null;
    this._length = materialUniform._length;
    if (materialUniform._value instanceof Array) {
      this._value = new Float32Array(materialUniform._value);
    } else {
      this._value = new Float32Array([materialUniform._value]);
    }
  }

  _createClass(RenderMaterialUniform, [{
    key: 'value',
    set: function set(value) {
      if (this._value.length == 1) {
        this._value[0] = value;
      } else {
        for (var i = 0; i < this._value.length; ++i) {
          this._value[i] = value[i];
        }
      }
    }
  }]);

  return RenderMaterialUniform;
}();

var RenderMaterial = function () {
  function RenderMaterial(renderer, material, program) {
    _classCallCheck(this, RenderMaterial);

    this._program = program;
    this._state = material.state._state;
    this._activeFrameId = 0;
    this._completeForActiveFrame = false;

    this._samplerDictionary = {};
    this._samplers = [];
    for (var i = 0; i < material._samplers.length; ++i) {
      var renderSampler = new RenderMaterialSampler(renderer, material._samplers[i], i);
      this._samplers.push(renderSampler);
      this._samplerDictionary[renderSampler._uniformName] = renderSampler;
    }

    this._uniform_dictionary = {};
    this._uniforms = [];
    var _iteratorNormalCompletion5 = true;
    var _didIteratorError5 = false;
    var _iteratorError5 = undefined;

    try {
      for (var _iterator5 = material._uniforms[Symbol.iterator](), _step5; !(_iteratorNormalCompletion5 = (_step5 = _iterator5.next()).done); _iteratorNormalCompletion5 = true) {
        var uniform = _step5.value;

        var renderUniform = new RenderMaterialUniform(uniform);
        this._uniforms.push(renderUniform);
        this._uniform_dictionary[renderUniform._uniformName] = renderUniform;
      }
    } catch (err) {
      _didIteratorError5 = true;
      _iteratorError5 = err;
    } finally {
      try {
        if (!_iteratorNormalCompletion5 && _iterator5.return) {
          _iterator5.return();
        }
      } finally {
        if (_didIteratorError5) {
          throw _iteratorError5;
        }
      }
    }

    this._firstBind = true;

    this._renderOrder = material.renderOrder;
    if (this._renderOrder == _material.RENDER_ORDER.DEFAULT) {
      if (this._state & _material.CAP.BLEND) {
        this._renderOrder = _material.RENDER_ORDER.TRANSPARENT;
      } else {
        this._renderOrder = _material.RENDER_ORDER.OPAQUE;
      }
    }
  }

  _createClass(RenderMaterial, [{
    key: 'bind',
    value: function bind(gl) {
      // First time we do a binding, cache the uniform locations and remove
      // unused uniforms from the list.
      if (this._firstBind) {
        for (var i = 0; i < this._samplers.length;) {
          var sampler = this._samplers[i];
          if (!this._program.uniform[sampler._uniformName]) {
            this._samplers.splice(i, 1);
            continue;
          }
          ++i;
        }

        for (var _i = 0; _i < this._uniforms.length;) {
          var uniform = this._uniforms[_i];
          uniform._uniform = this._program.uniform[uniform._uniformName];
          if (!uniform._uniform) {
            this._uniforms.splice(_i, 1);
            continue;
          }
          ++_i;
        }
        this._firstBind = false;
      }

      var _iteratorNormalCompletion6 = true;
      var _didIteratorError6 = false;
      var _iteratorError6 = undefined;

      try {
        for (var _iterator6 = this._samplers[Symbol.iterator](), _step6; !(_iteratorNormalCompletion6 = (_step6 = _iterator6.next()).done); _iteratorNormalCompletion6 = true) {
          var _sampler = _step6.value;

          gl.activeTexture(gl.TEXTURE0 + _sampler._index);
          if (_sampler._renderTexture && _sampler._renderTexture._complete) {
            gl.bindTexture(gl.TEXTURE_2D, _sampler._renderTexture._texture);
          } else {
            gl.bindTexture(gl.TEXTURE_2D, null);
          }
        }
      } catch (err) {
        _didIteratorError6 = true;
        _iteratorError6 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion6 && _iterator6.return) {
            _iterator6.return();
          }
        } finally {
          if (_didIteratorError6) {
            throw _iteratorError6;
          }
        }
      }

      var _iteratorNormalCompletion7 = true;
      var _didIteratorError7 = false;
      var _iteratorError7 = undefined;

      try {
        for (var _iterator7 = this._uniforms[Symbol.iterator](), _step7; !(_iteratorNormalCompletion7 = (_step7 = _iterator7.next()).done); _iteratorNormalCompletion7 = true) {
          var _uniform = _step7.value;

          switch (_uniform._length) {
            case 1:
              gl.uniform1fv(_uniform._uniform, _uniform._value);break;
            case 2:
              gl.uniform2fv(_uniform._uniform, _uniform._value);break;
            case 3:
              gl.uniform3fv(_uniform._uniform, _uniform._value);break;
            case 4:
              gl.uniform4fv(_uniform._uniform, _uniform._value);break;
          }
        }
      } catch (err) {
        _didIteratorError7 = true;
        _iteratorError7 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion7 && _iterator7.return) {
            _iterator7.return();
          }
        } finally {
          if (_didIteratorError7) {
            throw _iteratorError7;
          }
        }
      }
    }
  }, {
    key: 'markActive',
    value: function markActive(frameId) {
      if (this._activeFrameId != frameId) {
        this._activeFrameId = frameId;
        this._completeForActiveFrame = true;
        for (var i = 0; i < this._samplers.length; ++i) {
          var sampler = this._samplers[i];
          if (sampler._renderTexture) {
            if (!sampler._renderTexture._complete) {
              this._completeForActiveFrame = false;
              break;
            }
            sampler._renderTexture.markActive(frameId);
          }
        }
      }
      return this._completeForActiveFrame;
    }

    // Material State fetchers

  }, {
    key: '_capsDiff',


    // Only really for use from the renderer
    value: function _capsDiff(otherState) {
      return otherState & _material.MAT_STATE.CAPS_RANGE ^ this._state & _material.MAT_STATE.CAPS_RANGE;
    }
  }, {
    key: '_blendDiff',
    value: function _blendDiff(otherState) {
      if (!(this._state & _material.CAP.BLEND)) {
        return 0;
      }
      return otherState & _material.MAT_STATE.BLEND_FUNC_RANGE ^ this._state & _material.MAT_STATE.BLEND_FUNC_RANGE;
    }
  }, {
    key: '_depthFuncDiff',
    value: function _depthFuncDiff(otherState) {
      if (!(this._state & _material.CAP.DEPTH_TEST)) {
        return 0;
      }
      return otherState & _material.MAT_STATE.DEPTH_FUNC_RANGE ^ this._state & _material.MAT_STATE.DEPTH_FUNC_RANGE;
    }
  }, {
    key: 'cullFace',
    get: function get() {
      return !!(this._state & _material.CAP.CULL_FACE);
    }
  }, {
    key: 'blend',
    get: function get() {
      return !!(this._state & _material.CAP.BLEND);
    }
  }, {
    key: 'depthTest',
    get: function get() {
      return !!(this._state & _material.CAP.DEPTH_TEST);
    }
  }, {
    key: 'stencilTest',
    get: function get() {
      return !!(this._state & _material.CAP.STENCIL_TEST);
    }
  }, {
    key: 'colorMask',
    get: function get() {
      return !!(this._state & _material.CAP.COLOR_MASK);
    }
  }, {
    key: 'depthMask',
    get: function get() {
      return !!(this._state & _material.CAP.DEPTH_MASK);
    }
  }, {
    key: 'stencilMask',
    get: function get() {
      return !!(this._state & _material.CAP.STENCIL_MASK);
    }
  }, {
    key: 'depthFunc',
    get: function get() {
      return ((this._state & _material.MAT_STATE.DEPTH_FUNC_RANGE) >> _material.MAT_STATE.DEPTH_FUNC_SHIFT) + GL.NEVER;
    }
  }, {
    key: 'blendFuncSrc',
    get: function get() {
      return (0, _material.stateToBlendFunc)(this._state, _material.MAT_STATE.BLEND_SRC_RANGE, _material.MAT_STATE.BLEND_SRC_SHIFT);
    }
  }, {
    key: 'blendFuncDst',
    get: function get() {
      return (0, _material.stateToBlendFunc)(this._state, _material.MAT_STATE.BLEND_DST_RANGE, _material.MAT_STATE.BLEND_DST_SHIFT);
    }
  }]);

  return RenderMaterial;
}();

var Renderer = exports.Renderer = function () {
  function Renderer(gl) {
    _classCallCheck(this, Renderer);

    this._gl = gl || createWebGLContext();
    this._frameId = 0;
    this._programCache = {};
    this._textureCache = {};
    this._renderPrimitives = Array(_material.RENDER_ORDER.DEFAULT);
    this._cameraPositions = [];

    this._vaoExt = gl.getExtension('OES_vertex_array_object');

    var fragHighPrecision = gl.getShaderPrecisionFormat(gl.FRAGMENT_SHADER, gl.HIGH_FLOAT);
    this._defaultFragPrecision = fragHighPrecision.precision > 0 ? 'highp' : 'mediump';

    this._depthMaskNeedsReset = false;
    this._colorMaskNeedsReset = false;
  }

  _createClass(Renderer, [{
    key: 'createRenderBuffer',
    value: function createRenderBuffer(target, data) {
      var usage = arguments.length > 2 && arguments[2] !== undefined ? arguments[2] : GL.STATIC_DRAW;

      var gl = this._gl;
      var glBuffer = gl.createBuffer();

      if (data instanceof Promise) {
        var renderBuffer = new RenderBuffer(target, usage, data.then(function (data) {
          gl.bindBuffer(target, glBuffer);
          gl.bufferData(target, data, usage);
          renderBuffer._length = data.byteLength;
          return glBuffer;
        }));
        return renderBuffer;
      } else {
        gl.bindBuffer(target, glBuffer);
        gl.bufferData(target, data, usage);
        return new RenderBuffer(target, usage, glBuffer, data.byteLength);
      }
    }
  }, {
    key: 'updateRenderBuffer',
    value: function updateRenderBuffer(buffer, data) {
      var _this3 = this;

      var offset = arguments.length > 2 && arguments[2] !== undefined ? arguments[2] : 0;

      if (buffer._buffer) {
        var gl = this._gl;
        gl.bindBuffer(buffer._target, buffer._buffer);
        if (offset == 0 && buffer._length == data.byteLength) {
          gl.bufferData(buffer._target, data, buffer._usage);
        } else {
          gl.bufferSubData(buffer._target, offset, data);
        }
      } else {
        buffer.waitForComplete().then(function (buffer) {
          _this3.updateRenderBuffer(buffer, data, offset);
        });
      }
    }
  }, {
    key: 'createRenderPrimitive',
    value: function createRenderPrimitive(primitive, material) {
      var renderPrimitive = new RenderPrimitive(primitive);

      var program = this._getMaterialProgram(material, renderPrimitive);
      var renderMaterial = new RenderMaterial(this, material, program);
      renderPrimitive.setRenderMaterial(renderMaterial);

      if (!this._renderPrimitives[renderMaterial._renderOrder]) {
        this._renderPrimitives[renderMaterial._renderOrder] = [];
      }

      this._renderPrimitives[renderMaterial._renderOrder].push(renderPrimitive);

      return renderPrimitive;
    }
  }, {
    key: 'createMesh',
    value: function createMesh(primitive, material) {
      var meshNode = new _node.Node();
      meshNode.addRenderPrimitive(this.createRenderPrimitive(primitive, material));
      return meshNode;
    }
  }, {
    key: 'drawViews',
    value: function drawViews(views, rootNode) {
      if (!rootNode) {
        return;
      }

      var gl = this._gl;
      this._frameId++;

      rootNode.markActive(this._frameId);

      // If there's only one view then flip the algorithm a bit so that we're only
      // setting the viewport once.
      if (views.length == 1 && views[0].viewport) {
        var vp = views[0].viewport;
        this._gl.viewport(vp.x, vp.y, vp.width, vp.height);
      }

      // Get the positions of the 'camera' for each view matrix.
      for (var i = 0; i < views.length; ++i) {
        _glMatrix.mat4.invert(inverseMatrix, views[i].viewMatrix);

        if (this._cameraPositions.length <= i) {
          this._cameraPositions.push(_glMatrix.vec3.create());
        }
        var cameraPosition = this._cameraPositions[i];
        _glMatrix.vec3.set(cameraPosition, 0, 0, 0);
        _glMatrix.vec3.transformMat4(cameraPosition, cameraPosition, inverseMatrix);
      }

      // Draw each set of render primitives in order
      var _iteratorNormalCompletion8 = true;
      var _didIteratorError8 = false;
      var _iteratorError8 = undefined;

      try {
        for (var _iterator8 = this._renderPrimitives[Symbol.iterator](), _step8; !(_iteratorNormalCompletion8 = (_step8 = _iterator8.next()).done); _iteratorNormalCompletion8 = true) {
          var renderPrimitives = _step8.value;

          if (renderPrimitives && renderPrimitives.length) {
            this._drawRenderPrimitiveSet(views, renderPrimitives);
          }
        }
      } catch (err) {
        _didIteratorError8 = true;
        _iteratorError8 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion8 && _iterator8.return) {
            _iterator8.return();
          }
        } finally {
          if (_didIteratorError8) {
            throw _iteratorError8;
          }
        }
      }

      if (this._vaoExt) {
        this._vaoExt.bindVertexArrayOES(null);
      }

      if (this._depthMaskNeedsReset) {
        gl.depthMask(true);
      }
      if (this._colorMaskNeedsReset) {
        gl.colorMask(true, true, true, true);
      }
    }
  }, {
    key: '_drawRenderPrimitiveSet',
    value: function _drawRenderPrimitiveSet(views, renderPrimitives) {
      var gl = this._gl;
      var program = null;
      var material = null;
      var attribMask = 0;

      // Loop through every primitive known to the renderer.
      var _iteratorNormalCompletion9 = true;
      var _didIteratorError9 = false;
      var _iteratorError9 = undefined;

      try {
        for (var _iterator9 = renderPrimitives[Symbol.iterator](), _step9; !(_iteratorNormalCompletion9 = (_step9 = _iterator9.next()).done); _iteratorNormalCompletion9 = true) {
          var primitive = _step9.value;

          // Skip over those that haven't been marked as active for this frame.
          if (primitive._activeFrameId != this._frameId) {
            continue;
          }

          // Bind the primitive material's program if it's different than the one we
          // were using for the previous primitive.
          // TODO: The ording of this could be more efficient.
          if (program != primitive._material._program) {
            program = primitive._material._program;
            program.use();

            if (program.uniform.LIGHT_DIRECTION) {
              gl.uniform3fv(program.uniform.LIGHT_DIRECTION, DEF_LIGHT_DIR);
            }

            if (program.uniform.LIGHT_COLOR) {
              gl.uniform3fv(program.uniform.LIGHT_COLOR, DEF_LIGHT_COLOR);
            }

            if (views.length == 1) {
              gl.uniformMatrix4fv(program.uniform.PROJECTION_MATRIX, false, views[0].projectionMatrix);
              gl.uniformMatrix4fv(program.uniform.VIEW_MATRIX, false, views[0].viewMatrix);
              gl.uniform3fv(program.uniform.CAMERA_POSITION, this._cameraPositions[0]);
              gl.uniform1i(program.uniform.EYE_INDEX, views[0].eyeIndex);
            }
          }

          if (material != primitive._material) {
            this._bindMaterialState(primitive._material, material);
            primitive._material.bind(gl, program, material);
            material = primitive._material;
          }

          if (this._vaoExt) {
            if (primitive._vao) {
              this._vaoExt.bindVertexArrayOES(primitive._vao);
            } else {
              primitive._vao = this._vaoExt.createVertexArrayOES();
              this._vaoExt.bindVertexArrayOES(primitive._vao);
              this._bindPrimitive(primitive);
            }
          } else {
            this._bindPrimitive(primitive, attribMask);
            attribMask = primitive._attributeMask;
          }

          for (var i = 0; i < views.length; ++i) {
            var view = views[i];
            if (views.length > 1) {
              if (view.viewport) {
                var vp = view.viewport;
                gl.viewport(vp.x, vp.y, vp.width, vp.height);
              }
              gl.uniformMatrix4fv(program.uniform.PROJECTION_MATRIX, false, view.projectionMatrix);
              gl.uniformMatrix4fv(program.uniform.VIEW_MATRIX, false, view.viewMatrix);
              gl.uniform3fv(program.uniform.CAMERA_POSITION, this._cameraPositions[i]);
              gl.uniform1i(program.uniform.EYE_INDEX, view.eyeIndex);
            }

            var _iteratorNormalCompletion10 = true;
            var _didIteratorError10 = false;
            var _iteratorError10 = undefined;

            try {
              for (var _iterator10 = primitive._instances[Symbol.iterator](), _step10; !(_iteratorNormalCompletion10 = (_step10 = _iterator10.next()).done); _iteratorNormalCompletion10 = true) {
                var instance = _step10.value;

                if (instance._activeFrameId != this._frameId) {
                  continue;
                }

                gl.uniformMatrix4fv(program.uniform.MODEL_MATRIX, false, instance.worldMatrix);

                if (primitive._indexBuffer) {
                  gl.drawElements(primitive._mode, primitive._elementCount, primitive._indexType, primitive._indexByteOffset);
                } else {
                  gl.drawArrays(primitive._mode, 0, primitive._elementCount);
                }
              }
            } catch (err) {
              _didIteratorError10 = true;
              _iteratorError10 = err;
            } finally {
              try {
                if (!_iteratorNormalCompletion10 && _iterator10.return) {
                  _iterator10.return();
                }
              } finally {
                if (_didIteratorError10) {
                  throw _iteratorError10;
                }
              }
            }
          }
        }
      } catch (err) {
        _didIteratorError9 = true;
        _iteratorError9 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion9 && _iterator9.return) {
            _iterator9.return();
          }
        } finally {
          if (_didIteratorError9) {
            throw _iteratorError9;
          }
        }
      }
    }
  }, {
    key: '_getRenderTexture',
    value: function _getRenderTexture(texture) {
      var _this4 = this;

      if (!texture) {
        return null;
      }

      var key = texture.textureKey;
      if (!key) {
        throw new Error('Texure does not have a valid key');
      }

      if (key in this._textureCache) {
        return this._textureCache[key];
      } else {
        var gl = this._gl;
        var textureHandle = gl.createTexture();

        var renderTexture = new RenderTexture(textureHandle);
        this._textureCache[key] = renderTexture;

        if (texture instanceof _texture.DataTexture) {
          gl.bindTexture(gl.TEXTURE_2D, textureHandle);
          gl.texImage2D(gl.TEXTURE_2D, 0, texture.format, texture.width, texture.height, 0, texture.format, texture._type, texture._data);
          this._setSamplerParameters(texture);
          renderTexture._complete = true;
        } else {
          texture.waitForComplete().then(function () {
            gl.bindTexture(gl.TEXTURE_2D, textureHandle);
            gl.texImage2D(gl.TEXTURE_2D, 0, texture.format, texture.format, gl.UNSIGNED_BYTE, texture.source);
            _this4._setSamplerParameters(texture);
            renderTexture._complete = true;

            if (texture instanceof _texture.VideoTexture) {
              // Once the video starts playing, set a callback to update it's
              // contents each frame.
              texture._video.addEventListener('playing', function () {
                renderTexture._activeCallback = function () {
                  if (!texture._video.paused && !texture._video.waiting) {
                    gl.bindTexture(gl.TEXTURE_2D, textureHandle);
                    gl.texImage2D(gl.TEXTURE_2D, 0, texture.format, texture.format, gl.UNSIGNED_BYTE, texture.source);
                  }
                };
              });
            }
          });
        }

        return renderTexture;
      }
    }
  }, {
    key: '_setSamplerParameters',
    value: function _setSamplerParameters(texture) {
      var gl = this._gl;

      var sampler = texture.sampler;
      var powerOfTwo = isPowerOfTwo(texture.width) && isPowerOfTwo(texture.height);
      var mipmap = powerOfTwo && texture.mipmap;
      if (mipmap) {
        gl.generateMipmap(gl.TEXTURE_2D);
      }

      var minFilter = sampler.minFilter || (mipmap ? gl.LINEAR_MIPMAP_LINEAR : gl.LINEAR);
      var wrapS = sampler.wrapS || (powerOfTwo ? gl.REPEAT : gl.CLAMP_TO_EDGE);
      var wrapT = sampler.wrapT || (powerOfTwo ? gl.REPEAT : gl.CLAMP_TO_EDGE);

      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, sampler.magFilter || gl.LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, minFilter);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, wrapS);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, wrapT);
    }
  }, {
    key: '_getProgramKey',
    value: function _getProgramKey(name, defines) {
      var key = name + ':';

      for (var define in defines) {
        key += define + '=' + defines[define] + ',';
      }

      return key;
    }
  }, {
    key: '_getMaterialProgram',
    value: function _getMaterialProgram(material, renderPrimitive) {
      var _this5 = this;

      var materialName = material.materialName;
      var vertexSource = material.vertexSource;
      var fragmentSource = material.fragmentSource;

      // These should always be defined for every material
      if (materialName == null) {
        throw new Error('Material does not have a name');
      }
      if (vertexSource == null) {
        throw new Error('Material "' + materialName + '" does not have a vertex source');
      }
      if (fragmentSource == null) {
        throw new Error('Material "' + materialName + '" does not have a fragment source');
      }

      var defines = material.getProgramDefines(renderPrimitive);
      var key = this._getProgramKey(materialName, defines);

      if (key in this._programCache) {
        return this._programCache[key];
      } else {
        var multiview = false; // Handle this dynamically later
        var fullVertexSource = vertexSource;
        fullVertexSource += multiview ? VERTEX_SHADER_MULTI_ENTRY : VERTEX_SHADER_SINGLE_ENTRY;

        var precisionMatch = fragmentSource.match(PRECISION_REGEX);
        var fragPrecisionHeader = precisionMatch ? '' : 'precision ' + this._defaultFragPrecision + ' float;\n';

        var fullFragmentSource = fragPrecisionHeader + fragmentSource;
        fullFragmentSource += FRAGMENT_SHADER_ENTRY;

        var program = new _program.Program(this._gl, fullVertexSource, fullFragmentSource, ATTRIB, defines);
        this._programCache[key] = program;

        program.onNextUse(function (program) {
          // Bind the samplers to the right texture index. This is constant for
          // the lifetime of the program.
          for (var i = 0; i < material._samplers.length; ++i) {
            var sampler = material._samplers[i];
            var uniform = program.uniform[sampler._uniformName];
            if (uniform) {
              _this5._gl.uniform1i(uniform, i);
            }
          }
        });

        return program;
      }
    }
  }, {
    key: '_bindPrimitive',
    value: function _bindPrimitive(primitive, attribMask) {
      var gl = this._gl;

      // If the active attributes have changed then update the active set.
      if (attribMask != primitive._attributeMask) {
        for (var attrib in ATTRIB) {
          if (primitive._attributeMask & ATTRIB_MASK[attrib]) {
            gl.enableVertexAttribArray(ATTRIB[attrib]);
          } else {
            gl.disableVertexAttribArray(ATTRIB[attrib]);
          }
        }
      }

      // Bind the primitive attributes and indices.
      var _iteratorNormalCompletion11 = true;
      var _didIteratorError11 = false;
      var _iteratorError11 = undefined;

      try {
        for (var _iterator11 = primitive._attributeBuffers[Symbol.iterator](), _step11; !(_iteratorNormalCompletion11 = (_step11 = _iterator11.next()).done); _iteratorNormalCompletion11 = true) {
          var attributeBuffer = _step11.value;

          gl.bindBuffer(gl.ARRAY_BUFFER, attributeBuffer._buffer._buffer);
          var _iteratorNormalCompletion12 = true;
          var _didIteratorError12 = false;
          var _iteratorError12 = undefined;

          try {
            for (var _iterator12 = attributeBuffer._attributes[Symbol.iterator](), _step12; !(_iteratorNormalCompletion12 = (_step12 = _iterator12.next()).done); _iteratorNormalCompletion12 = true) {
              var _attrib = _step12.value;

              gl.vertexAttribPointer(_attrib._attrib_index, _attrib._componentCount, _attrib._componentType, _attrib._normalized, _attrib._stride, _attrib._byteOffset);
            }
          } catch (err) {
            _didIteratorError12 = true;
            _iteratorError12 = err;
          } finally {
            try {
              if (!_iteratorNormalCompletion12 && _iterator12.return) {
                _iterator12.return();
              }
            } finally {
              if (_didIteratorError12) {
                throw _iteratorError12;
              }
            }
          }
        }
      } catch (err) {
        _didIteratorError11 = true;
        _iteratorError11 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion11 && _iterator11.return) {
            _iterator11.return();
          }
        } finally {
          if (_didIteratorError11) {
            throw _iteratorError11;
          }
        }
      }

      if (primitive._indexBuffer) {
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, primitive._indexBuffer._buffer);
      } else {
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
      }
    }
  }, {
    key: '_bindMaterialState',
    value: function _bindMaterialState(material) {
      var prevMaterial = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : null;

      var gl = this._gl;

      var state = material._state;
      var prevState = prevMaterial ? prevMaterial._state : ~state;

      // Return early if both materials use identical state
      if (state == prevState) {
        return;
      }

      // Any caps bits changed?
      if (material._capsDiff(prevState)) {
        setCap(gl, gl.CULL_FACE, _material.CAP.CULL_FACE, prevState, state);
        setCap(gl, gl.BLEND, _material.CAP.BLEND, prevState, state);
        setCap(gl, gl.DEPTH_TEST, _material.CAP.DEPTH_TEST, prevState, state);
        setCap(gl, gl.STENCIL_TEST, _material.CAP.STENCIL_TEST, prevState, state);

        var colorMaskChange = (state & _material.CAP.COLOR_MASK) - (prevState & _material.CAP.COLOR_MASK);
        if (colorMaskChange) {
          var mask = colorMaskChange > 1;
          this._colorMaskNeedsReset = !mask;
          gl.colorMask(mask, mask, mask, mask);
        }

        var depthMaskChange = (state & _material.CAP.DEPTH_MASK) - (prevState & _material.CAP.DEPTH_MASK);
        if (depthMaskChange) {
          this._depthMaskNeedsReset = !(depthMaskChange > 1);
          gl.depthMask(depthMaskChange > 1);
        }

        var stencilMaskChange = (state & _material.CAP.STENCIL_MASK) - (prevState & _material.CAP.STENCIL_MASK);
        if (stencilMaskChange) {
          gl.stencilMask(stencilMaskChange > 1);
        }
      }

      // Blending enabled and blend func changed?
      if (material._blendDiff(prevState)) {
        gl.blendFunc(material.blendFuncSrc, material.blendFuncDst);
      }

      // Depth testing enabled and depth func changed?
      if (material._depthFuncDiff(prevState)) {
        gl.depthFunc(material.depthFunc);
      }
    }
  }, {
    key: 'gl',
    get: function get() {
      return this._gl;
    }
  }]);

  return Renderer;
}();

/***/ }),

/***/ "./src/core/texture.js":
/*!*****************************!*\
  !*** ./src/core/texture.js ***!
  \*****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

// Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var GL = WebGLRenderingContext; // For enums

var TextureSampler = exports.TextureSampler = function TextureSampler() {
  _classCallCheck(this, TextureSampler);

  this.minFilter = null;
  this.magFilter = null;
  this.wrapS = null;
  this.wrapT = null;
};

var Texture = exports.Texture = function () {
  function Texture() {
    _classCallCheck(this, Texture);

    this.sampler = new TextureSampler();
    this.mipmap = true;
    // TODO: Anisotropy
  }

  _createClass(Texture, [{
    key: 'format',
    get: function get() {
      return GL.RGBA;
    }
  }, {
    key: 'width',
    get: function get() {
      return 0;
    }
  }, {
    key: 'height',
    get: function get() {
      return 0;
    }
  }, {
    key: 'textureKey',
    get: function get() {
      return null;
    }
  }]);

  return Texture;
}();

var ImageTexture = exports.ImageTexture = function (_Texture) {
  _inherits(ImageTexture, _Texture);

  function ImageTexture(img) {
    _classCallCheck(this, ImageTexture);

    var _this = _possibleConstructorReturn(this, (ImageTexture.__proto__ || Object.getPrototypeOf(ImageTexture)).call(this));

    _this._img = img;
    _this._imgBitmap = null;

    if (img.src && img.complete) {
      if (img.naturalWidth) {
        _this._promise = _this._finishImage();
      } else {
        _this._promise = Promise.reject('Image provided had failed to load.');
      }
    } else {
      _this._promise = new Promise(function (resolve, reject) {
        img.addEventListener('load', function () {
          return resolve(_this._finishImage());
        });
        img.addEventListener('error', reject);
      });
    }
    return _this;
  }

  _createClass(ImageTexture, [{
    key: '_finishImage',
    value: function _finishImage() {
      var _this2 = this;

      if (window.createImageBitmap) {
        return window.createImageBitmap(this._img).then(function (imgBitmap) {
          _this2._imgBitmap = imgBitmap;
          return Promise.resolve(_this2);
        });
      }
      return Promise.resolve(this);
    }
  }, {
    key: 'waitForComplete',
    value: function waitForComplete() {
      return this._promise;
    }
  }, {
    key: 'format',
    get: function get() {
      // TODO: Can be RGB in some cases.
      return GL.RGBA;
    }
  }, {
    key: 'width',
    get: function get() {
      return this._img.width;
    }
  }, {
    key: 'height',
    get: function get() {
      return this._img.height;
    }
  }, {
    key: 'textureKey',
    get: function get() {
      return this._img.src;
    }
  }, {
    key: 'source',
    get: function get() {
      return this._imgBitmap || this._img;
    }
  }]);

  return ImageTexture;
}(Texture);

var UrlTexture = exports.UrlTexture = function (_ImageTexture) {
  _inherits(UrlTexture, _ImageTexture);

  function UrlTexture(url) {
    _classCallCheck(this, UrlTexture);

    var img = new Image();

    var _this3 = _possibleConstructorReturn(this, (UrlTexture.__proto__ || Object.getPrototypeOf(UrlTexture)).call(this, img));

    img.src = url;
    return _this3;
  }

  return UrlTexture;
}(ImageTexture);

var BlobTexture = exports.BlobTexture = function (_ImageTexture2) {
  _inherits(BlobTexture, _ImageTexture2);

  function BlobTexture(blob) {
    _classCallCheck(this, BlobTexture);

    var img = new Image();

    var _this4 = _possibleConstructorReturn(this, (BlobTexture.__proto__ || Object.getPrototypeOf(BlobTexture)).call(this, img));

    img.src = window.URL.createObjectURL(blob);
    return _this4;
  }

  return BlobTexture;
}(ImageTexture);

var VideoTexture = exports.VideoTexture = function (_Texture2) {
  _inherits(VideoTexture, _Texture2);

  function VideoTexture(video) {
    _classCallCheck(this, VideoTexture);

    var _this5 = _possibleConstructorReturn(this, (VideoTexture.__proto__ || Object.getPrototypeOf(VideoTexture)).call(this));

    _this5._video = video;

    if (video.readyState >= 2) {
      _this5._promise = Promise.resolve(_this5);
    } else if (video.error) {
      _this5._promise = Promise.reject(video.error);
    } else {
      _this5._promise = new Promise(function (resolve, reject) {
        video.addEventListener('loadeddata', function () {
          return resolve(_this5);
        });
        video.addEventListener('error', reject);
      });
    }
    return _this5;
  }

  _createClass(VideoTexture, [{
    key: 'waitForComplete',
    value: function waitForComplete() {
      return this._promise;
    }
  }, {
    key: 'format',
    get: function get() {
      // TODO: Can be RGB in some cases.
      return GL.RGBA;
    }
  }, {
    key: 'width',
    get: function get() {
      return this._video.videoWidth;
    }
  }, {
    key: 'height',
    get: function get() {
      return this._video.videoHeight;
    }
  }, {
    key: 'textureKey',
    get: function get() {
      return this._video.src;
    }
  }, {
    key: 'source',
    get: function get() {
      return this._video;
    }
  }]);

  return VideoTexture;
}(Texture);

var nextDataTextureIndex = 0;

var DataTexture = exports.DataTexture = function (_Texture3) {
  _inherits(DataTexture, _Texture3);

  function DataTexture(data, width, height) {
    var format = arguments.length > 3 && arguments[3] !== undefined ? arguments[3] : GL.RGBA;
    var type = arguments.length > 4 && arguments[4] !== undefined ? arguments[4] : GL.UNSIGNED_BYTE;

    _classCallCheck(this, DataTexture);

    var _this6 = _possibleConstructorReturn(this, (DataTexture.__proto__ || Object.getPrototypeOf(DataTexture)).call(this));

    _this6._data = data;
    _this6._width = width;
    _this6._height = height;
    _this6._format = format;
    _this6._type = type;
    _this6._key = 'DATA_' + nextDataTextureIndex;
    nextDataTextureIndex++;
    return _this6;
  }

  _createClass(DataTexture, [{
    key: 'format',
    get: function get() {
      return this._format;
    }
  }, {
    key: 'width',
    get: function get() {
      return this._width;
    }
  }, {
    key: 'height',
    get: function get() {
      return this._height;
    }
  }, {
    key: 'textureKey',
    get: function get() {
      return this._key;
    }
  }]);

  return DataTexture;
}(Texture);

var ColorTexture = exports.ColorTexture = function (_DataTexture) {
  _inherits(ColorTexture, _DataTexture);

  function ColorTexture(r, g, b, a) {
    _classCallCheck(this, ColorTexture);

    var colorData = new Uint8Array([r * 255.0, g * 255.0, b * 255.0, a * 255.0]);

    var _this7 = _possibleConstructorReturn(this, (ColorTexture.__proto__ || Object.getPrototypeOf(ColorTexture)).call(this, colorData, 1, 1));

    _this7.mipmap = false;
    _this7._key = 'COLOR_' + colorData[0] + '_' + colorData[1] + '_' + colorData[2] + '_' + colorData[3];
    return _this7;
  }

  return ColorTexture;
}(DataTexture);

/***/ }),

/***/ "./src/cottontail.js":
/*!***************************!*\
  !*** ./src/cottontail.js ***!
  \***************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});

var _node = __webpack_require__(/*! ./core/node.js */ "./src/core/node.js");

Object.defineProperty(exports, 'Node', {
  enumerable: true,
  get: function get() {
    return _node.Node;
  }
});

var _renderer = __webpack_require__(/*! ./core/renderer.js */ "./src/core/renderer.js");

Object.defineProperty(exports, 'Renderer', {
  enumerable: true,
  get: function get() {
    return _renderer.Renderer;
  }
});
Object.defineProperty(exports, 'createWebGLContext', {
  enumerable: true,
  get: function get() {
    return _renderer.createWebGLContext;
  }
});

var _texture = __webpack_require__(/*! ./core/texture.js */ "./src/core/texture.js");

Object.defineProperty(exports, 'UrlTexture', {
  enumerable: true,
  get: function get() {
    return _texture.UrlTexture;
  }
});

var _primitiveStream = __webpack_require__(/*! ./geometry/primitive-stream.js */ "./src/geometry/primitive-stream.js");

Object.defineProperty(exports, 'PrimitiveStream', {
  enumerable: true,
  get: function get() {
    return _primitiveStream.PrimitiveStream;
  }
});

var _boxBuilder = __webpack_require__(/*! ./geometry/box-builder.js */ "./src/geometry/box-builder.js");

Object.defineProperty(exports, 'BoxBuilder', {
  enumerable: true,
  get: function get() {
    return _boxBuilder.BoxBuilder;
  }
});

var _pbr = __webpack_require__(/*! ./materials/pbr.js */ "./src/materials/pbr.js");

Object.defineProperty(exports, 'PbrMaterial', {
  enumerable: true,
  get: function get() {
    return _pbr.PbrMaterial;
  }
});

var _glMatrix = __webpack_require__(/*! ./math/gl-matrix.js */ "./src/math/gl-matrix.js");

Object.defineProperty(exports, 'mat4', {
  enumerable: true,
  get: function get() {
    return _glMatrix.mat4;
  }
});
Object.defineProperty(exports, 'mat3', {
  enumerable: true,
  get: function get() {
    return _glMatrix.mat3;
  }
});
Object.defineProperty(exports, 'vec3', {
  enumerable: true,
  get: function get() {
    return _glMatrix.vec3;
  }
});
Object.defineProperty(exports, 'quat', {
  enumerable: true,
  get: function get() {
    return _glMatrix.quat;
  }
});

var _boundsRenderer = __webpack_require__(/*! ./nodes/bounds-renderer.js */ "./src/nodes/bounds-renderer.js");

Object.defineProperty(exports, 'BoundsRenderer', {
  enumerable: true,
  get: function get() {
    return _boundsRenderer.BoundsRenderer;
  }
});

var _button = __webpack_require__(/*! ./nodes/button.js */ "./src/nodes/button.js");

Object.defineProperty(exports, 'ButtonNode', {
  enumerable: true,
  get: function get() {
    return _button.ButtonNode;
  }
});

var _cubeSea = __webpack_require__(/*! ./nodes/cube-sea.js */ "./src/nodes/cube-sea.js");

Object.defineProperty(exports, 'CubeSeaNode', {
  enumerable: true,
  get: function get() {
    return _cubeSea.CubeSeaNode;
  }
});

var _gltf = __webpack_require__(/*! ./nodes/gltf2.js */ "./src/nodes/gltf2.js");

Object.defineProperty(exports, 'Gltf2Node', {
  enumerable: true,
  get: function get() {
    return _gltf.Gltf2Node;
  }
});

var _skybox = __webpack_require__(/*! ./nodes/skybox.js */ "./src/nodes/skybox.js");

Object.defineProperty(exports, 'SkyboxNode', {
  enumerable: true,
  get: function get() {
    return _skybox.SkyboxNode;
  }
});

var _video = __webpack_require__(/*! ./nodes/video.js */ "./src/nodes/video.js");

Object.defineProperty(exports, 'VideoNode', {
  enumerable: true,
  get: function get() {
    return _video.VideoNode;
  }
});

var _scene = __webpack_require__(/*! ./scenes/scene.js */ "./src/scenes/scene.js");

Object.defineProperty(exports, 'WebXRView', {
  enumerable: true,
  get: function get() {
    return _scene.WebXRView;
  }
});
Object.defineProperty(exports, 'Scene', {
  enumerable: true,
  get: function get() {
    return _scene.Scene;
  }
});

var _fallbackHelper = __webpack_require__(/*! ./util/fallback-helper.js */ "./src/util/fallback-helper.js");

Object.defineProperty(exports, 'FallbackHelper', {
  enumerable: true,
  get: function get() {
    return _fallbackHelper.FallbackHelper;
  }
});

/***/ }),

/***/ "./src/geometry/box-builder.js":
/*!*************************************!*\
  !*** ./src/geometry/box-builder.js ***!
  \*************************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.BoxBuilder = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _primitiveStream = __webpack_require__(/*! ./primitive-stream.js */ "./src/geometry/primitive-stream.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var BoxBuilder = exports.BoxBuilder = function (_GeometryBuilderBase) {
  _inherits(BoxBuilder, _GeometryBuilderBase);

  function BoxBuilder() {
    _classCallCheck(this, BoxBuilder);

    return _possibleConstructorReturn(this, (BoxBuilder.__proto__ || Object.getPrototypeOf(BoxBuilder)).apply(this, arguments));
  }

  _createClass(BoxBuilder, [{
    key: 'pushBox',
    value: function pushBox(min, max) {
      var stream = this.primitiveStream;

      var w = max[0] - min[0];
      var h = max[1] - min[1];
      var d = max[2] - min[2];

      var wh = w * 0.5;
      var hh = h * 0.5;
      var dh = d * 0.5;

      var cx = min[0] + wh;
      var cy = min[1] + hh;
      var cz = min[2] + dh;

      stream.startGeometry();

      // Bottom
      var idx = stream.nextVertexIndex;
      stream.pushTriangle(idx, idx + 1, idx + 2);
      stream.pushTriangle(idx, idx + 2, idx + 3);

      //                 X       Y       Z      U    V    NX    NY   NZ
      stream.pushVertex(-wh + cx, -hh + cy, -dh + cz, 0.0, 1.0, 0.0, -1.0, 0.0);
      stream.pushVertex(+wh + cx, -hh + cy, -dh + cz, 1.0, 1.0, 0.0, -1.0, 0.0);
      stream.pushVertex(+wh + cx, -hh + cy, +dh + cz, 1.0, 0.0, 0.0, -1.0, 0.0);
      stream.pushVertex(-wh + cx, -hh + cy, +dh + cz, 0.0, 0.0, 0.0, -1.0, 0.0);

      // Top
      idx = stream.nextVertexIndex;
      stream.pushTriangle(idx, idx + 2, idx + 1);
      stream.pushTriangle(idx, idx + 3, idx + 2);

      stream.pushVertex(-wh + cx, +hh + cy, -dh + cz, 0.0, 0.0, 0.0, 1.0, 0.0);
      stream.pushVertex(+wh + cx, +hh + cy, -dh + cz, 1.0, 0.0, 0.0, 1.0, 0.0);
      stream.pushVertex(+wh + cx, +hh + cy, +dh + cz, 1.0, 1.0, 0.0, 1.0, 0.0);
      stream.pushVertex(-wh + cx, +hh + cy, +dh + cz, 0.0, 1.0, 0.0, 1.0, 0.0);

      // Left
      idx = stream.nextVertexIndex;
      stream.pushTriangle(idx, idx + 2, idx + 1);
      stream.pushTriangle(idx, idx + 3, idx + 2);

      stream.pushVertex(-wh + cx, -hh + cy, -dh + cz, 0.0, 1.0, -1.0, 0.0, 0.0);
      stream.pushVertex(-wh + cx, +hh + cy, -dh + cz, 0.0, 0.0, -1.0, 0.0, 0.0);
      stream.pushVertex(-wh + cx, +hh + cy, +dh + cz, 1.0, 0.0, -1.0, 0.0, 0.0);
      stream.pushVertex(-wh + cx, -hh + cy, +dh + cz, 1.0, 1.0, -1.0, 0.0, 0.0);

      // Right
      idx = stream.nextVertexIndex;
      stream.pushTriangle(idx, idx + 1, idx + 2);
      stream.pushTriangle(idx, idx + 2, idx + 3);

      stream.pushVertex(+wh + cx, -hh + cy, -dh + cz, 1.0, 1.0, 1.0, 0.0, 0.0);
      stream.pushVertex(+wh + cx, +hh + cy, -dh + cz, 1.0, 0.0, 1.0, 0.0, 0.0);
      stream.pushVertex(+wh + cx, +hh + cy, +dh + cz, 0.0, 0.0, 1.0, 0.0, 0.0);
      stream.pushVertex(+wh + cx, -hh + cy, +dh + cz, 0.0, 1.0, 1.0, 0.0, 0.0);

      // Back
      idx = stream.nextVertexIndex;
      stream.pushTriangle(idx, idx + 2, idx + 1);
      stream.pushTriangle(idx, idx + 3, idx + 2);

      stream.pushVertex(-wh + cx, -hh + cy, -dh + cz, 1.0, 1.0, 0.0, 0.0, -1.0);
      stream.pushVertex(+wh + cx, -hh + cy, -dh + cz, 0.0, 1.0, 0.0, 0.0, -1.0);
      stream.pushVertex(+wh + cx, +hh + cy, -dh + cz, 0.0, 0.0, 0.0, 0.0, -1.0);
      stream.pushVertex(-wh + cx, +hh + cy, -dh + cz, 1.0, 0.0, 0.0, 0.0, -1.0);

      // Front
      idx = stream.nextVertexIndex;
      stream.pushTriangle(idx, idx + 1, idx + 2);
      stream.pushTriangle(idx, idx + 2, idx + 3);

      stream.pushVertex(-wh + cx, -hh + cy, +dh + cz, 0.0, 1.0, 0.0, 0.0, 1.0);
      stream.pushVertex(+wh + cx, -hh + cy, +dh + cz, 1.0, 1.0, 0.0, 0.0, 1.0);
      stream.pushVertex(+wh + cx, +hh + cy, +dh + cz, 1.0, 0.0, 0.0, 0.0, 1.0);
      stream.pushVertex(-wh + cx, +hh + cy, +dh + cz, 0.0, 0.0, 0.0, 0.0, 1.0);

      stream.endGeometry();
    }
  }, {
    key: 'pushCube',
    value: function pushCube() {
      var center = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : [0, 0, 0];
      var size = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : 1.0;

      var hs = size * 0.5;
      this.pushBox([center[0] - hs, center[1] - hs, center[2] - hs], [center[0] + hs, center[1] + hs, center[2] + hs]);
    }
  }]);

  return BoxBuilder;
}(_primitiveStream.GeometryBuilderBase);

/***/ }),

/***/ "./src/geometry/primitive-stream.js":
/*!******************************************!*\
  !*** ./src/geometry/primitive-stream.js ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.GeometryBuilderBase = exports.PrimitiveStream = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }(); // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var GL = WebGLRenderingContext; // For enums

var tempVec3 = _glMatrix.vec3.create();

var PrimitiveStream = exports.PrimitiveStream = function () {
  function PrimitiveStream(options) {
    _classCallCheck(this, PrimitiveStream);

    this._vertices = [];
    this._indices = [];

    this._geometryStarted = false;

    this._vertexOffset = 0;
    this._vertexIndex = 0;
    this._highIndex = 0;

    this._flipWinding = false;
    this._invertNormals = false;
    this._transform = null;
    this._normalTransform = null;
    this._min = null;
    this._max = null;
  }

  _createClass(PrimitiveStream, [{
    key: 'startGeometry',
    value: function startGeometry() {
      if (this._geometryStarted) {
        throw new Error('Attempted to start a new geometry before the previous one was ended.');
      }

      this._geometryStarted = true;
      this._vertexIndex = 0;
      this._highIndex = 0;
    }
  }, {
    key: 'endGeometry',
    value: function endGeometry() {
      if (!this._geometryStarted) {
        throw new Error('Attempted to end a geometry before one was started.');
      }

      if (this._highIndex >= this._vertexIndex) {
        throw new Error('Geometry contains indices that are out of bounds.\n                       (Contains an index of ' + this._highIndex + ' when the vertex count is ' + this._vertexIndex + ')');
      }

      this._geometryStarted = false;
      this._vertexOffset += this._vertexIndex;

      // TODO: Anything else need to be done to finish processing here?
    }
  }, {
    key: 'pushVertex',
    value: function pushVertex(x, y, z, u, v, nx, ny, nz) {
      if (!this._geometryStarted) {
        throw new Error('Cannot push vertices before calling startGeometry().');
      }

      // Transform the incoming vertex if we have a transformation matrix
      if (this._transform) {
        tempVec3[0] = x;
        tempVec3[1] = y;
        tempVec3[2] = z;
        _glMatrix.vec3.transformMat4(tempVec3, tempVec3, this._transform);
        x = tempVec3[0];
        y = tempVec3[1];
        z = tempVec3[2];

        tempVec3[0] = nx;
        tempVec3[1] = ny;
        tempVec3[2] = nz;
        _glMatrix.vec3.transformMat3(tempVec3, tempVec3, this._normalTransform);
        nx = tempVec3[0];
        ny = tempVec3[1];
        nz = tempVec3[2];
      }

      if (this._invertNormals) {
        nx *= -1.0;
        ny *= -1.0;
        nz *= -1.0;
      }

      this._vertices.push(x, y, z, u, v, nx, ny, nz);

      if (this._min) {
        this._min[0] = Math.min(this._min[0], x);
        this._min[1] = Math.min(this._min[1], y);
        this._min[2] = Math.min(this._min[2], z);
        this._max[0] = Math.max(this._max[0], x);
        this._max[1] = Math.max(this._max[1], y);
        this._max[2] = Math.max(this._max[2], z);
      } else {
        this._min = _glMatrix.vec3.fromValues(x, y, z);
        this._max = _glMatrix.vec3.fromValues(x, y, z);
      }

      return this._vertexIndex++;
    }
  }, {
    key: 'pushTriangle',
    value: function pushTriangle(idxA, idxB, idxC) {
      if (!this._geometryStarted) {
        throw new Error('Cannot push triangles before calling startGeometry().');
      }

      this._highIndex = Math.max(this._highIndex, idxA, idxB, idxC);

      idxA += this._vertexOffset;
      idxB += this._vertexOffset;
      idxC += this._vertexOffset;

      if (this._flipWinding) {
        this._indices.push(idxC, idxB, idxA);
      } else {
        this._indices.push(idxA, idxB, idxC);
      }
    }
  }, {
    key: 'clear',
    value: function clear() {
      if (this._geometryStarted) {
        throw new Error('Cannot clear before ending the current geometry.');
      }

      this._vertices = [];
      this._indices = [];
      this._vertexOffset = 0;
      this._min = null;
      this._max = null;
    }
  }, {
    key: 'finishPrimitive',
    value: function finishPrimitive(renderer) {
      if (!this._vertexOffset) {
        throw new Error('Attempted to call finishPrimitive() before creating any geometry.');
      }

      var vertexBuffer = renderer.createRenderBuffer(GL.ARRAY_BUFFER, new Float32Array(this._vertices));
      var indexBuffer = renderer.createRenderBuffer(GL.ELEMENT_ARRAY_BUFFER, new Uint16Array(this._indices));

      var attribs = [new _primitive.PrimitiveAttribute('POSITION', vertexBuffer, 3, GL.FLOAT, 32, 0), new _primitive.PrimitiveAttribute('TEXCOORD_0', vertexBuffer, 2, GL.FLOAT, 32, 12), new _primitive.PrimitiveAttribute('NORMAL', vertexBuffer, 3, GL.FLOAT, 32, 20)];

      var primitive = new _primitive.Primitive(attribs, this._indices.length);
      primitive.setIndexBuffer(indexBuffer);
      primitive.setBounds(this._min, this._max);

      return primitive;
    }
  }, {
    key: 'flipWinding',
    set: function set(value) {
      if (this._geometryStarted) {
        throw new Error('Cannot change flipWinding before ending the current geometry.');
      }
      this._flipWinding = value;
    },
    get: function get() {
      this._flipWinding;
    }
  }, {
    key: 'invertNormals',
    set: function set(value) {
      if (this._geometryStarted) {
        throw new Error('Cannot change invertNormals before ending the current geometry.');
      }
      this._invertNormals = value;
    },
    get: function get() {
      this._invertNormals;
    }
  }, {
    key: 'transform',
    set: function set(value) {
      if (this._geometryStarted) {
        throw new Error('Cannot change transform before ending the current geometry.');
      }
      this._transform = value;
      if (this._transform) {
        if (!this._normalTransform) {
          this._normalTransform = _glMatrix.mat3.create();
        }
        _glMatrix.mat3.fromMat4(this._normalTransform, this._transform);
      }
    },
    get: function get() {
      this._transform;
    }
  }, {
    key: 'nextVertexIndex',
    get: function get() {
      return this._vertexIndex;
    }
  }]);

  return PrimitiveStream;
}();

var GeometryBuilderBase = exports.GeometryBuilderBase = function () {
  function GeometryBuilderBase(primitiveStream) {
    _classCallCheck(this, GeometryBuilderBase);

    if (primitiveStream) {
      this._stream = primitiveStream;
    } else {
      this._stream = new PrimitiveStream();
    }
  }

  _createClass(GeometryBuilderBase, [{
    key: 'finishPrimitive',
    value: function finishPrimitive(renderer) {
      return this._stream.finishPrimitive(renderer);
    }
  }, {
    key: 'clear',
    value: function clear() {
      this._stream.clear();
    }
  }, {
    key: 'primitiveStream',
    set: function set(value) {
      this._stream = value;
    },
    get: function get() {
      return this._stream;
    }
  }]);

  return GeometryBuilderBase;
}();

/***/ }),

/***/ "./src/loaders/gltf2.js":
/*!******************************!*\
  !*** ./src/loaders/gltf2.js ***!
  \******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Gltf2Loader = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }(); // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var _pbr = __webpack_require__(/*! ../materials/pbr.js */ "./src/materials/pbr.js");

var _node2 = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

var _texture = __webpack_require__(/*! ../core/texture.js */ "./src/core/texture.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var GL = WebGLRenderingContext; // For enums

var GLB_MAGIC = 0x46546C67;
var CHUNK_TYPE = {
  JSON: 0x4E4F534A,
  BIN: 0x004E4942
};

function isAbsoluteUri(uri) {
  var absRegEx = new RegExp('^' + window.location.protocol, 'i');
  return !!uri.match(absRegEx);
}

function isDataUri(uri) {
  var dataRegEx = /^data:/;
  return !!uri.match(dataRegEx);
}

function resolveUri(uri, baseUrl) {
  if (isAbsoluteUri(uri) || isDataUri(uri)) {
    return uri;
  }
  return baseUrl + uri;
}

function getComponentCount(type) {
  switch (type) {
    case 'SCALAR':
      return 1;
    case 'VEC2':
      return 2;
    case 'VEC3':
      return 3;
    case 'VEC4':
      return 4;
    default:
      return 0;
  }
}

/**
 * Gltf2SceneLoader
 * Loads glTF 2.0 scenes into a renderable node tree.
 */

var Gltf2Loader = exports.Gltf2Loader = function () {
  function Gltf2Loader(renderer) {
    _classCallCheck(this, Gltf2Loader);

    this.renderer = renderer;
    this._gl = renderer._gl;
  }

  _createClass(Gltf2Loader, [{
    key: 'loadFromUrl',
    value: function loadFromUrl(url) {
      var _this = this;

      return fetch(url).then(function (response) {
        var i = url.lastIndexOf('/');
        var baseUrl = i !== 0 ? url.substring(0, i + 1) : '';

        if (url.endsWith('.gltf')) {
          return response.json().then(function (json) {
            return _this.loadFromJson(json, baseUrl);
          });
        } else if (url.endsWith('.glb')) {
          return response.arrayBuffer().then(function (arrayBuffer) {
            return _this.loadFromBinary(arrayBuffer, baseUrl);
          });
        } else {
          throw new Error('Unrecognized file extension');
        }
      });
    }
  }, {
    key: 'loadFromBinary',
    value: function loadFromBinary(arrayBuffer, baseUrl) {
      var headerView = new DataView(arrayBuffer, 0, 12);
      var magic = headerView.getUint32(0, true);
      var version = headerView.getUint32(4, true);
      var length = headerView.getUint32(8, true);

      if (magic != GLB_MAGIC) {
        throw new Error('Invalid magic string in binary header.');
      }

      if (version != 2) {
        throw new Error('Incompatible version in binary header.');
      }

      var chunks = {};
      var chunkOffset = 12;
      while (chunkOffset < length) {
        var chunkHeaderView = new DataView(arrayBuffer, chunkOffset, 8);
        var chunkLength = chunkHeaderView.getUint32(0, true);
        var chunkType = chunkHeaderView.getUint32(4, true);
        chunks[chunkType] = arrayBuffer.slice(chunkOffset + 8, chunkOffset + 8 + chunkLength);
        chunkOffset += chunkLength + 8;
      }

      if (!chunks[CHUNK_TYPE.JSON]) {
        throw new Error('File contained no json chunk.');
      }

      var decoder = new TextDecoder('utf-8');
      var jsonString = decoder.decode(chunks[CHUNK_TYPE.JSON]);
      var json = JSON.parse(jsonString);
      return this.loadFromJson(json, baseUrl, chunks[CHUNK_TYPE.BIN]);
    }
  }, {
    key: 'loadFromJson',
    value: function loadFromJson(json, baseUrl, binaryChunk) {
      if (!json.asset) {
        throw new Error('Missing asset description.');
      }

      if (json.asset.minVersion != '2.0' && json.asset.version != '2.0') {
        throw new Error('Incompatible asset version.');
      }

      var buffers = [];
      if (binaryChunk) {
        buffers[0] = new Gltf2Resource({}, baseUrl, binaryChunk);
      } else {
        var _iteratorNormalCompletion = true;
        var _didIteratorError = false;
        var _iteratorError = undefined;

        try {
          for (var _iterator = json.buffers[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
            var buffer = _step.value;

            buffers.push(new Gltf2Resource(buffer, baseUrl));
          }
        } catch (err) {
          _didIteratorError = true;
          _iteratorError = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion && _iterator.return) {
              _iterator.return();
            }
          } finally {
            if (_didIteratorError) {
              throw _iteratorError;
            }
          }
        }
      }

      var bufferViews = [];
      var _iteratorNormalCompletion2 = true;
      var _didIteratorError2 = false;
      var _iteratorError2 = undefined;

      try {
        for (var _iterator2 = json.bufferViews[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
          var bufferView = _step2.value;

          bufferViews.push(new Gltf2BufferView(bufferView, buffers));
        }
      } catch (err) {
        _didIteratorError2 = true;
        _iteratorError2 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion2 && _iterator2.return) {
            _iterator2.return();
          }
        } finally {
          if (_didIteratorError2) {
            throw _iteratorError2;
          }
        }
      }

      var images = [];
      if (json.images) {
        var _iteratorNormalCompletion3 = true;
        var _didIteratorError3 = false;
        var _iteratorError3 = undefined;

        try {
          for (var _iterator3 = json.images[Symbol.iterator](), _step3; !(_iteratorNormalCompletion3 = (_step3 = _iterator3.next()).done); _iteratorNormalCompletion3 = true) {
            var image = _step3.value;

            images.push(new Gltf2Resource(image, baseUrl));
          }
        } catch (err) {
          _didIteratorError3 = true;
          _iteratorError3 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion3 && _iterator3.return) {
              _iterator3.return();
            }
          } finally {
            if (_didIteratorError3) {
              throw _iteratorError3;
            }
          }
        }
      }

      var textures = [];
      if (json.textures) {
        var _iteratorNormalCompletion4 = true;
        var _didIteratorError4 = false;
        var _iteratorError4 = undefined;

        try {
          for (var _iterator4 = json.textures[Symbol.iterator](), _step4; !(_iteratorNormalCompletion4 = (_step4 = _iterator4.next()).done); _iteratorNormalCompletion4 = true) {
            var texture = _step4.value;

            var _image = images[texture.source];
            var glTexture = _image.texture(bufferViews);
            if (texture.sampler) {
              var sampler = sampler[texture.sampler];
              glTexture.sampler.minFilter = sampler.minFilter;
              glTexture.sampler.magFilter = sampler.magFilter;
              glTexture.sampler.wrapS = sampler.wrapS;
              glTexture.sampler.wrapT = sampler.wrapT;
            }
            textures.push(glTexture);
          }
        } catch (err) {
          _didIteratorError4 = true;
          _iteratorError4 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion4 && _iterator4.return) {
              _iterator4.return();
            }
          } finally {
            if (_didIteratorError4) {
              throw _iteratorError4;
            }
          }
        }
      }

      function getTexture(textureInfo) {
        if (!textureInfo) {
          return null;
        }
        return textures[textureInfo.index];
      }

      var materials = [];
      if (json.materials) {
        var _iteratorNormalCompletion5 = true;
        var _didIteratorError5 = false;
        var _iteratorError5 = undefined;

        try {
          for (var _iterator5 = json.materials[Symbol.iterator](), _step5; !(_iteratorNormalCompletion5 = (_step5 = _iterator5.next()).done); _iteratorNormalCompletion5 = true) {
            var material = _step5.value;

            var glMaterial = new _pbr.PbrMaterial();
            var pbr = material.pbrMetallicRoughness || {};

            glMaterial.baseColorFactor.value = pbr.baseColorFactor || [1, 1, 1, 1];
            glMaterial.baseColor.texture = getTexture(pbr.baseColorTexture);
            glMaterial.metallicRoughnessFactor.value = [pbr.metallicFactor || 1.0, pbr.roughnessFactor || 1.0];
            glMaterial.metallicRoughness.texture = getTexture(pbr.metallicRoughnessTexture);
            glMaterial.normal.texture = getTexture(json.normalTexture);
            glMaterial.occlusion.texture = getTexture(json.occlusionTexture);
            glMaterial.occlusionStrength.value = json.occlusionTexture && json.occlusionTexture.strength ? json.occlusionTexture.strength : 1.0;
            glMaterial.emissiveFactor.value = material.emissiveFactor || [0, 0, 0];
            glMaterial.emissive.texture = getTexture(json.emissiveTexture);
            if (!glMaterial.emissive.texture && json.emissiveFactor) {
              glMaterial.emissive.texture = new _texture.ColorTexture(1.0, 1.0, 1.0, 1.0);
            }

            switch (material.alphaMode) {
              case 'BLEND':
                glMaterial.state.blend = true;
                break;
              case 'MASK':
                // Not really supported.
                glMaterial.state.blend = true;
                break;
              default:
                // Includes 'OPAQUE'
                glMaterial.state.blend = false;
            }

            // glMaterial.alpha_mode = material.alphaMode;
            // glMaterial.alpha_cutoff = material.alphaCutoff;
            glMaterial.state.cullFace = !material.doubleSided;

            materials.push(glMaterial);
          }
        } catch (err) {
          _didIteratorError5 = true;
          _iteratorError5 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion5 && _iterator5.return) {
              _iterator5.return();
            }
          } finally {
            if (_didIteratorError5) {
              throw _iteratorError5;
            }
          }
        }
      }

      var accessors = json.accessors;

      var meshes = [];
      var _iteratorNormalCompletion6 = true;
      var _didIteratorError6 = false;
      var _iteratorError6 = undefined;

      try {
        for (var _iterator6 = json.meshes[Symbol.iterator](), _step6; !(_iteratorNormalCompletion6 = (_step6 = _iterator6.next()).done); _iteratorNormalCompletion6 = true) {
          var mesh = _step6.value;

          var glMesh = new Gltf2Mesh();
          meshes.push(glMesh);

          var _iteratorNormalCompletion8 = true;
          var _didIteratorError8 = false;
          var _iteratorError8 = undefined;

          try {
            for (var _iterator8 = mesh.primitives[Symbol.iterator](), _step8; !(_iteratorNormalCompletion8 = (_step8 = _iterator8.next()).done); _iteratorNormalCompletion8 = true) {
              var primitive = _step8.value;

              var _material = null;
              if ('material' in primitive) {
                _material = materials[primitive.material];
              } else {
                // Create a "default" material if the primitive has none.
                _material = new _pbr.PbrMaterial();
              }

              var attributes = [];
              var elementCount = 0;
              /* let glPrimitive = new Gltf2Primitive(primitive, material);
              glMesh.primitives.push(glPrimitive); */

              var min = null;
              var max = null;

              for (var name in primitive.attributes) {
                var accessor = accessors[primitive.attributes[name]];
                var _bufferView = bufferViews[accessor.bufferView];
                elementCount = accessor.count;

                var glAttribute = new _primitive.PrimitiveAttribute(name, _bufferView.renderBuffer(this.renderer, GL.ARRAY_BUFFER), getComponentCount(accessor.type), accessor.componentType, _bufferView.byteStride || 0, accessor.byteOffset || 0);
                glAttribute.normalized = accessor.normalized || false;

                if (name == 'POSITION') {
                  min = accessor.min;
                  max = accessor.max;
                }

                attributes.push(glAttribute);
              }

              var glPrimitive = new _primitive.Primitive(attributes, elementCount, primitive.mode);

              if ('indices' in primitive) {
                var _accessor = accessors[primitive.indices];
                var _bufferView2 = bufferViews[_accessor.bufferView];

                glPrimitive.setIndexBuffer(_bufferView2.renderBuffer(this.renderer, GL.ELEMENT_ARRAY_BUFFER), _accessor.byteOffset || 0, _accessor.componentType);
                glPrimitive.indexType = _accessor.componentType;
                glPrimitive.indexByteOffset = _accessor.byteOffset || 0;
                glPrimitive.elementCount = _accessor.count;
              }

              if (min && max) {
                glPrimitive.setBounds(min, max);
              }

              // After all the attributes have been processed, get a program that is
              // appropriate for both the material and the primitive attributes.
              glMesh.primitives.push(this.renderer.createRenderPrimitive(glPrimitive, _material));
            }
          } catch (err) {
            _didIteratorError8 = true;
            _iteratorError8 = err;
          } finally {
            try {
              if (!_iteratorNormalCompletion8 && _iterator8.return) {
                _iterator8.return();
              }
            } finally {
              if (_didIteratorError8) {
                throw _iteratorError8;
              }
            }
          }
        }
      } catch (err) {
        _didIteratorError6 = true;
        _iteratorError6 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion6 && _iterator6.return) {
            _iterator6.return();
          }
        } finally {
          if (_didIteratorError6) {
            throw _iteratorError6;
          }
        }
      }

      var sceneNode = new _node2.Node();
      var scene = json.scenes[json.scene];
      var _iteratorNormalCompletion7 = true;
      var _didIteratorError7 = false;
      var _iteratorError7 = undefined;

      try {
        for (var _iterator7 = scene.nodes[Symbol.iterator](), _step7; !(_iteratorNormalCompletion7 = (_step7 = _iterator7.next()).done); _iteratorNormalCompletion7 = true) {
          var nodeId = _step7.value;

          var node = json.nodes[nodeId];
          sceneNode.addNode(this.processNodes(node, json.nodes, meshes));
        }
      } catch (err) {
        _didIteratorError7 = true;
        _iteratorError7 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion7 && _iterator7.return) {
            _iterator7.return();
          }
        } finally {
          if (_didIteratorError7) {
            throw _iteratorError7;
          }
        }
      }

      return sceneNode;
    }
  }, {
    key: 'processNodes',
    value: function processNodes(node, nodes, meshes) {
      var glNode = new _node2.Node();
      glNode.name = node.name;

      if ('mesh' in node) {
        var mesh = meshes[node.mesh];
        var _iteratorNormalCompletion9 = true;
        var _didIteratorError9 = false;
        var _iteratorError9 = undefined;

        try {
          for (var _iterator9 = mesh.primitives[Symbol.iterator](), _step9; !(_iteratorNormalCompletion9 = (_step9 = _iterator9.next()).done); _iteratorNormalCompletion9 = true) {
            var primitive = _step9.value;

            glNode.addRenderPrimitive(primitive);
          }
        } catch (err) {
          _didIteratorError9 = true;
          _iteratorError9 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion9 && _iterator9.return) {
              _iterator9.return();
            }
          } finally {
            if (_didIteratorError9) {
              throw _iteratorError9;
            }
          }
        }
      }

      if (node.matrix) {
        glNode.matrix = new Float32Array(node.matrix);
      } else if (node.translation || node.rotation || node.scale) {
        if (node.translation) {
          glNode.translation = new Float32Array(node.translation);
        }

        if (node.rotation) {
          glNode.rotation = new Float32Array(node.rotation);
        }

        if (node.scale) {
          glNode.scale = new Float32Array(node.scale);
        }
      }

      if (node.children) {
        var _iteratorNormalCompletion10 = true;
        var _didIteratorError10 = false;
        var _iteratorError10 = undefined;

        try {
          for (var _iterator10 = node.children[Symbol.iterator](), _step10; !(_iteratorNormalCompletion10 = (_step10 = _iterator10.next()).done); _iteratorNormalCompletion10 = true) {
            var nodeId = _step10.value;

            var _node = nodes[nodeId];
            glNode.addNode(this.processNodes(_node, nodes, meshes));
          }
        } catch (err) {
          _didIteratorError10 = true;
          _iteratorError10 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion10 && _iterator10.return) {
              _iterator10.return();
            }
          } finally {
            if (_didIteratorError10) {
              throw _iteratorError10;
            }
          }
        }
      }

      return glNode;
    }
  }]);

  return Gltf2Loader;
}();

var Gltf2Mesh = function Gltf2Mesh() {
  _classCallCheck(this, Gltf2Mesh);

  this.primitives = [];
};

var Gltf2BufferView = function () {
  function Gltf2BufferView(json, buffers) {
    _classCallCheck(this, Gltf2BufferView);

    this.buffer = buffers[json.buffer];
    this.byteOffset = json.byteOffset || 0;
    this.byteLength = json.byteLength || null;
    this.byteStride = json.byteStride;

    this._viewPromise = null;
    this._renderBuffer = null;
  }

  _createClass(Gltf2BufferView, [{
    key: 'dataView',
    value: function dataView() {
      var _this2 = this;

      if (!this._viewPromise) {
        this._viewPromise = this.buffer.arrayBuffer().then(function (arrayBuffer) {
          return new DataView(arrayBuffer, _this2.byteOffset, _this2.byteLength);
        });
      }
      return this._viewPromise;
    }
  }, {
    key: 'renderBuffer',
    value: function renderBuffer(renderer, target) {
      if (!this._renderBuffer) {
        this._renderBuffer = renderer.createRenderBuffer(target, this.dataView());
      }
      return this._renderBuffer;
    }
  }]);

  return Gltf2BufferView;
}();

var Gltf2Resource = function () {
  function Gltf2Resource(json, baseUrl, arrayBuffer) {
    _classCallCheck(this, Gltf2Resource);

    this.json = json;
    this.baseUrl = baseUrl;

    this._dataPromise = null;
    this._texture = null;
    if (arrayBuffer) {
      this._dataPromise = Promise.resolve(arrayBuffer);
    }
  }

  _createClass(Gltf2Resource, [{
    key: 'arrayBuffer',
    value: function arrayBuffer() {
      if (!this._dataPromise) {
        if (isDataUri(this.json.uri)) {
          var base64String = this.json.uri.replace('data:application/octet-stream;base64,', '');
          var binaryArray = Uint8Array.from(atob(base64String), function (c) {
            return c.charCodeAt(0);
          });
          this._dataPromise = Promise.resolve(binaryArray.buffer);
          return this._dataPromise;
        }

        this._dataPromise = fetch(resolveUri(this.json.uri, this.baseUrl)).then(function (response) {
          return response.arrayBuffer();
        });
      }
      return this._dataPromise;
    }
  }, {
    key: 'texture',
    value: function texture(bufferViews) {
      var _this3 = this;

      if (!this._texture) {
        var img = new Image();
        this._texture = new _texture.ImageTexture(img);

        if (this.json.uri) {
          if (isDataUri(this.json.uri)) {
            img.src = this.json.uri;
          } else {
            img.src = '' + this.baseUrl + this.json.uri;
          }
        } else {
          var view = bufferViews[this.json.bufferView];
          view.dataView().then(function (dataView) {
            var blob = new Blob([dataView], { type: _this3.json.mimeType });
            img.src = window.URL.createObjectURL(blob);
          });
        }
      }
      return this._texture;
    }
  }]);

  return Gltf2Resource;
}();

/***/ }),

/***/ "./src/materials/pbr.js":
/*!******************************!*\
  !*** ./src/materials/pbr.js ***!
  \******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.PbrMaterial = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _renderer = __webpack_require__(/*! ../core/renderer.js */ "./src/core/renderer.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var VERTEX_SOURCE = '\nattribute vec3 POSITION, NORMAL;\nattribute vec2 TEXCOORD_0, TEXCOORD_1;\n\nuniform vec3 CAMERA_POSITION;\nuniform vec3 LIGHT_DIRECTION;\n\nvarying vec3 vLight; // Vector from vertex to light.\nvarying vec3 vView; // Vector from vertex to camera.\nvarying vec2 vTex;\n\n#ifdef USE_NORMAL_MAP\nattribute vec4 TANGENT;\nvarying mat3 vTBN;\n#else\nvarying vec3 vNorm;\n#endif\n\n#ifdef USE_VERTEX_COLOR\nattribute vec4 COLOR_0;\nvarying vec4 vCol;\n#endif\n\nvec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n  vec3 n = normalize(vec3(model * vec4(NORMAL, 0.0)));\n#ifdef USE_NORMAL_MAP\n  vec3 t = normalize(vec3(model * vec4(TANGENT.xyz, 0.0)));\n  vec3 b = cross(n, t) * TANGENT.w;\n  vTBN = mat3(t, b, n);\n#else\n  vNorm = n;\n#endif\n\n#ifdef USE_VERTEX_COLOR\n  vCol = COLOR_0;\n#endif\n\n  vTex = TEXCOORD_0;\n  vec4 mPos = model * vec4(POSITION, 1.0);\n  vLight = -LIGHT_DIRECTION;\n  vView = CAMERA_POSITION - mPos.xyz;\n  return proj * view * mPos;\n}';

// These equations are borrowed with love from this docs from Epic because I
// just don't have anything novel to bring to the PBR scene.
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
var EPIC_PBR_FUNCTIONS = '\nvec3 lambertDiffuse(vec3 cDiff) {\n  return cDiff / M_PI;\n}\n\nfloat specD(float a, float nDotH) {\n  float aSqr = a * a;\n  float f = ((nDotH * nDotH) * (aSqr - 1.0) + 1.0);\n  return aSqr / (M_PI * f * f);\n}\n\nfloat specG(float roughness, float nDotL, float nDotV) {\n  float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;\n  float gl = nDotL / (nDotL * (1.0 - k) + k);\n  float gv = nDotV / (nDotV * (1.0 - k) + k);\n  return gl * gv;\n}\n\nvec3 specF(float vDotH, vec3 F0) {\n  float exponent = (-5.55473 * vDotH - 6.98316) * vDotH;\n  float base = 2.0;\n  return F0 + (1.0 - F0) * pow(base, exponent);\n}';

var FRAGMENT_SOURCE = '\n#define M_PI 3.14159265\n\nuniform vec4 baseColorFactor;\n#ifdef USE_BASE_COLOR_MAP\nuniform sampler2D baseColorTex;\n#endif\n\nvarying vec3 vLight;\nvarying vec3 vView;\nvarying vec2 vTex;\n\n#ifdef USE_VERTEX_COLOR\nvarying vec4 vCol;\n#endif\n\n#ifdef USE_NORMAL_MAP\nuniform sampler2D normalTex;\nvarying mat3 vTBN;\n#else\nvarying vec3 vNorm;\n#endif\n\n#ifdef USE_METAL_ROUGH_MAP\nuniform sampler2D metallicRoughnessTex;\n#endif\nuniform vec2 metallicRoughnessFactor;\n\n#ifdef USE_OCCLUSION\nuniform sampler2D occlusionTex;\nuniform float occlusionStrength;\n#endif\n\n#ifdef USE_EMISSIVE_TEXTURE\nuniform sampler2D emissiveTex;\n#endif\nuniform vec3 emissiveFactor;\n\nuniform vec3 LIGHT_COLOR;\n\nconst vec3 dielectricSpec = vec3(0.04);\nconst vec3 black = vec3(0.0);\n\n' + EPIC_PBR_FUNCTIONS + '\n\nvec4 fragment_main() {\n#ifdef USE_BASE_COLOR_MAP\n  vec4 baseColor = texture2D(baseColorTex, vTex) * baseColorFactor;\n#else\n  vec4 baseColor = baseColorFactor;\n#endif\n\n#ifdef USE_VERTEX_COLOR\n  baseColor *= vCol;\n#endif\n\n#ifdef USE_NORMAL_MAP\n  vec3 n = texture2D(normalTex, vTex).rgb;\n  n = normalize(vTBN * (2.0 * n - 1.0));\n#else\n  vec3 n = normalize(vNorm);\n#endif\n\n#ifdef FULLY_ROUGH\n  float metallic = 0.0;\n#else\n  float metallic = metallicRoughnessFactor.x;\n#endif\n\n  float roughness = metallicRoughnessFactor.y;\n\n#ifdef USE_METAL_ROUGH_MAP\n  vec4 metallicRoughness = texture2D(metallicRoughnessTex, vTex);\n  metallic *= metallicRoughness.b;\n  roughness *= metallicRoughness.g;\n#endif\n  \n  vec3 l = normalize(vLight);\n  vec3 v = normalize(vView);\n  vec3 h = normalize(l+v);\n\n  float nDotL = clamp(dot(n, l), 0.001, 1.0);\n  float nDotV = abs(dot(n, v)) + 0.001;\n  float nDotH = max(dot(n, h), 0.0);\n  float vDotH = max(dot(v, h), 0.0);\n\n  // From GLTF Spec\n  vec3 cDiff = mix(baseColor.rgb * (1.0 - dielectricSpec.r), black, metallic); // Diffuse color\n  vec3 F0 = mix(dielectricSpec, baseColor.rgb, metallic); // Specular color\n  float a = roughness * roughness;\n\n#ifdef FULLY_ROUGH\n  vec3 specular = F0 * 0.45;\n#else\n  vec3 F = specF(vDotH, F0);\n  float D = specD(a, nDotH);\n  float G = specG(roughness, nDotL, nDotV);\n  vec3 specular = (D * F * G) / (4.0 * nDotL * nDotV);\n#endif\n  float halfLambert = dot(n, l) * 0.5 + 0.5;\n  halfLambert *= halfLambert;\n\n  vec3 color = (halfLambert * LIGHT_COLOR * lambertDiffuse(cDiff)) + specular;\n\n#ifdef USE_OCCLUSION\n  float occlusion = texture2D(occlusionTex, vTex).r;\n  color = mix(color, color * occlusion, occlusionStrength);\n#endif\n  \n  vec3 emissive = emissiveFactor;\n#ifdef USE_EMISSIVE_TEXTURE\n  emissive *= texture2D(emissiveTex, vTex).rgb;\n#endif\n  color += emissive;\n\n  // gamma correction\n  color = pow(color, vec3(1.0/2.2));\n\n  return vec4(color, baseColor.a);\n}';

var PbrMaterial = exports.PbrMaterial = function (_Material) {
  _inherits(PbrMaterial, _Material);

  function PbrMaterial() {
    _classCallCheck(this, PbrMaterial);

    var _this = _possibleConstructorReturn(this, (PbrMaterial.__proto__ || Object.getPrototypeOf(PbrMaterial)).call(this));

    _this.baseColor = _this.defineSampler('baseColorTex');
    _this.metallicRoughness = _this.defineSampler('metallicRoughnessTex');
    _this.normal = _this.defineSampler('normalTex');
    _this.occlusion = _this.defineSampler('occlusionTex');
    _this.emissive = _this.defineSampler('emissiveTex');

    _this.baseColorFactor = _this.defineUniform('baseColorFactor', [1.0, 1.0, 1.0, 1.0]);
    _this.metallicRoughnessFactor = _this.defineUniform('metallicRoughnessFactor', [1.0, 1.0]);
    _this.occlusionStrength = _this.defineUniform('occlusionStrength', 1.0);
    _this.emissiveFactor = _this.defineUniform('emissiveFactor', [0, 0, 0]);
    return _this;
  }

  _createClass(PbrMaterial, [{
    key: 'getProgramDefines',
    value: function getProgramDefines(renderPrimitive) {
      var programDefines = {};

      if (renderPrimitive._attributeMask & _renderer.ATTRIB_MASK.COLOR_0) {
        programDefines['USE_VERTEX_COLOR'] = 1;
      }

      if (renderPrimitive._attributeMask & _renderer.ATTRIB_MASK.TEXCOORD_0) {
        if (this.baseColor.texture) {
          programDefines['USE_BASE_COLOR_MAP'] = 1;
        }

        if (this.normal.texture && renderPrimitive._attributeMask & _renderer.ATTRIB_MASK.TANGENT) {
          programDefines['USE_NORMAL_MAP'] = 1;
        }

        if (this.metallicRoughness.texture) {
          programDefines['USE_METAL_ROUGH_MAP'] = 1;
        }

        if (this.occlusion.texture) {
          programDefines['USE_OCCLUSION'] = 1;
        }

        if (this.emissive.texture) {
          programDefines['USE_EMISSIVE_TEXTURE'] = 1;
        }
      }

      if ((!this.metallicRoughness.texture || !(renderPrimitive._attributeMask & _renderer.ATTRIB_MASK.TEXCOORD_0)) && this.metallicRoughnessFactor.value[1] == 1.0) {
        programDefines['FULLY_ROUGH'] = 1;
      }

      return programDefines;
    }
  }, {
    key: 'materialName',
    get: function get() {
      return 'PBR';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return VERTEX_SOURCE;
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return FRAGMENT_SOURCE;
    }
  }]);

  return PbrMaterial;
}(_material.Material);

/***/ }),

/***/ "./src/math/gl-matrix.js":
/*!*******************************!*\
  !*** ./src/math/gl-matrix.js ***!
  \*******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.vec4 = exports.vec3 = exports.vec2 = exports.quat2 = exports.quat = exports.mat4 = exports.mat3 = exports.mat2d = exports.mat2 = exports.glMatrix = undefined;

var _common = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/common.js */ "./node_modules/gl-matrix/src/gl-matrix/common.js");

var glMatrix = _interopRequireWildcard(_common);

var _mat = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/mat2.js */ "./node_modules/gl-matrix/src/gl-matrix/mat2.js");

var mat2 = _interopRequireWildcard(_mat);

var _mat2d = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/mat2d.js */ "./node_modules/gl-matrix/src/gl-matrix/mat2d.js");

var mat2d = _interopRequireWildcard(_mat2d);

var _mat2 = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/mat3.js */ "./node_modules/gl-matrix/src/gl-matrix/mat3.js");

var mat3 = _interopRequireWildcard(_mat2);

var _mat3 = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/mat4.js */ "./node_modules/gl-matrix/src/gl-matrix/mat4.js");

var mat4 = _interopRequireWildcard(_mat3);

var _quat = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/quat.js */ "./node_modules/gl-matrix/src/gl-matrix/quat.js");

var quat = _interopRequireWildcard(_quat);

var _quat2 = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/quat2.js */ "./node_modules/gl-matrix/src/gl-matrix/quat2.js");

var quat2 = _interopRequireWildcard(_quat2);

var _vec = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/vec2.js */ "./node_modules/gl-matrix/src/gl-matrix/vec2.js");

var vec2 = _interopRequireWildcard(_vec);

var _vec2 = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/vec3.js */ "./node_modules/gl-matrix/src/gl-matrix/vec3.js");

var vec3 = _interopRequireWildcard(_vec2);

var _vec3 = __webpack_require__(/*! ../../node_modules/gl-matrix/src/gl-matrix/vec4.js */ "./node_modules/gl-matrix/src/gl-matrix/vec4.js");

var vec4 = _interopRequireWildcard(_vec3);

function _interopRequireWildcard(obj) { if (obj && obj.__esModule) { return obj; } else { var newObj = {}; if (obj != null) { for (var key in obj) { if (Object.prototype.hasOwnProperty.call(obj, key)) newObj[key] = obj[key]; } } newObj.default = obj; return newObj; } }

// Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

exports.glMatrix = glMatrix;
exports.mat2 = mat2;
exports.mat2d = mat2d;
exports.mat3 = mat3;
exports.mat4 = mat4;
exports.quat = quat;
exports.quat2 = quat2;
exports.vec2 = vec2;
exports.vec3 = vec3;
exports.vec4 = vec4;

/***/ }),

/***/ "./src/nodes/bounds-renderer.js":
/*!**************************************!*\
  !*** ./src/nodes/bounds-renderer.js ***!
  \**************************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.BoundsRenderer = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
This file renders a passed in XRStageBounds object and attempts
to render geometry on the floor to indicate where the bounds is.
XRStageBounds' `geometry` is a series of XRStageBoundsPoints (in
clockwise-order) with `x` and `z` properties for each.
*/

var GL = WebGLRenderingContext; // For enums

var BoundsMaterial = function (_Material) {
  _inherits(BoundsMaterial, _Material);

  function BoundsMaterial() {
    _classCallCheck(this, BoundsMaterial);

    var _this = _possibleConstructorReturn(this, (BoundsMaterial.__proto__ || Object.getPrototypeOf(BoundsMaterial)).call(this));

    _this.state.blend = true;
    _this.state.blendFuncSrc = GL.SRC_ALPHA;
    _this.state.blendFuncDst = GL.ONE;
    _this.state.depthTest = false;
    return _this;
  }

  _createClass(BoundsMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'BOUNDS_RENDERER';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec2 POSITION;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      return proj * view * model * vec4(POSITION, 1.0);\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    precision mediump float;\n\n    vec4 fragment_main() {\n      return vec4(0.0, 1.0, 0.0, 0.3);\n    }';
    }
  }]);

  return BoundsMaterial;
}(_material.Material);

var BoundsRenderer = exports.BoundsRenderer = function (_Node) {
  _inherits(BoundsRenderer, _Node);

  function BoundsRenderer() {
    _classCallCheck(this, BoundsRenderer);

    var _this2 = _possibleConstructorReturn(this, (BoundsRenderer.__proto__ || Object.getPrototypeOf(BoundsRenderer)).call(this));

    _this2._stageBounds = null;
    return _this2;
  }

  _createClass(BoundsRenderer, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      this.stageBounds = this._stageBounds;
    }
  }, {
    key: 'stageBounds',
    get: function get() {
      return this._stageBounds;
    },
    set: function set(stageBounds) {
      if (this._stageBounds) {
        this.clearRenderPrimitives();
      }
      this._stageBounds = stageBounds;
      if (!stageBounds || stageBounds.length === 0 || !this._renderer) {
        return;
      }

      var verts = [];
      var indices = [];

      // Tessellate the bounding points from XRStageBounds and connect
      // each point to a neighbor and 0,0,0.
      var pointCount = stageBounds.geometry.length;
      for (var i = 0; i < pointCount; i++) {
        var point = stageBounds.geometry[i];
        verts.push(point.x, 0, point.z);
        indices.push(i, i === 0 ? pointCount - 1 : i - 1, pointCount);
      }
      // Center point
      verts.push(0, 0, 0);

      var vertexBuffer = this._renderer.createRenderBuffer(GL.ARRAY_BUFFER, new Float32Array(verts));
      var indexBuffer = this._renderer.createRenderBuffer(GL.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices));

      var attribs = [new _primitive.PrimitiveAttribute('POSITION', vertexBuffer, 3, GL.FLOAT, 12, 0)];

      var primitive = new _primitive.Primitive(attribs, indices.length);
      primitive.setIndexBuffer(indexBuffer);

      var renderPrimitive = this._renderer.createRenderPrimitive(primitive, new BoundsMaterial());
      this.addRenderPrimitive(renderPrimitive);
    }
  }]);

  return BoundsRenderer;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/button.js":
/*!*****************************!*\
  !*** ./src/nodes/button.js ***!
  \*****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ButtonNode = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _primitiveStream = __webpack_require__(/*! ../geometry/primitive-stream.js */ "./src/geometry/primitive-stream.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var BUTTON_SIZE = 0.1;
var BUTTON_CORNER_RADIUS = 0.025;
var BUTTON_CORNER_SEGMENTS = 8;
var BUTTON_ICON_SIZE = 0.07;
var BUTTON_LAYER_DISTANCE = 0.005;
var BUTTON_COLOR = 0.75;
var BUTTON_ALPHA = 0.85;
var BUTTON_HOVER_COLOR = 0.9;
var BUTTON_HOVER_ALPHA = 1.0;
var BUTTON_HOVER_SCALE = 1.1;
var BUTTON_HOVER_TRANSITION_TIME_MS = 200;

var ButtonMaterial = function (_Material) {
  _inherits(ButtonMaterial, _Material);

  function ButtonMaterial() {
    _classCallCheck(this, ButtonMaterial);

    var _this = _possibleConstructorReturn(this, (ButtonMaterial.__proto__ || Object.getPrototypeOf(ButtonMaterial)).call(this));

    _this.state.blend = true;

    _this.defineUniform('hoverAmount', 0);
    return _this;
  }

  _createClass(ButtonMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'BUTTON_MATERIAL';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec3 POSITION;\n\n    uniform float hoverAmount;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      float scale = mix(1.0, ' + BUTTON_HOVER_SCALE + ', hoverAmount);\n      vec4 pos = vec4(POSITION.x * scale, POSITION.y * scale, POSITION.z * (scale + (hoverAmount * 0.2)), 1.0);\n      return proj * view * model * pos;\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    uniform float hoverAmount;\n\n    const vec4 default_color = vec4(' + BUTTON_COLOR + ', ' + BUTTON_COLOR + ', ' + BUTTON_COLOR + ', ' + BUTTON_ALPHA + ');\n    const vec4 hover_color = vec4(' + BUTTON_HOVER_COLOR + ', ' + BUTTON_HOVER_COLOR + ',\n                                  ' + BUTTON_HOVER_COLOR + ', ' + BUTTON_HOVER_ALPHA + ');\n\n    vec4 fragment_main() {\n      return mix(default_color, hover_color, hoverAmount);\n    }';
    }
  }]);

  return ButtonMaterial;
}(_material.Material);

var ButtonIconMaterial = function (_Material2) {
  _inherits(ButtonIconMaterial, _Material2);

  function ButtonIconMaterial() {
    _classCallCheck(this, ButtonIconMaterial);

    var _this2 = _possibleConstructorReturn(this, (ButtonIconMaterial.__proto__ || Object.getPrototypeOf(ButtonIconMaterial)).call(this));

    _this2.state.blend = true;

    _this2.defineUniform('hoverAmount', 0);
    _this2.icon = _this2.defineSampler('icon');
    return _this2;
  }

  _createClass(ButtonIconMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'BUTTON_ICON_MATERIAL';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec3 POSITION;\n    attribute vec2 TEXCOORD_0;\n\n    uniform float hoverAmount;\n\n    varying vec2 vTexCoord;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      vTexCoord = TEXCOORD_0;\n      float scale = mix(1.0, ' + BUTTON_HOVER_SCALE + ', hoverAmount);\n      vec4 pos = vec4(POSITION.x * scale, POSITION.y * scale, POSITION.z * (scale + (hoverAmount * 0.2)), 1.0);\n      return proj * view * model * pos;\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    uniform sampler2D icon;\n    varying vec2 vTexCoord;\n\n    vec4 fragment_main() {\n      return texture2D(icon, vTexCoord);\n    }';
    }
  }]);

  return ButtonIconMaterial;
}(_material.Material);

var ButtonNode = exports.ButtonNode = function (_Node) {
  _inherits(ButtonNode, _Node);

  function ButtonNode(iconTexture, callback) {
    _classCallCheck(this, ButtonNode);

    // All buttons are selectable by default.
    var _this3 = _possibleConstructorReturn(this, (ButtonNode.__proto__ || Object.getPrototypeOf(ButtonNode)).call(this));

    _this3.selectable = true;

    _this3._selectHandler = callback;
    _this3._iconTexture = iconTexture;
    _this3._hovered = false;
    _this3._hoverT = 0;
    return _this3;
  }

  _createClass(ButtonNode, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      var stream = new _primitiveStream.PrimitiveStream();

      var hd = BUTTON_LAYER_DISTANCE * 0.5;

      // Build a rounded rect for the background.
      var hs = BUTTON_SIZE * 0.5;
      var ihs = hs - BUTTON_CORNER_RADIUS;
      stream.startGeometry();

      // Rounded corners and sides
      var segments = BUTTON_CORNER_SEGMENTS * 4;
      for (var i = 0; i < segments; ++i) {
        var rad = i * (Math.PI * 2.0 / segments);
        var x = Math.cos(rad) * BUTTON_CORNER_RADIUS;
        var y = Math.sin(rad) * BUTTON_CORNER_RADIUS;
        var section = Math.floor(i / BUTTON_CORNER_SEGMENTS);
        switch (section) {
          case 0:
            x += ihs;
            y += ihs;
            break;
          case 1:
            x -= ihs;
            y += ihs;
            break;
          case 2:
            x -= ihs;
            y -= ihs;
            break;
          case 3:
            x += ihs;
            y -= ihs;
            break;
        }

        stream.pushVertex(x, y, -hd, 0, 0, 0, 0, 1);

        if (i > 1) {
          stream.pushTriangle(0, i - 1, i);
        }
      }

      stream.endGeometry();

      var buttonPrimitive = stream.finishPrimitive(renderer);
      this._buttonRenderPrimitive = renderer.createRenderPrimitive(buttonPrimitive, new ButtonMaterial());
      this.addRenderPrimitive(this._buttonRenderPrimitive);

      // Build a simple textured quad for the foreground.
      hs = BUTTON_ICON_SIZE * 0.5;
      stream.clear();
      stream.startGeometry();

      stream.pushVertex(-hs, hs, hd, 0, 0, 0, 0, 1);
      stream.pushVertex(-hs, -hs, hd, 0, 1, 0, 0, 1);
      stream.pushVertex(hs, -hs, hd, 1, 1, 0, 0, 1);
      stream.pushVertex(hs, hs, hd, 1, 0, 0, 0, 1);

      stream.pushTriangle(0, 1, 2);
      stream.pushTriangle(0, 2, 3);

      stream.endGeometry();

      var iconPrimitive = stream.finishPrimitive(renderer);
      var iconMaterial = new ButtonIconMaterial();
      iconMaterial.icon.texture = this._iconTexture;
      this._iconRenderPrimitive = renderer.createRenderPrimitive(iconPrimitive, iconMaterial);
      this.addRenderPrimitive(this._iconRenderPrimitive);
    }
  }, {
    key: 'onHoverStart',
    value: function onHoverStart() {
      this._hovered = true;
    }
  }, {
    key: 'onHoverEnd',
    value: function onHoverEnd() {
      this._hovered = false;
    }
  }, {
    key: '_updateHoverState',
    value: function _updateHoverState() {
      var t = this._hoverT / BUTTON_HOVER_TRANSITION_TIME_MS;
      // Cubic Ease In/Out
      // TODO: Get a better animation system
      var hoverAmount = t < .5 ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
      this._buttonRenderPrimitive.uniforms.hoverAmount.value = hoverAmount;
      this._iconRenderPrimitive.uniforms.hoverAmount.value = hoverAmount;
    }
  }, {
    key: 'onUpdate',
    value: function onUpdate(timestamp, frameDelta) {
      if (this._hovered && this._hoverT < BUTTON_HOVER_TRANSITION_TIME_MS) {
        this._hoverT = Math.min(BUTTON_HOVER_TRANSITION_TIME_MS, this._hoverT + frameDelta);
        this._updateHoverState();
      } else if (!this._hovered && this._hoverT > 0) {
        this._hoverT = Math.max(0.0, this._hoverT - frameDelta);
        this._updateHoverState();
      }
    }
  }, {
    key: 'iconTexture',
    get: function get() {
      return this._iconTexture;
    },
    set: function set(value) {
      if (this._iconTexture == value) {
        return;
      }

      this._iconTexture = value;
      this._iconRenderPrimitive.samplers.icon.texture = value;
    }
  }]);

  return ButtonNode;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/cube-sea.js":
/*!*******************************!*\
  !*** ./src/nodes/cube-sea.js ***!
  \*******************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.CubeSeaNode = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _texture = __webpack_require__(/*! ../core/texture.js */ "./src/core/texture.js");

var _boxBuilder = __webpack_require__(/*! ../geometry/box-builder.js */ "./src/geometry/box-builder.js");

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var CubeSeaMaterial = function (_Material) {
  _inherits(CubeSeaMaterial, _Material);

  function CubeSeaMaterial() {
    var heavy = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : false;

    _classCallCheck(this, CubeSeaMaterial);

    var _this = _possibleConstructorReturn(this, (CubeSeaMaterial.__proto__ || Object.getPrototypeOf(CubeSeaMaterial)).call(this));

    _this.heavy = heavy;

    _this.baseColor = _this.defineSampler('baseColor');
    return _this;
  }

  _createClass(CubeSeaMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'CUBE_SEA';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec3 POSITION;\n    attribute vec2 TEXCOORD_0;\n    attribute vec3 NORMAL;\n\n    varying vec2 vTexCoord;\n    varying vec3 vLight;\n\n    const vec3 lightDir = vec3(0.75, 0.5, 1.0);\n    const vec3 ambientColor = vec3(0.5, 0.5, 0.5);\n    const vec3 lightColor = vec3(0.75, 0.75, 0.75);\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      vec3 normalRotated = vec3(model * vec4(NORMAL, 0.0));\n      float lightFactor = max(dot(normalize(lightDir), normalRotated), 0.0);\n      vLight = ambientColor + (lightColor * lightFactor);\n      vTexCoord = TEXCOORD_0;\n      return proj * view * model * vec4(POSITION, 1.0);\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      if (!this.heavy) {
        return '\n      precision mediump float;\n      uniform sampler2D baseColor;\n      varying vec2 vTexCoord;\n      varying vec3 vLight;\n\n      vec4 fragment_main() {\n        return vec4(vLight, 1.0) * texture2D(baseColor, vTexCoord);\n      }';
      } else {
        // Used when we want to stress the GPU a bit more.
        // Stolen with love from https://www.clicktorelease.com/code/codevember-2016/4/
        return '\n      precision mediump float;\n\n      uniform sampler2D diffuse;\n      varying vec2 vTexCoord;\n      varying vec3 vLight;\n\n      vec2 dimensions = vec2(64, 64);\n      float seed = 0.42;\n\n      vec2 hash( vec2 p ) {\n        p=vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)));\n        return fract(sin(p)*18.5453);\n      }\n\n      vec3 hash3( vec2 p ) {\n          vec3 q = vec3( dot(p,vec2(127.1,311.7)),\n                 dot(p,vec2(269.5,183.3)),\n                 dot(p,vec2(419.2,371.9)) );\n        return fract(sin(q)*43758.5453);\n      }\n\n      float iqnoise( in vec2 x, float u, float v ) {\n        vec2 p = floor(x);\n        vec2 f = fract(x);\n        float k = 1.0+63.0*pow(1.0-v,4.0);\n        float va = 0.0;\n        float wt = 0.0;\n        for( int j=-2; j<=2; j++ )\n          for( int i=-2; i<=2; i++ ) {\n            vec2 g = vec2( float(i),float(j) );\n            vec3 o = hash3( p + g )*vec3(u,u,1.0);\n            vec2 r = g - f + o.xy;\n            float d = dot(r,r);\n            float ww = pow( 1.0-smoothstep(0.0,1.414,sqrt(d)), k );\n            va += o.z*ww;\n            wt += ww;\n          }\n        return va/wt;\n      }\n\n      // return distance, and cell id\n      vec2 voronoi( in vec2 x ) {\n        vec2 n = floor( x );\n        vec2 f = fract( x );\n        vec3 m = vec3( 8.0 );\n        for( int j=-1; j<=1; j++ )\n          for( int i=-1; i<=1; i++ ) {\n            vec2  g = vec2( float(i), float(j) );\n            vec2  o = hash( n + g );\n            vec2  r = g - f + (0.5+0.5*sin(seed+6.2831*o));\n            float d = dot( r, r );\n            if( d<m.x )\n              m = vec3( d, o );\n          }\n        return vec2( sqrt(m.x), m.y+m.z );\n      }\n\n      vec4 fragment_main() {\n        vec2 uv = ( vTexCoord );\n        uv *= vec2( 10., 10. );\n        uv += seed;\n        vec2 p = 0.5 - 0.5*sin( 0.*vec2(1.01,1.71) );\n\n        vec2 c = voronoi( uv );\n        vec3 col = vec3( c.y / 2. );\n\n        float f = iqnoise( 1. * uv + c.y, p.x, p.y );\n        col *= 1.0 + .25 * vec3( f );\n\n        return vec4(vLight, 1.0) * texture2D(diffuse, vTexCoord) * vec4( col, 1. );\n      }';
      }
    }
  }]);

  return CubeSeaMaterial;
}(_material.Material);

var CubeSeaNode = exports.CubeSeaNode = function (_Node) {
  _inherits(CubeSeaNode, _Node);

  function CubeSeaNode() {
    var options = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : {};

    _classCallCheck(this, CubeSeaNode);

    // Test variables
    // If true, use a very heavyweight shader to stress the GPU.
    var _this2 = _possibleConstructorReturn(this, (CubeSeaNode.__proto__ || Object.getPrototypeOf(CubeSeaNode)).call(this));

    _this2.heavyGpu = !!options.heavyGpu;

    // Number and size of the static cubes. Warning, large values
    // don't render right due to overflow of the int16 indices.
    _this2.cubeCount = options.cubeCount || (_this2.heavyGpu ? 12 : 10);
    _this2.cubeScale = options.cubeScale || 1.0;

    // Draw only half the world cubes. Helps test variable render cost
    // when combined with heavyGpu.
    _this2.halfOnly = !!options.halfOnly;

    // Automatically spin the world cubes. Intended for automated testing,
    // not recommended for viewing in a headset.
    _this2.autoRotate = !!options.autoRotate;

    _this2._texture = new _texture.UrlTexture(options.imageUrl || 'media/textures/cube-sea.png');

    _this2._material = new CubeSeaMaterial(_this2.heavyGpu);
    _this2._material.baseColor.texture = _this2._texture;

    _this2._renderPrimitive = null;
    return _this2;
  }

  _createClass(CubeSeaNode, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      this._renderPrimitive = null;

      var boxBuilder = new _boxBuilder.BoxBuilder();

      // Build the spinning "hero" cubes
      boxBuilder.pushCube([0, 0.25, -0.8], 0.1);
      boxBuilder.pushCube([0.8, 0.25, 0], 0.1);
      boxBuilder.pushCube([0, 0.25, 0.8], 0.1);
      boxBuilder.pushCube([-0.8, 0.25, 0], 0.1);

      var heroPrimitive = boxBuilder.finishPrimitive(renderer);

      this.heroNode = renderer.createMesh(heroPrimitive, this._material);

      this.rebuildCubes(boxBuilder);

      this.cubeSeaNode = new _node.Node();
      this.cubeSeaNode.addRenderPrimitive(this._renderPrimitive);

      this.addNode(this.cubeSeaNode);
      this.addNode(this.heroNode);

      return this.waitForComplete();
    }
  }, {
    key: 'rebuildCubes',
    value: function rebuildCubes(boxBuilder) {
      if (!this._renderer) {
        return;
      }

      if (!boxBuilder) {
        boxBuilder = new _boxBuilder.BoxBuilder();
      } else {
        boxBuilder.clear();
      }

      var size = 0.4 * this.cubeScale;

      // Build the cube sea
      var halfGrid = this.cubeCount * 0.5;
      for (var x = 0; x < this.cubeCount; ++x) {
        for (var y = 0; y < this.cubeCount; ++y) {
          for (var z = 0; z < this.cubeCount; ++z) {
            var pos = [x - halfGrid, y - halfGrid, z - halfGrid];
            // Only draw cubes on one side. Useful for testing variable render
            // cost that depends on view direction.
            if (this.halfOnly && pos[0] < 0) {
              continue;
            }

            // Don't place a cube in the center of the grid.
            if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0) {
              continue;
            }

            boxBuilder.pushCube(pos, size);
          }
        }
      }

      if (this.cubeCount > 12) {
        // Each cube has 6 sides with 2 triangles and 3 indices per triangle, so
        // the total number of indices needed is cubeCount^3 * 36. This exceeds
        // the short index range past 12 cubes.
        boxBuilder.indexType = 5125; // gl.UNSIGNED_INT
      }
      var cubeSeaPrimitive = boxBuilder.finishPrimitive(this._renderer);

      if (!this._renderPrimitive) {
        this._renderPrimitive = this._renderer.createRenderPrimitive(cubeSeaPrimitive, this._material);
      } else {
        this._renderPrimitive.setPrimitive(cubeSeaPrimitive);
      }
    }
  }, {
    key: 'onUpdate',
    value: function onUpdate(timestamp, frameDelta) {
      if (this.autoRotate) {
        _glMatrix.mat4.fromRotation(this.cubeSeaNode.matrix, timestamp / 500, [0, -1, 0]);
      }
      _glMatrix.mat4.fromRotation(this.heroNode.matrix, timestamp / 2000, [0, 1, 0]);
    }
  }]);

  return CubeSeaNode;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/gltf2.js":
/*!****************************!*\
  !*** ./src/nodes/gltf2.js ***!
  \****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Gltf2Node = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _gltf = __webpack_require__(/*! ../loaders/gltf2.js */ "./src/loaders/gltf2.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Using a weak map here allows us to cache a loader per-renderer without
// modifying the renderer object or leaking memory when it's garbage collected.
var gltfLoaderMap = new WeakMap();

var Gltf2Node = exports.Gltf2Node = function (_Node) {
  _inherits(Gltf2Node, _Node);

  function Gltf2Node(options) {
    _classCallCheck(this, Gltf2Node);

    var _this = _possibleConstructorReturn(this, (Gltf2Node.__proto__ || Object.getPrototypeOf(Gltf2Node)).call(this));

    _this._url = options.url;

    _this._promise = null;
    _this._resolver = null;
    _this._rejecter = null;
    return _this;
  }

  _createClass(Gltf2Node, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      var _this2 = this;

      var loader = gltfLoaderMap.get(renderer);
      if (!loader) {
        loader = new _gltf.Gltf2Loader(renderer);
        gltfLoaderMap.set(renderer, loader);
      }

      // Do we have a previously resolved promise? If so clear it.
      if (!this._resolver && this._promise) {
        this._promise = null;
      }

      this._ensurePromise();

      loader.loadFromUrl(this._url).then(function (sceneNode) {
        _this2.addNode(sceneNode);
        _this2._resolver(sceneNode.waitForComplete());
        _this2._resolver = null;
        _this2._rejecter = null;
      }).catch(function (err) {
        _this2._rejecter(err);
        _this2._resolver = null;
        _this2._rejecter = null;
      });
    }
  }, {
    key: '_ensurePromise',
    value: function _ensurePromise() {
      var _this3 = this;

      if (!this._promise) {
        this._promise = new Promise(function (resolve, reject) {
          _this3._resolver = resolve;
          _this3._rejecter = reject;
        });
      }
      return this._promise;
    }
  }, {
    key: 'waitForComplete',
    value: function waitForComplete() {
      return this._ensurePromise();
    }
  }]);

  return Gltf2Node;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/input-renderer.js":
/*!*************************************!*\
  !*** ./src/nodes/input-renderer.js ***!
  \*************************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.InputRenderer = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

var _texture = __webpack_require__(/*! ../core/texture.js */ "./src/core/texture.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var GL = WebGLRenderingContext; // For enums

// Laser texture data, 48x1 RGBA (not premultiplied alpha). This represents a
// "cross section" of the laser beam with a bright core and a feathered edge.
// Borrowed from Chromium source code.
var LASER_TEXTURE_DATA = new Uint8Array([0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff, 0x02, 0xbf, 0xbf, 0xbf, 0x04, 0xcc, 0xcc, 0xcc, 0x05, 0xdb, 0xdb, 0xdb, 0x07, 0xcc, 0xcc, 0xcc, 0x0a, 0xd8, 0xd8, 0xd8, 0x0d, 0xd2, 0xd2, 0xd2, 0x11, 0xce, 0xce, 0xce, 0x15, 0xce, 0xce, 0xce, 0x1a, 0xce, 0xce, 0xce, 0x1f, 0xcd, 0xcd, 0xcd, 0x24, 0xc8, 0xc8, 0xc8, 0x2a, 0xc9, 0xc9, 0xc9, 0x2f, 0xc9, 0xc9, 0xc9, 0x34, 0xc9, 0xc9, 0xc9, 0x39, 0xc9, 0xc9, 0xc9, 0x3d, 0xc8, 0xc8, 0xc8, 0x41, 0xcb, 0xcb, 0xcb, 0x44, 0xee, 0xee, 0xee, 0x87, 0xfa, 0xfa, 0xfa, 0xc8, 0xf9, 0xf9, 0xf9, 0xc9, 0xf9, 0xf9, 0xf9, 0xc9, 0xfa, 0xfa, 0xfa, 0xc9, 0xfa, 0xfa, 0xfa, 0xc9, 0xf9, 0xf9, 0xf9, 0xc9, 0xf9, 0xf9, 0xf9, 0xc9, 0xfa, 0xfa, 0xfa, 0xc8, 0xee, 0xee, 0xee, 0x87, 0xcb, 0xcb, 0xcb, 0x44, 0xc8, 0xc8, 0xc8, 0x41, 0xc9, 0xc9, 0xc9, 0x3d, 0xc9, 0xc9, 0xc9, 0x39, 0xc9, 0xc9, 0xc9, 0x34, 0xc9, 0xc9, 0xc9, 0x2f, 0xc8, 0xc8, 0xc8, 0x2a, 0xcd, 0xcd, 0xcd, 0x24, 0xce, 0xce, 0xce, 0x1f, 0xce, 0xce, 0xce, 0x1a, 0xce, 0xce, 0xce, 0x15, 0xd2, 0xd2, 0xd2, 0x11, 0xd8, 0xd8, 0xd8, 0x0d, 0xcc, 0xcc, 0xcc, 0x0a, 0xdb, 0xdb, 0xdb, 0x07, 0xcc, 0xcc, 0xcc, 0x05, 0xbf, 0xbf, 0xbf, 0x04, 0xff, 0xff, 0xff, 0x02, 0xff, 0xff, 0xff, 0x01]);

var LASER_LENGTH = 1.0;
var LASER_DIAMETER = 0.01;
var LASER_FADE_END = 0.535;
var LASER_FADE_POINT = 0.5335;
var LASER_DEFAULT_COLOR = [1.0, 1.0, 1.0, 0.25];

var CURSOR_RADIUS = 0.004;
var CURSOR_SHADOW_RADIUS = 0.007;
var CURSOR_SHADOW_INNER_LUMINANCE = 0.5;
var CURSOR_SHADOW_OUTER_LUMINANCE = 0.0;
var CURSOR_SHADOW_INNER_OPACITY = 0.75;
var CURSOR_SHADOW_OUTER_OPACITY = 0.0;
var CURSOR_OPACITY = 0.9;
var CURSOR_SEGMENTS = 16;
var CURSOR_DEFAULT_COLOR = [1.0, 1.0, 1.0, 1.0];
var CURSOR_DEFAULT_HIDDEN_COLOR = [0.5, 0.5, 0.5, 0.25];

var DEFAULT_RESET_OPTIONS = {
  controllers: true,
  lasers: true,
  cursors: true
};

var LaserMaterial = function (_Material) {
  _inherits(LaserMaterial, _Material);

  function LaserMaterial() {
    _classCallCheck(this, LaserMaterial);

    var _this = _possibleConstructorReturn(this, (LaserMaterial.__proto__ || Object.getPrototypeOf(LaserMaterial)).call(this));

    _this.renderOrder = _material.RENDER_ORDER.ADDITIVE;
    _this.state.cullFace = false;
    _this.state.blend = true;
    _this.state.blendFuncSrc = GL.ONE;
    _this.state.blendFuncDst = GL.ONE;
    _this.state.depthMask = false;

    _this.laser = _this.defineSampler('diffuse');
    _this.laser.texture = new _texture.DataTexture(LASER_TEXTURE_DATA, 48, 1);
    _this.laserColor = _this.defineUniform('laserColor', LASER_DEFAULT_COLOR);
    return _this;
  }

  _createClass(LaserMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'INPUT_LASER';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec3 POSITION;\n    attribute vec2 TEXCOORD_0;\n\n    varying vec2 vTexCoord;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      vTexCoord = TEXCOORD_0;\n      return proj * view * model * vec4(POSITION, 1.0);\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    precision mediump float;\n\n    uniform vec4 laserColor;\n    uniform sampler2D diffuse;\n    varying vec2 vTexCoord;\n\n    const float fadePoint = ' + LASER_FADE_POINT + ';\n    const float fadeEnd = ' + LASER_FADE_END + ';\n\n    vec4 fragment_main() {\n      vec2 uv = vTexCoord;\n      float front_fade_factor = 1.0 - clamp(1.0 - (uv.y - fadePoint) / (1.0 - fadePoint), 0.0, 1.0);\n      float back_fade_factor = clamp((uv.y - fadePoint) / (fadeEnd - fadePoint), 0.0, 1.0);\n      vec4 color = laserColor * texture2D(diffuse, vTexCoord);\n      float opacity = color.a * front_fade_factor * back_fade_factor;\n      return vec4(color.rgb * opacity, opacity);\n    }';
    }
  }]);

  return LaserMaterial;
}(_material.Material);

var CURSOR_VERTEX_SHADER = '\nattribute vec4 POSITION;\n\nvarying float vLuminance;\nvarying float vOpacity;\n\nvec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n  vLuminance = POSITION.z;\n  vOpacity = POSITION.w;\n\n  // Billboarded, constant size vertex transform.\n  vec4 screenPos = proj * view * model * vec4(0.0, 0.0, 0.0, 1.0);\n  screenPos /= screenPos.w;\n  screenPos.xy += POSITION.xy;\n  return screenPos;\n}';

var CURSOR_FRAGMENT_SHADER = '\nprecision mediump float;\n\nuniform vec4 cursorColor;\nvarying float vLuminance;\nvarying float vOpacity;\n\nvec4 fragment_main() {\n  vec3 color = cursorColor.rgb * vLuminance;\n  float opacity = cursorColor.a * vOpacity;\n  return vec4(color * opacity, opacity);\n}';

// Cursors are drawn as billboards that always face the camera and are rendered
// as a fixed size no matter how far away they are.

var CursorMaterial = function (_Material2) {
  _inherits(CursorMaterial, _Material2);

  function CursorMaterial() {
    _classCallCheck(this, CursorMaterial);

    var _this2 = _possibleConstructorReturn(this, (CursorMaterial.__proto__ || Object.getPrototypeOf(CursorMaterial)).call(this));

    _this2.renderOrder = _material.RENDER_ORDER.ADDITIVE;
    _this2.state.cullFace = false;
    _this2.state.blend = true;
    _this2.state.blendFuncSrc = GL.ONE;
    _this2.state.depthMask = false;

    _this2.cursorColor = _this2.defineUniform('cursorColor', CURSOR_DEFAULT_COLOR);
    return _this2;
  }

  _createClass(CursorMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'INPUT_CURSOR';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return CURSOR_VERTEX_SHADER;
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return CURSOR_FRAGMENT_SHADER;
    }
  }]);

  return CursorMaterial;
}(_material.Material);

var CursorHiddenMaterial = function (_Material3) {
  _inherits(CursorHiddenMaterial, _Material3);

  function CursorHiddenMaterial() {
    _classCallCheck(this, CursorHiddenMaterial);

    var _this3 = _possibleConstructorReturn(this, (CursorHiddenMaterial.__proto__ || Object.getPrototypeOf(CursorHiddenMaterial)).call(this));

    _this3.renderOrder = _material.RENDER_ORDER.ADDITIVE;
    _this3.state.cullFace = false;
    _this3.state.blend = true;
    _this3.state.blendFuncSrc = GL.ONE;
    _this3.state.depthFunc = GL.GEQUAL;
    _this3.state.depthMask = false;

    _this3.cursorColor = _this3.defineUniform('cursorColor', CURSOR_DEFAULT_HIDDEN_COLOR);
    return _this3;
  }

  // TODO: Rename to "program_name"


  _createClass(CursorHiddenMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'INPUT_CURSOR_2';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return CURSOR_VERTEX_SHADER;
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return CURSOR_FRAGMENT_SHADER;
    }
  }]);

  return CursorHiddenMaterial;
}(_material.Material);

var InputRenderer = exports.InputRenderer = function (_Node) {
  _inherits(InputRenderer, _Node);

  function InputRenderer() {
    _classCallCheck(this, InputRenderer);

    var _this4 = _possibleConstructorReturn(this, (InputRenderer.__proto__ || Object.getPrototypeOf(InputRenderer)).call(this));

    _this4._maxInputElements = 32;

    _this4._controllers = [];
    _this4._controllerNode = null;
    _this4._controllerNodeHandedness = null;
    _this4._lasers = null;
    _this4._cursors = null;

    _this4._activeControllers = 0;
    _this4._activeLasers = 0;
    _this4._activeCursors = 0;
    return _this4;
  }

  _createClass(InputRenderer, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      this._controllers = [];
      this._controllerNode = null;
      this._controllerNodeHandedness = null;
      this._lasers = null;
      this._cursors = null;

      this._activeControllers = 0;
      this._activeLasers = 0;
      this._activeCursors = 0;
    }
  }, {
    key: 'setControllerMesh',
    value: function setControllerMesh(controllerNode) {
      var handedness = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : 'right';

      this._controllerNode = controllerNode;
      this._controllerNode.visible = false;
      // FIXME: Temporary fix to initialize for cloning.
      this.addNode(this._controllerNode);
      this._controllerNodeHandedness = handedness;
    }
  }, {
    key: 'addController',
    value: function addController(gripMatrix) {
      if (!this._controllerNode) {
        return;
      }

      var controller = null;
      if (this._activeControllers < this._controllers.length) {
        controller = this._controllers[this._activeControllers];
      } else {
        controller = this._controllerNode.clone();
        this.addNode(controller);
        this._controllers.push(controller);
      }
      this._activeControllers = (this._activeControllers + 1) % this._maxInputElements;

      controller.matrix = gripMatrix;
      controller.visible = true;
    }
  }, {
    key: 'addLaserPointer',
    value: function addLaserPointer(pointerMatrix) {
      // Create the laser pointer mesh if needed.
      if (!this._lasers && this._renderer) {
        this._lasers = [this._createLaserMesh()];
        this.addNode(this._lasers[0]);
      }

      var laser = null;
      if (this._activeLasers < this._lasers.length) {
        laser = this._lasers[this._activeLasers];
      } else {
        laser = this._lasers[0].clone();
        this.addNode(laser);
        this._lasers.push(laser);
      }
      this._activeLasers = (this._activeLasers + 1) % this._maxInputElements;

      laser.matrix = pointerMatrix;
      laser.visible = true;
    }
  }, {
    key: 'addCursor',
    value: function addCursor(cursorPos) {
      // Create the cursor mesh if needed.
      if (!this._cursors && this._renderer) {
        this._cursors = [this._createCursorMesh()];
        this.addNode(this._cursors[0]);
      }

      var cursor = null;
      if (this._activeCursors < this._cursors.length) {
        cursor = this._cursors[this._activeCursors];
      } else {
        cursor = this._cursors[0].clone();
        this.addNode(cursor);
        this._cursors.push(cursor);
      }
      this._activeCursors = (this._activeCursors + 1) % this._maxInputElements;

      cursor.translation = cursorPos;
      cursor.visible = true;
    }
  }, {
    key: 'reset',
    value: function reset(options) {
      if (!options) {
        options = DEFAULT_RESET_OPTIONS;
      }
      if (this._controllers && options.controllers) {
        var _iteratorNormalCompletion = true;
        var _didIteratorError = false;
        var _iteratorError = undefined;

        try {
          for (var _iterator = this._controllers[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
            var controller = _step.value;

            controller.visible = false;
          }
        } catch (err) {
          _didIteratorError = true;
          _iteratorError = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion && _iterator.return) {
              _iterator.return();
            }
          } finally {
            if (_didIteratorError) {
              throw _iteratorError;
            }
          }
        }

        this._activeControllers = 0;
      }
      if (this._lasers && options.lasers) {
        var _iteratorNormalCompletion2 = true;
        var _didIteratorError2 = false;
        var _iteratorError2 = undefined;

        try {
          for (var _iterator2 = this._lasers[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
            var laser = _step2.value;

            laser.visible = false;
          }
        } catch (err) {
          _didIteratorError2 = true;
          _iteratorError2 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion2 && _iterator2.return) {
              _iterator2.return();
            }
          } finally {
            if (_didIteratorError2) {
              throw _iteratorError2;
            }
          }
        }

        this._activeLasers = 0;
      }
      if (this._cursors && options.cursors) {
        var _iteratorNormalCompletion3 = true;
        var _didIteratorError3 = false;
        var _iteratorError3 = undefined;

        try {
          for (var _iterator3 = this._cursors[Symbol.iterator](), _step3; !(_iteratorNormalCompletion3 = (_step3 = _iterator3.next()).done); _iteratorNormalCompletion3 = true) {
            var cursor = _step3.value;

            cursor.visible = false;
          }
        } catch (err) {
          _didIteratorError3 = true;
          _iteratorError3 = err;
        } finally {
          try {
            if (!_iteratorNormalCompletion3 && _iterator3.return) {
              _iterator3.return();
            }
          } finally {
            if (_didIteratorError3) {
              throw _iteratorError3;
            }
          }
        }

        this._activeCursors = 0;
      }
    }
  }, {
    key: '_createLaserMesh',
    value: function _createLaserMesh() {
      var gl = this._renderer._gl;

      var lr = LASER_DIAMETER * 0.5;
      var ll = LASER_LENGTH;

      // Laser is rendered as cross-shaped beam
      var laserVerts = [
      // X    Y   Z    U    V
      0.0, lr, 0.0, 0.0, 1.0, 0.0, lr, -ll, 0.0, 0.0, 0.0, -lr, 0.0, 1.0, 1.0, 0.0, -lr, -ll, 1.0, 0.0, lr, 0.0, 0.0, 0.0, 1.0, lr, 0.0, -ll, 0.0, 0.0, -lr, 0.0, 0.0, 1.0, 1.0, -lr, 0.0, -ll, 1.0, 0.0, 0.0, -lr, 0.0, 0.0, 1.0, 0.0, -lr, -ll, 0.0, 0.0, 0.0, lr, 0.0, 1.0, 1.0, 0.0, lr, -ll, 1.0, 0.0, -lr, 0.0, 0.0, 0.0, 1.0, -lr, 0.0, -ll, 0.0, 0.0, lr, 0.0, 0.0, 1.0, 1.0, lr, 0.0, -ll, 1.0, 0.0];
      var laserIndices = [0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6, 8, 9, 10, 9, 11, 10, 12, 13, 14, 13, 15, 14];

      var laserVertexBuffer = this._renderer.createRenderBuffer(gl.ARRAY_BUFFER, new Float32Array(laserVerts));
      var laserIndexBuffer = this._renderer.createRenderBuffer(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(laserIndices));

      var laserIndexCount = laserIndices.length;

      var laserAttribs = [new _primitive.PrimitiveAttribute('POSITION', laserVertexBuffer, 3, gl.FLOAT, 20, 0), new _primitive.PrimitiveAttribute('TEXCOORD_0', laserVertexBuffer, 2, gl.FLOAT, 20, 12)];

      var laserPrimitive = new _primitive.Primitive(laserAttribs, laserIndexCount);
      laserPrimitive.setIndexBuffer(laserIndexBuffer);

      var laserMaterial = new LaserMaterial();

      var laserRenderPrimitive = this._renderer.createRenderPrimitive(laserPrimitive, laserMaterial);
      var meshNode = new _node.Node();
      meshNode.addRenderPrimitive(laserRenderPrimitive);
      return meshNode;
    }
  }, {
    key: '_createCursorMesh',
    value: function _createCursorMesh() {
      var gl = this._renderer._gl;

      // Cursor is a circular white dot with a dark "shadow" skirt around the edge
      // that fades from black to transparent as it moves out from the center.
      // Cursor verts are packed as [X, Y, Luminance, Opacity]
      var cursorVerts = [];
      var cursorIndices = [];

      var segRad = 2.0 * Math.PI / CURSOR_SEGMENTS;

      // Cursor center
      for (var i = 0; i < CURSOR_SEGMENTS; ++i) {
        var rad = i * segRad;
        var x = Math.cos(rad);
        var y = Math.sin(rad);
        cursorVerts.push(x * CURSOR_RADIUS, y * CURSOR_RADIUS, 1.0, CURSOR_OPACITY);

        if (i > 1) {
          cursorIndices.push(0, i - 1, i);
        }
      }

      var indexOffset = CURSOR_SEGMENTS;

      // Cursor Skirt
      for (var _i = 0; _i < CURSOR_SEGMENTS; ++_i) {
        var _rad = _i * segRad;
        var _x2 = Math.cos(_rad);
        var _y = Math.sin(_rad);
        cursorVerts.push(_x2 * CURSOR_RADIUS, _y * CURSOR_RADIUS, CURSOR_SHADOW_INNER_LUMINANCE, CURSOR_SHADOW_INNER_OPACITY);
        cursorVerts.push(_x2 * CURSOR_SHADOW_RADIUS, _y * CURSOR_SHADOW_RADIUS, CURSOR_SHADOW_OUTER_LUMINANCE, CURSOR_SHADOW_OUTER_OPACITY);

        if (_i > 0) {
          var _idx = indexOffset + _i * 2;
          cursorIndices.push(_idx - 2, _idx - 1, _idx);
          cursorIndices.push(_idx - 1, _idx + 1, _idx);
        }
      }

      var idx = indexOffset + CURSOR_SEGMENTS * 2;
      cursorIndices.push(idx - 2, idx - 1, indexOffset);
      cursorIndices.push(idx - 1, indexOffset + 1, indexOffset);

      var cursorVertexBuffer = this._renderer.createRenderBuffer(gl.ARRAY_BUFFER, new Float32Array(cursorVerts));
      var cursorIndexBuffer = this._renderer.createRenderBuffer(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(cursorIndices));

      var cursorIndexCount = cursorIndices.length;

      var cursorAttribs = [new _primitive.PrimitiveAttribute('POSITION', cursorVertexBuffer, 4, gl.FLOAT, 16, 0)];

      var cursorPrimitive = new _primitive.Primitive(cursorAttribs, cursorIndexCount);
      cursorPrimitive.setIndexBuffer(cursorIndexBuffer);

      var cursorMaterial = new CursorMaterial();
      var cursorHiddenMaterial = new CursorHiddenMaterial();

      // Cursor renders two parts: The bright opaque cursor for areas where it's
      // not obscured and a more transparent, darker version for areas where it's
      // behind another object.
      var cursorRenderPrimitive = this._renderer.createRenderPrimitive(cursorPrimitive, cursorMaterial);
      var cursorHiddenRenderPrimitive = this._renderer.createRenderPrimitive(cursorPrimitive, cursorHiddenMaterial);
      var meshNode = new _node.Node();
      meshNode.addRenderPrimitive(cursorRenderPrimitive);
      meshNode.addRenderPrimitive(cursorHiddenRenderPrimitive);
      return meshNode;
    }
  }]);

  return InputRenderer;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/seven-segment-text.js":
/*!*****************************************!*\
  !*** ./src/nodes/seven-segment-text.js ***!
  \*****************************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.SevenSegmentText = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

function _toConsumableArray(arr) { if (Array.isArray(arr)) { for (var i = 0, arr2 = Array(arr.length); i < arr.length; i++) { arr2[i] = arr[i]; } return arr2; } else { return Array.from(arr); } }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
Renders simple text using a seven-segment LED style pattern. Only really good
for numbers and a limited number of other characters.
*/

var TEXT_KERNING = 2.0;

var SevenSegmentMaterial = function (_Material) {
  _inherits(SevenSegmentMaterial, _Material);

  function SevenSegmentMaterial() {
    _classCallCheck(this, SevenSegmentMaterial);

    return _possibleConstructorReturn(this, (SevenSegmentMaterial.__proto__ || Object.getPrototypeOf(SevenSegmentMaterial)).apply(this, arguments));
  }

  _createClass(SevenSegmentMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'SEVEN_SEGMENT_TEXT';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec2 POSITION;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      return proj * view * model * vec4(POSITION, 0.0, 1.0);\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    precision mediump float;\n    const vec4 color = vec4(0.0, 1.0, 0.0, 1.0);\n\n    vec4 fragment_main() {\n      return color;\n    }';
    }
  }]);

  return SevenSegmentMaterial;
}(_material.Material);

var SevenSegmentText = exports.SevenSegmentText = function (_Node) {
  _inherits(SevenSegmentText, _Node);

  function SevenSegmentText() {
    _classCallCheck(this, SevenSegmentText);

    var _this2 = _possibleConstructorReturn(this, (SevenSegmentText.__proto__ || Object.getPrototypeOf(SevenSegmentText)).call(this));

    _this2._text = '';
    _this2._charNodes = [];
    return _this2;
  }

  _createClass(SevenSegmentText, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      this.clearNodes();
      this._charNodes = [];

      var vertices = [];
      var segmentIndices = {};
      var indices = [];

      var width = 0.5;
      var thickness = 0.25;

      function defineSegment(id, left, top, right, bottom) {
        var idx = vertices.length / 2;
        vertices.push(left, top, right, top, right, bottom, left, bottom);

        segmentIndices[id] = [idx, idx + 2, idx + 1, idx, idx + 3, idx + 2];
      }

      var characters = {};
      function defineCharacter(c, segments) {
        var character = {
          character: c,
          offset: indices.length * 2,
          count: 0
        };

        for (var i = 0; i < segments.length; ++i) {
          var idx = segments[i];
          var segment = segmentIndices[idx];
          character.count += segment.length;
          indices.push.apply(indices, _toConsumableArray(segment));
        }

        characters[c] = character;
      }

      /* Segment layout is as follows:
       |-0-|
      3   4
      |-1-|
      5   6
      |-2-|
       */

      defineSegment(0, -1, 1, width, 1 - thickness);
      defineSegment(1, -1, thickness * 0.5, width, -thickness * 0.5);
      defineSegment(2, -1, -1 + thickness, width, -1);
      defineSegment(3, -1, 1, -1 + thickness, -thickness * 0.5);
      defineSegment(4, width - thickness, 1, width, -thickness * 0.5);
      defineSegment(5, -1, thickness * 0.5, -1 + thickness, -1);
      defineSegment(6, width - thickness, thickness * 0.5, width, -1);

      defineCharacter('0', [0, 2, 3, 4, 5, 6]);
      defineCharacter('1', [4, 6]);
      defineCharacter('2', [0, 1, 2, 4, 5]);
      defineCharacter('3', [0, 1, 2, 4, 6]);
      defineCharacter('4', [1, 3, 4, 6]);
      defineCharacter('5', [0, 1, 2, 3, 6]);
      defineCharacter('6', [0, 1, 2, 3, 5, 6]);
      defineCharacter('7', [0, 4, 6]);
      defineCharacter('8', [0, 1, 2, 3, 4, 5, 6]);
      defineCharacter('9', [0, 1, 2, 3, 4, 6]);
      defineCharacter('A', [0, 1, 3, 4, 5, 6]);
      defineCharacter('B', [1, 2, 3, 5, 6]);
      defineCharacter('C', [0, 2, 3, 5]);
      defineCharacter('D', [1, 2, 4, 5, 6]);
      defineCharacter('E', [0, 1, 2, 4, 6]);
      defineCharacter('F', [0, 1, 3, 5]);
      defineCharacter('P', [0, 1, 3, 4, 5]);
      defineCharacter('-', [1]);
      defineCharacter(' ', []);
      defineCharacter('_', [2]); // Used for undefined characters

      var gl = renderer.gl;
      var vertexBuffer = renderer.createRenderBuffer(gl.ARRAY_BUFFER, new Float32Array(vertices));
      var indexBuffer = renderer.createRenderBuffer(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices));

      var vertexAttribs = [new _primitive.PrimitiveAttribute('POSITION', vertexBuffer, 2, gl.FLOAT, 8, 0)];

      var primitive = new _primitive.Primitive(vertexAttribs, indices.length);
      primitive.setIndexBuffer(indexBuffer);

      var material = new SevenSegmentMaterial();

      this._charPrimitives = {};
      for (var char in characters) {
        var charDef = characters[char];
        primitive.elementCount = charDef.count;
        primitive.indexByteOffset = charDef.offset;
        this._charPrimitives[char] = renderer.createRenderPrimitive(primitive, material);
      }

      this.text = this._text;
    }
  }, {
    key: 'text',
    get: function get() {
      return this._text;
    },
    set: function set(value) {
      this._text = value;

      var i = 0;
      var charPrimitive = null;
      for (; i < value.length; ++i) {
        if (value[i] in this._charPrimitives) {
          charPrimitive = this._charPrimitives[value[i]];
        } else {
          charPrimitive = this._charPrimitives['_'];
        }

        if (this._charNodes.length <= i) {
          var node = new _node.Node();
          node.addRenderPrimitive(charPrimitive);
          var offset = i * TEXT_KERNING;
          node.translation = [offset, 0, 0];
          this._charNodes.push(node);
          this.addNode(node);
        } else {
          // This is sort of an abuse of how these things are expected to work,
          // but it's the cheapest thing I could think of that didn't break the
          // world.
          this._charNodes[i].clearRenderPrimitives();
          this._charNodes[i].addRenderPrimitive(charPrimitive);
          this._charNodes[i].visible = true;
        }
      }

      // If there's any nodes left over make them invisible
      for (; i < this._charNodes.length; ++i) {
        this._charNodes[i].visible = false;
      }
    }
  }]);

  return SevenSegmentText;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/skybox.js":
/*!*****************************!*\
  !*** ./src/nodes/skybox.js ***!
  \*****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.SkyboxNode = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _texture = __webpack_require__(/*! ../core/texture.js */ "./src/core/texture.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
Node for displaying 360 equirect images as a skybox.
*/

var GL = WebGLRenderingContext; // For enums

var SkyboxMaterial = function (_Material) {
  _inherits(SkyboxMaterial, _Material);

  function SkyboxMaterial() {
    _classCallCheck(this, SkyboxMaterial);

    var _this = _possibleConstructorReturn(this, (SkyboxMaterial.__proto__ || Object.getPrototypeOf(SkyboxMaterial)).call(this));

    _this.renderOrder = _material.RENDER_ORDER.SKY;
    _this.state.depthFunc = GL.LEQUAL;
    _this.state.depthMask = false;

    _this.image = _this.defineSampler('diffuse');

    _this.texCoordScaleOffset = _this.defineUniform('texCoordScaleOffset', [1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0], 4);
    return _this;
  }

  _createClass(SkyboxMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'SKYBOX';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    uniform int EYE_INDEX;\n    uniform vec4 texCoordScaleOffset[2];\n    attribute vec3 POSITION;\n    attribute vec2 TEXCOORD_0;\n    varying vec2 vTexCoord;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      vec4 scaleOffset = texCoordScaleOffset[EYE_INDEX];\n      vTexCoord = (TEXCOORD_0 * scaleOffset.xy) + scaleOffset.zw;\n      // Drop the translation portion of the view matrix\n      view[3].xyz = vec3(0.0, 0.0, 0.0);\n      vec4 out_vec = proj * view * model * vec4(POSITION, 1.0);\n\n      // Returning the W component for both Z and W forces the geometry depth to\n      // the far plane. When combined with a depth func of LEQUAL this makes the\n      // sky write to any depth fragment that has not been written to yet.\n      return out_vec.xyww;\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    uniform sampler2D diffuse;\n    varying vec2 vTexCoord;\n\n    vec4 fragment_main() {\n      return texture2D(diffuse, vTexCoord);\n    }';
    }
  }]);

  return SkyboxMaterial;
}(_material.Material);

var SkyboxNode = exports.SkyboxNode = function (_Node) {
  _inherits(SkyboxNode, _Node);

  function SkyboxNode(options) {
    _classCallCheck(this, SkyboxNode);

    var _this2 = _possibleConstructorReturn(this, (SkyboxNode.__proto__ || Object.getPrototypeOf(SkyboxNode)).call(this));

    _this2._url = options.url;
    _this2._displayMode = options.displayMode || 'mono';
    _this2._rotationY = options.rotationY || 0;
    return _this2;
  }

  _createClass(SkyboxNode, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      var vertices = [];
      var indices = [];

      var latSegments = 40;
      var lonSegments = 40;

      // Create the vertices/indices
      for (var i = 0; i <= latSegments; ++i) {
        var theta = i * Math.PI / latSegments;
        var sinTheta = Math.sin(theta);
        var cosTheta = Math.cos(theta);

        var idxOffsetA = i * (lonSegments + 1);
        var idxOffsetB = (i + 1) * (lonSegments + 1);

        for (var j = 0; j <= lonSegments; ++j) {
          var phi = j * 2 * Math.PI / lonSegments + this._rotationY;
          var x = Math.sin(phi) * sinTheta;
          var y = cosTheta;
          var z = -Math.cos(phi) * sinTheta;
          var u = j / lonSegments;
          var v = i / latSegments;

          // Vertex shader will force the geometry to the far plane, so the
          // radius of the sphere is immaterial.
          vertices.push(x, y, z, u, v);

          if (i < latSegments && j < lonSegments) {
            var idxA = idxOffsetA + j;
            var idxB = idxOffsetB + j;

            indices.push(idxA, idxB, idxA + 1, idxB, idxB + 1, idxA + 1);
          }
        }
      }

      var vertexBuffer = renderer.createRenderBuffer(GL.ARRAY_BUFFER, new Float32Array(vertices));
      var indexBuffer = renderer.createRenderBuffer(GL.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices));

      var attribs = [new _primitive.PrimitiveAttribute('POSITION', vertexBuffer, 3, GL.FLOAT, 20, 0), new _primitive.PrimitiveAttribute('TEXCOORD_0', vertexBuffer, 2, GL.FLOAT, 20, 12)];

      var primitive = new _primitive.Primitive(attribs, indices.length);
      primitive.setIndexBuffer(indexBuffer);

      var material = new SkyboxMaterial();
      material.image.texture = new _texture.UrlTexture(this._url);

      switch (this._displayMode) {
        case 'mono':
          material.texCoordScaleOffset.value = [1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0];
          break;
        case 'stereoTopBottom':
          material.texCoordScaleOffset.value = [1.0, 0.5, 0.0, 0.0, 1.0, 0.5, 0.0, 0.5];
          break;
        case 'stereoLeftRight':
          material.texCoordScaleOffset.value = [0.5, 1.0, 0.0, 0.0, 0.5, 1.0, 0.5, 0.0];
          break;
      }

      var renderPrimitive = renderer.createRenderPrimitive(primitive, material);
      this.addRenderPrimitive(renderPrimitive);
    }
  }]);

  return SkyboxNode;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/stats-viewer.js":
/*!***********************************!*\
  !*** ./src/nodes/stats-viewer.js ***!
  \***********************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.StatsViewer = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

var _sevenSegmentText = __webpack_require__(/*! ./seven-segment-text.js */ "./src/nodes/seven-segment-text.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
Heavily inspired by Mr. Doobs stats.js, this FPS counter is rendered completely
with WebGL, allowing it to be shown in cases where overlaid HTML elements aren't
usable (like WebXR), or if you want the FPS counter to be rendered as part of
your scene.
*/

var SEGMENTS = 30;
var MAX_FPS = 90;

var StatsMaterial = function (_Material) {
  _inherits(StatsMaterial, _Material);

  function StatsMaterial() {
    _classCallCheck(this, StatsMaterial);

    return _possibleConstructorReturn(this, (StatsMaterial.__proto__ || Object.getPrototypeOf(StatsMaterial)).apply(this, arguments));
  }

  _createClass(StatsMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'STATS_VIEWER';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    attribute vec3 POSITION;\n    attribute vec3 COLOR_0;\n    varying vec4 vColor;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      vColor = vec4(COLOR_0, 1.0);\n      return proj * view * model * vec4(POSITION, 1.0);\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    precision mediump float;\n    varying vec4 vColor;\n\n    vec4 fragment_main() {\n      return vColor;\n    }';
    }
  }]);

  return StatsMaterial;
}(_material.Material);

function segmentToX(i) {
  return 0.9 / SEGMENTS * i - 0.45;
}

function fpsToY(value) {
  return Math.min(value, MAX_FPS) * (0.7 / MAX_FPS) - 0.45;
}

function fpsToRGB(value) {
  return {
    r: Math.max(0.0, Math.min(1.0, 1.0 - value / 60)),
    g: Math.max(0.0, Math.min(1.0, (value - 15) / (MAX_FPS - 15))),
    b: Math.max(0.0, Math.min(1.0, (value - 15) / (MAX_FPS - 15)))
  };
}

var now = window.performance && performance.now ? performance.now.bind(performance) : Date.now;

var StatsViewer = exports.StatsViewer = function (_Node) {
  _inherits(StatsViewer, _Node);

  function StatsViewer() {
    _classCallCheck(this, StatsViewer);

    var _this2 = _possibleConstructorReturn(this, (StatsViewer.__proto__ || Object.getPrototypeOf(StatsViewer)).call(this));

    _this2._performanceMonitoring = false;

    _this2._startTime = now();
    _this2._prevFrameTime = _this2._startTime;
    _this2._prevGraphUpdateTime = _this2._startTime;
    _this2._frames = 0;
    _this2._fpsAverage = 0;
    _this2._fpsMin = 0;
    _this2._fpsStep = _this2._performanceMonitoring ? 1000 : 250;
    _this2._lastSegment = 0;

    _this2._fpsVertexBuffer = null;
    _this2._fpsRenderPrimitive = null;
    _this2._fpsNode = null;

    _this2._sevenSegmentNode = new _sevenSegmentText.SevenSegmentText();
    // Hard coded because it doesn't change:
    // Scale by 0.075 in X and Y
    // Translate into upper left corner w/ z = 0.02
    _this2._sevenSegmentNode.matrix = new Float32Array([0.075, 0, 0, 0, 0, 0.075, 0, 0, 0, 0, 1, 0, -0.3625, 0.3625, 0.02, 1]);
    return _this2;
  }

  _createClass(StatsViewer, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      this.clearNodes();

      var gl = renderer.gl;

      var fpsVerts = [];
      var fpsIndices = [];

      // Graph geometry
      for (var i = 0; i < SEGMENTS; ++i) {
        // Bar top
        fpsVerts.push(segmentToX(i), fpsToY(0), 0.02, 0.0, 1.0, 1.0);
        fpsVerts.push(segmentToX(i + 1), fpsToY(0), 0.02, 0.0, 1.0, 1.0);

        // Bar bottom
        fpsVerts.push(segmentToX(i), fpsToY(0), 0.02, 0.0, 1.0, 1.0);
        fpsVerts.push(segmentToX(i + 1), fpsToY(0), 0.02, 0.0, 1.0, 1.0);

        var idx = i * 4;
        fpsIndices.push(idx, idx + 3, idx + 1, idx + 3, idx, idx + 2);
      }

      function addBGSquare(left, bottom, right, top, z, r, g, b) {
        var idx = fpsVerts.length / 6;

        fpsVerts.push(left, bottom, z, r, g, b);
        fpsVerts.push(right, top, z, r, g, b);
        fpsVerts.push(left, top, z, r, g, b);
        fpsVerts.push(right, bottom, z, r, g, b);

        fpsIndices.push(idx, idx + 1, idx + 2, idx, idx + 3, idx + 1);
      }

      // Panel Background
      addBGSquare(-0.5, -0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.125);

      // FPS Background
      addBGSquare(-0.45, -0.45, 0.45, 0.25, 0.01, 0.0, 0.0, 0.4);

      // 30 FPS line
      addBGSquare(-0.45, fpsToY(30), 0.45, fpsToY(32), 0.015, 0.5, 0.0, 0.5);

      // 60 FPS line
      addBGSquare(-0.45, fpsToY(60), 0.45, fpsToY(62), 0.015, 0.2, 0.0, 0.75);

      this._fpsVertexBuffer = renderer.createRenderBuffer(gl.ARRAY_BUFFER, new Float32Array(fpsVerts), gl.DYNAMIC_DRAW);
      var fpsIndexBuffer = renderer.createRenderBuffer(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(fpsIndices));

      var fpsAttribs = [new _primitive.PrimitiveAttribute('POSITION', this._fpsVertexBuffer, 3, gl.FLOAT, 24, 0), new _primitive.PrimitiveAttribute('COLOR_0', this._fpsVertexBuffer, 3, gl.FLOAT, 24, 12)];

      var fpsPrimitive = new _primitive.Primitive(fpsAttribs, fpsIndices.length);
      fpsPrimitive.setIndexBuffer(fpsIndexBuffer);
      fpsPrimitive.setBounds([-0.5, -0.5, 0.0], [0.5, 0.5, 0.015]);

      this._fpsRenderPrimitive = renderer.createRenderPrimitive(fpsPrimitive, new StatsMaterial());
      this._fpsNode = new _node.Node();
      this._fpsNode.addRenderPrimitive(this._fpsRenderPrimitive);

      this.addNode(this._fpsNode);
      this.addNode(this._sevenSegmentNode);
    }
  }, {
    key: 'begin',
    value: function begin() {
      this._startTime = now();
    }
  }, {
    key: 'end',
    value: function end() {
      var time = now();

      var frameFps = 1000 / (time - this._prevFrameTime);
      this._prevFrameTime = time;
      this._fpsMin = this._frames ? Math.min(this._fpsMin, frameFps) : frameFps;
      this._frames++;

      if (time > this._prevGraphUpdateTime + this._fpsStep) {
        var intervalTime = time - this._prevGraphUpdateTime;
        this._fpsAverage = Math.round(1000 / (intervalTime / this._frames));

        // Draw both average and minimum FPS for this period
        // so that dropped frames are more clearly visible.
        this._updateGraph(this._fpsMin, this._fpsAverage);
        if (this._performanceMonitoring) {
          console.log('Average FPS: ' + this._fpsAverage + ' Min FPS: ' + this._fpsMin);
        }

        this._prevGraphUpdateTime = time;
        this._frames = 0;
        this._fpsMin = 0;
      }
    }
  }, {
    key: '_updateGraph',
    value: function _updateGraph(valueLow, valueHigh) {
      var color = fpsToRGB(valueLow);
      // Draw a range from the low to high value. Artificially widen the
      // range a bit to ensure that near-equal values still remain
      // visible - the logic here should match that used by the
      // "60 FPS line" setup below. Hitting 60fps consistently will
      // keep the top half of the 60fps background line visible.
      var y0 = fpsToY(valueLow - 1);
      var y1 = fpsToY(valueHigh + 1);

      // Update the current segment with the new FPS value
      var updateVerts = [segmentToX(this._lastSegment), y1, 0.02, color.r, color.g, color.b, segmentToX(this._lastSegment + 1), y1, 0.02, color.r, color.g, color.b, segmentToX(this._lastSegment), y0, 0.02, color.r, color.g, color.b, segmentToX(this._lastSegment + 1), y0, 0.02, color.r, color.g, color.b];

      // Re-shape the next segment into the green "progress" line
      color.r = 0.2;
      color.g = 1.0;
      color.b = 0.2;

      if (this._lastSegment == SEGMENTS - 1) {
        // If we're updating the last segment we need to do two bufferSubDatas
        // to update the segment and turn the first segment into the progress line.
        this._renderer.updateRenderBuffer(this._fpsVertexBuffer, new Float32Array(updateVerts), this._lastSegment * 24 * 4);
        updateVerts = [segmentToX(0), fpsToY(MAX_FPS), 0.02, color.r, color.g, color.b, segmentToX(.25), fpsToY(MAX_FPS), 0.02, color.r, color.g, color.b, segmentToX(0), fpsToY(0), 0.02, color.r, color.g, color.b, segmentToX(.25), fpsToY(0), 0.02, color.r, color.g, color.b];
        this._renderer.updateRenderBuffer(this._fpsVertexBuffer, new Float32Array(updateVerts), 0);
      } else {
        updateVerts.push(segmentToX(this._lastSegment + 1), fpsToY(MAX_FPS), 0.02, color.r, color.g, color.b, segmentToX(this._lastSegment + 1.25), fpsToY(MAX_FPS), 0.02, color.r, color.g, color.b, segmentToX(this._lastSegment + 1), fpsToY(0), 0.02, color.r, color.g, color.b, segmentToX(this._lastSegment + 1.25), fpsToY(0), 0.02, color.r, color.g, color.b);
        this._renderer.updateRenderBuffer(this._fpsVertexBuffer, new Float32Array(updateVerts), this._lastSegment * 24 * 4);
      }

      this._lastSegment = (this._lastSegment + 1) % SEGMENTS;

      this._sevenSegmentNode.text = this._fpsAverage + ' FP5';
    }
  }, {
    key: 'performanceMonitoring',
    get: function get() {
      return this._performanceMonitoring;
    },
    set: function set(value) {
      this._performanceMonitoring = value;
      this._fpsStep = value ? 1000 : 250;
    }
  }]);

  return StatsViewer;
}(_node.Node);

/***/ }),

/***/ "./src/nodes/video.js":
/*!****************************!*\
  !*** ./src/nodes/video.js ***!
  \****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.VideoNode = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _material = __webpack_require__(/*! ../core/material.js */ "./src/core/material.js");

var _primitive = __webpack_require__(/*! ../core/primitive.js */ "./src/core/primitive.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _texture = __webpack_require__(/*! ../core/texture.js */ "./src/core/texture.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
Node for displaying 2D or stereo videos on a quad.
*/

var GL = WebGLRenderingContext; // For enums

var VideoMaterial = function (_Material) {
  _inherits(VideoMaterial, _Material);

  function VideoMaterial() {
    _classCallCheck(this, VideoMaterial);

    var _this = _possibleConstructorReturn(this, (VideoMaterial.__proto__ || Object.getPrototypeOf(VideoMaterial)).call(this));

    _this.image = _this.defineSampler('diffuse');

    _this.texCoordScaleOffset = _this.defineUniform('texCoordScaleOffset', [1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0], 4);
    return _this;
  }

  _createClass(VideoMaterial, [{
    key: 'materialName',
    get: function get() {
      return 'VIDEO_PLAYER';
    }
  }, {
    key: 'vertexSource',
    get: function get() {
      return '\n    uniform int EYE_INDEX;\n    uniform vec4 texCoordScaleOffset[2];\n    attribute vec3 POSITION;\n    attribute vec2 TEXCOORD_0;\n    varying vec2 vTexCoord;\n\n    vec4 vertex_main(mat4 proj, mat4 view, mat4 model) {\n      vec4 scaleOffset = texCoordScaleOffset[EYE_INDEX];\n      vTexCoord = (TEXCOORD_0 * scaleOffset.xy) + scaleOffset.zw;\n      vec4 out_vec = proj * view * model * vec4(POSITION, 1.0);\n      return out_vec;\n    }';
    }
  }, {
    key: 'fragmentSource',
    get: function get() {
      return '\n    uniform sampler2D diffuse;\n    varying vec2 vTexCoord;\n\n    vec4 fragment_main() {\n      return texture2D(diffuse, vTexCoord);\n    }';
    }
  }]);

  return VideoMaterial;
}(_material.Material);

var VideoNode = exports.VideoNode = function (_Node) {
  _inherits(VideoNode, _Node);

  function VideoNode(options) {
    _classCallCheck(this, VideoNode);

    var _this2 = _possibleConstructorReturn(this, (VideoNode.__proto__ || Object.getPrototypeOf(VideoNode)).call(this));

    _this2._video = options.video;
    _this2._displayMode = options.displayMode || 'mono';

    _this2._video_texture = new _texture.VideoTexture(_this2._video);
    return _this2;
  }

  _createClass(VideoNode, [{
    key: 'onRendererChanged',
    value: function onRendererChanged(renderer) {
      var vertices = [-1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, -1.0, -1.0, 0.0, 0.0, 1.0];
      var indices = [0, 2, 1, 0, 3, 2];

      var vertexBuffer = renderer.createRenderBuffer(GL.ARRAY_BUFFER, new Float32Array(vertices));
      var indexBuffer = renderer.createRenderBuffer(GL.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices));

      var attribs = [new _primitive.PrimitiveAttribute('POSITION', vertexBuffer, 3, GL.FLOAT, 20, 0), new _primitive.PrimitiveAttribute('TEXCOORD_0', vertexBuffer, 2, GL.FLOAT, 20, 12)];

      var primitive = new _primitive.Primitive(attribs, indices.length);
      primitive.setIndexBuffer(indexBuffer);
      primitive.setBounds([-1.0, -1.0, 0.0], [1.0, 1.0, 0.015]);

      var material = new VideoMaterial();
      material.image.texture = this._video_texture;

      switch (this._displayMode) {
        case 'mono':
          material.texCoordScaleOffset.value = [1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0];
          break;
        case 'stereoTopBottom':
          material.texCoordScaleOffset.value = [1.0, 0.5, 0.0, 0.0, 1.0, 0.5, 0.0, 0.5];
          break;
        case 'stereoLeftRight':
          material.texCoordScaleOffset.value = [0.5, 1.0, 0.0, 0.0, 0.5, 1.0, 0.5, 0.0];
          break;
      }

      var renderPrimitive = renderer.createRenderPrimitive(primitive, material);
      this.addRenderPrimitive(renderPrimitive);
    }
  }, {
    key: 'aspectRatio',
    get: function get() {
      var width = this._video.videoWidth;
      var height = this._video.videoHeight;

      switch (this._displayMode) {
        case 'stereoTopBottom':
          height *= 0.5;break;
        case 'stereoLeftRight':
          width *= 0.5;break;
      }

      if (!height || !width) {
        return 1;
      }

      return width / height;
    }
  }]);

  return VideoNode;
}(_node.Node);

/***/ }),

/***/ "./src/scenes/scene.js":
/*!*****************************!*\
  !*** ./src/scenes/scene.js ***!
  \*****************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Scene = exports.WebXRView = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _renderer = __webpack_require__(/*! ../core/renderer.js */ "./src/core/renderer.js");

var _inputRenderer = __webpack_require__(/*! ../nodes/input-renderer.js */ "./src/nodes/input-renderer.js");

var _statsViewer = __webpack_require__(/*! ../nodes/stats-viewer.js */ "./src/nodes/stats-viewer.js");

var _node = __webpack_require__(/*! ../core/node.js */ "./src/core/node.js");

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var WebXRView = exports.WebXRView = function (_RenderView) {
  _inherits(WebXRView, _RenderView);

  function WebXRView(view, pose, layer) {
    _classCallCheck(this, WebXRView);

    return _possibleConstructorReturn(this, (WebXRView.__proto__ || Object.getPrototypeOf(WebXRView)).call(this, view ? view.projectionMatrix : null, pose && view ? pose.getViewMatrix(view) : null, layer && view ? layer.getViewport(view) : null, view ? view.eye : 'left'));
  }

  return WebXRView;
}(_renderer.RenderView);

var Scene = exports.Scene = function (_Node) {
  _inherits(Scene, _Node);

  function Scene() {
    _classCallCheck(this, Scene);

    var _this2 = _possibleConstructorReturn(this, (Scene.__proto__ || Object.getPrototypeOf(Scene)).call(this));

    _this2._timestamp = -1;
    _this2._frameDelta = 0;
    _this2._statsStanding = false;
    _this2._stats = null;
    _this2._statsEnabled = false;
    _this2.enableStats(true); // Ensure the stats are added correctly by default.

    _this2._inputRenderer = null;
    _this2._resetInputEndFrame = true;

    _this2._lastTimestamp = 0;

    _this2._hoverFrame = 0;
    _this2._hoveredNodes = [];

    _this2.clear = true;
    return _this2;
  }

  _createClass(Scene, [{
    key: 'setRenderer',
    value: function setRenderer(renderer) {
      this._setRenderer(renderer);
    }
  }, {
    key: 'loseRenderer',
    value: function loseRenderer() {
      if (this._renderer) {
        this._stats = null;
        this._renderer = null;
        this._inputRenderer = null;
      }
    }
  }, {
    key: 'updateInputSources',


    // Helper function that automatically adds the appropriate visual elements for
    // all input sources.
    value: function updateInputSources(frame, frameOfRef) {
      // FIXME: Check for the existence of the API first. This check should be
      // removed once the input API is part of the official spec.
      if (!frame.session.getInputSources) {
        return;
      }

      var inputSources = frame.session.getInputSources();

      var newHoveredNodes = [];
      var lastHoverFrame = this._hoverFrame;
      this._hoverFrame++;

      var _iteratorNormalCompletion = true;
      var _didIteratorError = false;
      var _iteratorError = undefined;

      try {
        for (var _iterator = inputSources[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
          var inputSource = _step.value;

          var inputPose = frame.getInputPose(inputSource, frameOfRef);

          if (!inputPose) {
            continue;
          }

          // Any time that we have a grip matrix, we'll render a controller.
          if (inputPose.gripMatrix) {
            this.inputRenderer.addController(inputPose.gripMatrix);
          }

          if (inputPose.pointerMatrix) {
            if (inputSource.pointerOrigin == 'hand') {
              // If we have a pointer matrix and the pointer origin is the users
              // hand (as opposed to their head or the screen) use it to render
              // a ray coming out of the input device to indicate the pointer
              // direction.
              this.inputRenderer.addLaserPointer(inputPose.pointerMatrix);
            }

            // If we have a pointer matrix we can also use it to render a cursor
            // for both handheld and gaze-based input sources.

            // Check and see if the pointer is pointing at any selectable objects.
            var hitResult = this.hitTest(inputPose.pointerMatrix);

            if (hitResult) {
              // Render a cursor at the intersection point.
              this.inputRenderer.addCursor(hitResult.intersection);

              if (hitResult.node._hoverFrameId != lastHoverFrame) {
                hitResult.node.onHoverStart();
              }
              hitResult.node._hoverFrameId = this._hoverFrame;
              newHoveredNodes.push(hitResult.node);
            } else {
              // Statically render the cursor 1 meters down the ray since we didn't
              // hit anything selectable.
              var cursorPos = _glMatrix.vec3.fromValues(0, 0, -1.0);
              _glMatrix.vec3.transformMat4(cursorPos, cursorPos, inputPose.pointerMatrix);
              this.inputRenderer.addCursor(cursorPos);
            }
          }
        }
      } catch (err) {
        _didIteratorError = true;
        _iteratorError = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion && _iterator.return) {
            _iterator.return();
          }
        } finally {
          if (_didIteratorError) {
            throw _iteratorError;
          }
        }
      }

      var _iteratorNormalCompletion2 = true;
      var _didIteratorError2 = false;
      var _iteratorError2 = undefined;

      try {
        for (var _iterator2 = this._hoveredNodes[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
          var hoverNode = _step2.value;

          if (hoverNode._hoverFrameId != this._hoverFrame) {
            hoverNode.onHoverEnd();
          }
        }
      } catch (err) {
        _didIteratorError2 = true;
        _iteratorError2 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion2 && _iterator2.return) {
            _iterator2.return();
          }
        } finally {
          if (_didIteratorError2) {
            throw _iteratorError2;
          }
        }
      }

      this._hoveredNodes = newHoveredNodes;
    }
  }, {
    key: 'handleSelect',
    value: function handleSelect(inputSource, frame, frameOfRef) {
      var inputPose = frame.getInputPose(inputSource, frameOfRef);

      if (!inputPose) {
        return;
      }

      this.handleSelectPointer(inputPose.pointerMatrix);
    }
  }, {
    key: 'handleSelectPointer',
    value: function handleSelectPointer(pointerMatrix) {
      if (pointerMatrix) {
        // Check and see if the pointer is pointing at any selectable objects.
        var hitResult = this.hitTest(pointerMatrix);

        if (hitResult) {
          // Render a cursor at the intersection point.
          hitResult.node.handleSelect();
        }
      }
    }
  }, {
    key: 'enableStats',
    value: function enableStats(enable) {
      if (enable == this._statsEnabled) {
        return;
      }

      this._statsEnabled = enable;

      if (enable) {
        this._stats = new _statsViewer.StatsViewer();
        this._stats.selectable = true;
        this.addNode(this._stats);

        if (this._statsStanding) {
          this._stats.translation = [0, 1.4, -0.75];
        } else {
          this._stats.translation = [0, -0.3, -0.5];
        }
        this._stats.scale = [0.3, 0.3, 0.3];
        _glMatrix.quat.fromEuler(this._stats.rotation, -45.0, 0.0, 0.0);
      } else if (!enable) {
        if (this._stats) {
          this.removeNode(this._stats);
          this._stats = null;
        }
      }
    }
  }, {
    key: 'standingStats',
    value: function standingStats(enable) {
      this._statsStanding = enable;
      if (this._stats) {
        if (this._statsStanding) {
          this._stats.translation = [0, 1.4, -0.75];
        } else {
          this._stats.translation = [0, -0.3, -0.5];
        }
        this._stats.scale = [0.3, 0.3, 0.3];
        _glMatrix.quat.fromEuler(this._stats.rotation, -45.0, 0.0, 0.0);
      }
    }
  }, {
    key: 'draw',
    value: function draw(projectionMatrix, viewMatrix, eye) {
      var view = new _renderer.RenderView();
      view.projectionMatrix = projectionMatrix;
      view.viewMatrix = viewMatrix;
      if (eye) {
        view.eye = eye;
      }

      this.drawViewArray([view]);
    }

    /** Draws the scene into the base layer of the XRFrame's session */

  }, {
    key: 'drawXRFrame',
    value: function drawXRFrame(xrFrame, pose) {
      if (!this._renderer || !pose) {
        return;
      }

      var gl = this._renderer.gl;
      var session = xrFrame.session;
      // Assumed to be a XRWebGLLayer for now.
      var layer = session.baseLayer;

      if (!gl) {
        return;
      }

      gl.bindFramebuffer(gl.FRAMEBUFFER, layer.framebuffer);

      if (this.clear) {
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      }

      var views = [];
      var _iteratorNormalCompletion3 = true;
      var _didIteratorError3 = false;
      var _iteratorError3 = undefined;

      try {
        for (var _iterator3 = xrFrame.views[Symbol.iterator](), _step3; !(_iteratorNormalCompletion3 = (_step3 = _iterator3.next()).done); _iteratorNormalCompletion3 = true) {
          var view = _step3.value;

          views.push(new WebXRView(view, pose, layer));
        }
      } catch (err) {
        _didIteratorError3 = true;
        _iteratorError3 = err;
      } finally {
        try {
          if (!_iteratorNormalCompletion3 && _iterator3.return) {
            _iterator3.return();
          }
        } finally {
          if (_didIteratorError3) {
            throw _iteratorError3;
          }
        }
      }

      this.drawViewArray(views);
    }
  }, {
    key: 'drawViewArray',
    value: function drawViewArray(views) {
      // Don't draw when we don't have a valid context
      if (!this._renderer) {
        return;
      }

      this._renderer.drawViews(views, this);
    }
  }, {
    key: 'startFrame',
    value: function startFrame() {
      var prevTimestamp = this._timestamp;
      this._timestamp = performance.now();
      if (this._stats) {
        this._stats.begin();
      }

      if (prevTimestamp >= 0) {
        this._frameDelta = this._timestamp - prevTimestamp;
      } else {
        this._frameDelta = 0;
      }

      this._update(this._timestamp, this._frameDelta);

      return this._frameDelta;
    }
  }, {
    key: 'endFrame',
    value: function endFrame() {
      if (this._inputRenderer && this._resetInputEndFrame) {
        this._inputRenderer.reset();
      }

      if (this._stats) {
        this._stats.end();
      }
    }

    // Override to load scene resources on construction or context restore.

  }, {
    key: 'onLoadScene',
    value: function onLoadScene(renderer) {
      return Promise.resolve();
    }
  }, {
    key: 'inputRenderer',
    get: function get() {
      if (!this._inputRenderer) {
        this._inputRenderer = new _inputRenderer.InputRenderer();
        this.addNode(this._inputRenderer);
      }
      return this._inputRenderer;
    }
  }]);

  return Scene;
}(_node.Node);

/***/ }),

/***/ "./src/util/fallback-helper.js":
/*!*************************************!*\
  !*** ./src/util/fallback-helper.js ***!
  \*************************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.FallbackHelper = undefined;

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }(); // Copyright 2018 The Immersive Web Community Group
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

var _glMatrix = __webpack_require__(/*! ../math/gl-matrix.js */ "./src/math/gl-matrix.js");

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var LOOK_SPEED = 0.0025;

var FallbackHelper = exports.FallbackHelper = function () {
  function FallbackHelper(scene, gl) {
    var _this = this;

    _classCallCheck(this, FallbackHelper);

    this.scene = scene;
    this.gl = gl;
    this._emulateStage = false;

    this.lookYaw = 0;
    this.lookPitch = 0;

    this.viewMatrix = _glMatrix.mat4.create();

    var projectionMatrix = _glMatrix.mat4.create();
    this.projectionMatrix = projectionMatrix;

    // Using a simple identity matrix for the view.
    _glMatrix.mat4.identity(this.viewMatrix);

    // We need to track the canvas size in order to resize the WebGL
    // backbuffer width and height, as well as update the projection matrix
    // and adjust the viewport.
    function onResize() {
      gl.canvas.width = gl.canvas.offsetWidth * window.devicePixelRatio;
      gl.canvas.height = gl.canvas.offsetHeight * window.devicePixelRatio;
      _glMatrix.mat4.perspective(projectionMatrix, Math.PI * 0.4, gl.canvas.width / gl.canvas.height, 0.1, 1000.0);
      gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    }
    window.addEventListener('resize', onResize);
    onResize();

    // Upding the view matrix with touch or mouse events.
    var canvas = gl.canvas;
    var lastTouchX = 0;
    var lastTouchY = 0;
    canvas.addEventListener('touchstart', function (ev) {
      if (ev.touches.length == 2) {
        lastTouchX = ev.touches[1].pageX;
        lastTouchY = ev.touches[1].pageY;
      }
    });
    canvas.addEventListener('touchmove', function (ev) {
      // Rotate the view when two fingers are being used.
      if (ev.touches.length == 2) {
        _this.onLook(ev.touches[1].pageX - lastTouchX, ev.touches[1].pageY - lastTouchY);
        lastTouchX = ev.touches[1].pageX;
        lastTouchY = ev.touches[1].pageY;
      }
    });
    canvas.addEventListener('mousemove', function (ev) {
      // Only rotate when the right button is pressed.
      if (ev.buttons & 2) {
        _this.onLook(ev.movementX, ev.movementY);
      }
    });
    canvas.addEventListener('contextmenu', function (ev) {
      // Prevent context menus on the canvas so that we can use right click to rotate.
      ev.preventDefault();
    });

    this.boundOnFrame = this.onFrame.bind(this);
    window.requestAnimationFrame(this.boundOnFrame);
  }

  _createClass(FallbackHelper, [{
    key: 'onLook',
    value: function onLook(yaw, pitch) {
      this.lookYaw += yaw * LOOK_SPEED;
      this.lookPitch += pitch * LOOK_SPEED;

      // Clamp pitch rotation beyond looking straight up or down.
      if (this.lookPitch < -Math.PI * 0.5) {
        this.lookPitch = -Math.PI * 0.5;
      }
      if (this.lookPitch > Math.PI * 0.5) {
        this.lookPitch = Math.PI * 0.5;
      }

      this.updateView();
    }
  }, {
    key: 'onFrame',
    value: function onFrame(t) {
      var gl = this.gl;
      window.requestAnimationFrame(this.boundOnFrame);

      this.scene.startFrame();

      // We can skip setting the framebuffer and viewport every frame, because
      // it won't change from frame to frame and we're updating the viewport
      // only when we resize for efficency.
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

      // We're drawing with our own projection and view matrix now, and we
      // don't have a list of view to loop through, but otherwise all of the
      // WebGL drawing logic is exactly the same.
      this.scene.draw(this.projectionMatrix, this.viewMatrix);

      this.scene.endFrame();
    }
  }, {
    key: 'updateView',
    value: function updateView() {
      _glMatrix.mat4.identity(this.viewMatrix);

      _glMatrix.mat4.rotateX(this.viewMatrix, this.viewMatrix, -this.lookPitch);
      _glMatrix.mat4.rotateY(this.viewMatrix, this.viewMatrix, -this.lookYaw);

      // If we're emulating a stage frame of reference we'll need to move the view
      // matrix roughly a meter and a half up in the air.
      if (this._emulateStage) {
        _glMatrix.mat4.translate(this.viewMatrix, this.viewMatrix, [0, -1.6, 0]);
      }
    }
  }, {
    key: 'emulateStage',
    get: function get() {
      return this._emulateStage;
    },
    set: function set(value) {
      this._emulateStage = value;
      this.updateView();
    }
  }]);

  return FallbackHelper;
}();

/***/ })

/******/ });
});
//# sourceMappingURL=cottontail.debug.js.map