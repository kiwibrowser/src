description("Test ImageData constructor");

function setRGBA(imageData, i, rgba) {
    var s = i * 4;
    imageData[s] = rgba[0];
    imageData[s + 1] = rgba[1];
    imageData[s + 2] = rgba[2];
    imageData[s + 3] = rgba[3];
}

function getRGBA(imageData, i) {
    var result = [];
    var s = i * 4;
    for (var j = 0; j < 4; j++) {
        result[j] = imageData[s + j];
    }
    return result;
}

shouldBeDefined("ImageData");
shouldBe("ImageData.length", "2");

imageData = new ImageData(100, 50);

shouldBeNonNull("imageData");
shouldBeNonNull("imageData.data");
shouldBe("imageData.width", "100");
shouldBe("imageData.height", "50");
shouldBe("getRGBA(imageData.data, 4)", "[0, 0, 0, 0]");

var testColor = [0, 255, 255, 128];
setRGBA(imageData.data, 4, testColor);
shouldBe("getRGBA(imageData.data, 4)", "testColor");

shouldThrow("new ImageData(10)");
shouldThrow("new ImageData(0, 10)");
shouldThrow("new ImageData(10, 0)");
shouldThrow("new ImageData('width', 'height')");
shouldThrow("new ImageData(1 << 31, 1 << 31)");

shouldThrow("new ImageData(new Uint8ClampedArray(0))");
shouldThrow("new ImageData(new Uint8Array(100), 25)");
shouldThrow("new ImageData(new Uint8ClampedArray(27), 2)");
shouldThrow("new ImageData(new Uint8ClampedArray(28), 7, 0)");
shouldThrow("new ImageData(new Uint8ClampedArray(104), 14)");
shouldThrow("new ImageData(new Uint8ClampedArray([12, 34, 168, 65328]), 1, 151)");
shouldThrow("new ImageData(self, 4, 4)");
shouldThrow("new ImageData(null, 4, 4)");
shouldThrow("new ImageData(imageData.data, 0)");
shouldThrow("new ImageData(imageData.data, 13)");
shouldThrow("new ImageData(imageData.data, 1 << 31)");
shouldThrow("new ImageData(imageData.data, 'biggish')");
shouldThrow("new ImageData(imageData.data, 1 << 24, 1 << 31)");
shouldBe("(new ImageData(new Uint8ClampedArray(28), 7)).height", "1");

imageDataFromData = new ImageData(imageData.data, 100);
shouldBe("imageDataFromData.width", "100");
shouldBe("imageDataFromData.height", "50");
shouldBe("imageDataFromData.data", "imageData.data");
shouldBe("getRGBA(imageDataFromData.data, 10)", "getRGBA(imageData.data, 10)");
setRGBA(imageData.data, 10, testColor);
shouldBe("getRGBA(imageDataFromData.data, 10)", "getRGBA(imageData.data, 10)");

var data = new Uint8ClampedArray(400);
data[22] = 129;
imageDataFromData = new ImageData(data, 20, 5);
shouldBe("imageDataFromData.width", "20");
shouldBe("imageDataFromData.height", "5");
shouldBe("imageDataFromData.data", "data");
shouldBe("getRGBA(imageDataFromData.data, 2)", "getRGBA(data, 2)");
setRGBA(imageDataFromData.data, 2, testColor);
shouldBe("getRGBA(imageDataFromData.data, 2)", "getRGBA(data, 2)");
