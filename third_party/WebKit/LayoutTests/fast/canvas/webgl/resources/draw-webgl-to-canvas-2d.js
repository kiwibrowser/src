if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}

var preserve_ctx2D;
var preserve_canvas3D;
var preserve_gl;
var nonpreserve_ctx2D;
var nonpreserve_canvas3D;
var nonpreserve_gl;
var imgdata;

function renderWebGL(gl) {
    gl.clearColor(0, 1, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
}

function drawWebGLToCanvas2D(ctx2D, canvas3D, isDrawingBufferUndefined) {
    // draw red rect on canvas 2d.
    ctx2D.fillStyle = 'red';
    ctx2D.fillRect(0, 0, 100, 100);
    var imageData = ctx2D.getImageData(0, 0, 1, 1);
    imgdata = imageData.data;
    shouldBe("imgdata[0]", "255");
    shouldBe("imgdata[1]", "0");
    shouldBe("imgdata[2]", "0");

    // draw the webgl contents (green rect) on the canvas 2d context.
    ctx2D.drawImage(canvas3D, 0, 0);
    ctx2D.getImageData(0, 0, 1, 1);
    imageData = ctx2D.getImageData(0, 0, 1, 1);
    imgdata = imageData.data;
    if (isDrawingBufferUndefined) {
        // Current implementation draws transparent texture on the canvas 2d context,
        // although the spec said it leads to undefined behavior.
        shouldBe("imgdata[0]", "255");
        shouldBe("imgdata[1]", "0");
        shouldBe("imgdata[2]", "0");
    } else {
        shouldBe("imgdata[0]", "0");
        shouldBe("imgdata[1]", "255");
        shouldBe("imgdata[2]", "0");
    }

    if (isDrawingBufferUndefined && window.testRunner)
        testRunner.notifyDone();
}

function asyncTest() {
    debug("Check for drawing webgl to canvas 2d several frames after drawing webgl contents.")
    debug("1) when drawingBuffer is preserved.")
    drawWebGLToCanvas2D(preserve_ctx2D, preserve_canvas3D, false);
    debug("2) when drawingBuffer is not preserved. It leads to undefined behavior.")
    drawWebGLToCanvas2D(nonpreserve_ctx2D, nonpreserve_canvas3D, true);
}

function startTestAfterFirstPaint() {
    // create both canvas 2d and webgl contexts.
    createContexts();
    // prepare webgl contents.
    renderWebGL(preserve_gl);
    renderWebGL(nonpreserve_gl);

    debug("Check for drawing webgl to canvas 2d on the same frame.")
    debug("1) when drawingBuffer is preserved.")
    drawWebGLToCanvas2D(preserve_ctx2D, preserve_canvas3D, false);
    debug("2) when drawingBuffer is not preserved.")
    drawWebGLToCanvas2D(nonpreserve_ctx2D, nonpreserve_canvas3D, false);

    if (window.testRunner) {
        testRunner.waitUntilDone();
        testRunner.layoutAndPaintAsyncThen(asyncTest);
    } else {
        window.requestAnimationFrame(asyncTest);
    }
}

window.onload = function () {
    window.requestAnimationFrame(startTestAfterFirstPaint);
}
