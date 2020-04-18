define([ 'util/ensureCallback' ], function (ensureCallback) {
    var IMAGE_SRC = 'assets/monstro-fada.png';

    var FRAME_WIDTH = 37 * 3;
    var FRAME_HEIGHT = 58 * 3;

    var FRAMES_HORIZ = 6;
    var FRAMES_VERT = 1;

    var TOTAL_FRAMES = FRAMES_HORIZ * FRAMES_VERT;

    function ImageSource(img) {
        this.img = img;

        var canvas = document.createElement('canvas');
        canvas.width = FRAME_WIDTH;
        canvas.height = FRAME_HEIGHT;

        var context = canvas.getContext('2d');
        context.globalCompositeOperation = 'copy';

        this.frameImages = [ ];
        this.frameInfos = [ ];
        var x, y;
        for (y = 0; y < FRAMES_VERT; ++y) {
            for (x = 0; x < FRAMES_HORIZ; ++x) {
                var px = x * FRAME_WIDTH;
                var py = y * FRAME_WIDTH;

                context.drawImage(
                    img,
                    px, py,
                    FRAME_WIDTH, FRAME_HEIGHT,
                    0, 0,
                    FRAME_WIDTH, FRAME_HEIGHT
                );

                var frameImage = new window.Image();
                frameImage.src = canvas.toDataURL();

                this.frameInfos.push({
                    x: px,
                    y: py,
                    width: FRAME_WIDTH,
                    height: FRAME_HEIGHT,
                    image: frameImage,
                    sheetImage: img
                });
            }
        }

        // TODO Cycle frameInfos
    }

    ImageSource.prototype.getImage = function getImage(frameIndex) {
        return this.frameInfos[frameIndex % TOTAL_FRAMES].image;
    };

    ImageSource.prototype.drawToCanvas = function drawToCanvas(context, dx, dy, frameIndex) {
        var frameInfo = this.frameInfos[frameIndex % TOTAL_FRAMES];

        context.drawImage(
            frameInfo.sheetImage,
            frameInfo.x, frameInfo.y,
            frameInfo.width, frameInfo.height,
            dx, dy,
            frameInfo.width, frameInfo.height
        );
    };

    ImageSource.prototype.getFrameInfo = function getFrameInfo(frameIndex) {
        return this.frameInfos[frameIndex % TOTAL_FRAMES];
    };

    return function spriteSheet(callback) {
        callback = ensureCallback(callback);

        var img = new window.Image();
        img.onload = function () {
            var imageSource = new ImageSource(img);
            callback(null, imageSource);
        };
        img.src = IMAGE_SRC;
    };
});
