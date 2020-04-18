self.onmessage = function(e) {
    createImageBitmap(e.data, {imageOrientation: "none", premultiplyAlpha: "none"}).then(imageBitmap => {
        postMessage(imageBitmap, [imageBitmap]);
    });
};
