var webgl2 = false;
var webglCanvas = document.getElementById("webgl-canvas");
if (!webglCanvas) {
  webglCanvas = document.getElementById("webgl2-canvas");
  webgl2 = true;
}
var glAttributes = {
  alpha : false,
  antialias : false,
};
var gl = webglCanvas.getContext(webgl2 ? "webgl2" : "webgl", glAttributes);

function runWithUserGesture(fn) {
  function thunk() {
    document.removeEventListener("keypress", thunk, false);
    fn()
  }
  document.addEventListener("keypress", thunk, false);
  eventSender.keyDown(" ", []);
}
