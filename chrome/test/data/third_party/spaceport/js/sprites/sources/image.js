define([ 'util/ensureCallback' ], function (ensureCallback) {
    var IMAGE_SRC = 'assets/html5-logo.png';

    function ImageSource(img) {
        this.img = img;

        this.frameInfo = {
            x: 0,
            y: 0,
            width: img.width,
            height: img.height,
            image: img,
            sheetImage: img
        };
    }

    ImageSource.prototype.getImage = function getImage(frameIndex) {
        return this.img;
    };

    ImageSource.prototype.drawToCanvas = function drawToCanvas(context, dx, dy, frameIndex) {
        context.drawImage(this.img, dx, dy);
    };

    ImageSource.prototype.getFrameInfo = function getFrameInfo(frameIndex) {
        return this.frameInfo;
    };

    return function image(callback) {
        callback = ensureCallback(callback);

        var img = new window.Image();
        img.onload = function () {
            var imageSource = new ImageSource(img);
            callback(null, imageSource);
        };
        img.src = IMAGE_SRC;
    };
});
