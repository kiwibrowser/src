var numToBlobCalls = 9;
var numToDataURLCalls = 3;
var testImages = [];
var canvasCtxs = [];

// Create an original canvas with content
var canvas = document.createElement("canvas");
var ctx = canvas.getContext("2d");
ctx.fillStyle = "#EE21AF";
ctx.fillRect(0, 0, 250, 150);

function testIfAllImagesAreCorrect(asyncTest)
{
    // All resultant images should be the same as both async and main threads use the same image encoder
    var imageMatched = true;
    var firstImageData = canvasCtxs[0].getImageData(0, 0, 250, 150).data;
    for (var i = 1; i < (numToBlobCalls + numToDataURLCalls); i++)
    {
        var nextImageData = canvasCtxs[i].getImageData(0, 0, 250, 150).data;
        for (var k = 0; k < firstImageData.length; k++)
        {
            if (firstImageData[k]!=nextImageData[k])
            {
                imageMatched = false;
                break;
            }
        }
        if (!imageMatched)
            break;
    }
    asyncTest.step(() => {
      assert_true(imageMatched, 'Images match');
    });
}

var counter = numToBlobCalls + numToDataURLCalls;
function onCanvasDrawCompleted(asyncTest)
{
    counter = counter - 1;
    if (counter == 0) {
        testIfAllImagesAreCorrect(asyncTest);
        asyncTest.done();
    }
}

function createTestCase(i, asyncTest)
{
    var canvas_test = document.createElement("canvas");
    var ctx_test = canvas_test.getContext("2d");
    canvasCtxs[i] = ctx_test;

    // Drawing canvas onto itself will force allocation of the backing store
    // and prevent getImageData from switching out of gpu-acceleration when
    // running this test under virtual/gpu
    canvasCtxs[i].drawImage(canvas_test, 0, 0);

    var newImg = new Image();
    newImg.onload = function() {
        ctx_test.drawImage(newImg, 0, 0, 250, 150);
        onCanvasDrawCompleted(asyncTest);
    }
    testImages[i] = newImg;
}

function createAllTestCases(asyncTest) {
    for (var i = 0; i < (numToBlobCalls + numToDataURLCalls); i++)
        createTestCase(i, asyncTest);
}
