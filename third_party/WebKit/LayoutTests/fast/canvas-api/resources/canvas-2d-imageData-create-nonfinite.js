description("Test the argument bounds of canvas createImageData.");

var canvas = document.getElementById('canvas');
var ctx = canvas.getContext('2d');

shouldThrow("ctx.createImageData(Infinity, Infinity)");
shouldThrow("ctx.createImageData(Infinity, 10)");
shouldThrow("ctx.createImageData(-Infinity, 10)");
shouldThrow("ctx.createImageData(10, Infinity)");
shouldThrow("ctx.createImageData(10, -Infinity)");
shouldThrow("ctx.createImageData(NaN, 10)");
shouldThrow("ctx.createImageData(10, NaN)");
