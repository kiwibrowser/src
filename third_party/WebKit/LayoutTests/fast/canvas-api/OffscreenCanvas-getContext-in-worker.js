importScripts("../../resources/testharness.js");
test(function() {
    var aCanvas = new OffscreenCanvas(50, 50);
    var bCanvas = new OffscreenCanvas(50, 50);
    var ctx1 = aCanvas.getContext('2d');
    var ctx2 = bCanvas.getContext('webgl');
    assert_true(ctx1 instanceof OffscreenCanvasRenderingContext2D);
    assert_true(ctx2 instanceof WebGLRenderingContext);
});
done();
