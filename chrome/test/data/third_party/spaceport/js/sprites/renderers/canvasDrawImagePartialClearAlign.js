define([ 'util/ensureCallback', 'sprites/canvas' ], function (ensureCallback, canvas) {
    function RenderContext(sourceData, frameData) {
        this.sourceData = sourceData;
        this.frameData = frameData;

        this.previousTransforms = null;
        this.previousFrameIndex = null;

        this.canvas = canvas();

        this.context = this.canvas.getContext('2d');
        this.context.globalCompositeOperation = 'source-over';
    }

    RenderContext.prototype.load = function load(callback) {
        callback = ensureCallback(callback);

        // Anything not simply translated is disallowed
        var isValid = this.frameData.every(function (frame) {
            return frame.every(function (transform) {
                return transform.matrix[0] === 1
                    && transform.matrix[1] === 0
                    && transform.matrix[3] === 0
                    && transform.matrix[4] === 1;
            });
        });

        if (!isValid) {
            callback(new Error("Not supported"));
            return;
        }

        document.body.appendChild(this.canvas);

        callback(null);
    };

    RenderContext.prototype.unload = function unload() {
        if (this.canvas.parentNode) {
            this.canvas.parentNode.removeChild(this.canvas);
        }
    };

    RenderContext.prototype.clear = function clear() {
        this.canvas.width = this.canvas.width;
        this.previousTransforms = null;
    };

    RenderContext.prototype.renderFrame = function renderFrame(frameIndex) {
        var context = this.context;
        var sourceData = this.sourceData;

        var transforms = this.frameData[frameIndex];
        var count = transforms.length;
        var i;

        var previousTransforms = this.previousTransforms;
        if (previousTransforms) {
            var frameInfo = sourceData.getFrameInfo(this.previousFrameIndex);
            var width = frameInfo.width;
            var height = frameInfo.height;
            for (i = 0; i < count; ++i) {
                var transform = previousTransforms[i];
                context.setTransform(1, 0, 0, 1, Math.floor(transform.x), Math.floor(transform.y));
                context.clearRect(0, 0, width, height);
            }
        }

        for (i = 0; i < count; ++i) {
            var transform = transforms[i];
            context.setTransform(1, 0, 0, 1, Math.floor(transform.x), Math.floor(transform.y));
            sourceData.drawToCanvas(context, 0, 0, frameIndex);
        }

        this.previousTransforms = transforms;
        this.previousFrameIndex = frameIndex;
    };

    return function (element, frameData) {
        return new RenderContext(element, frameData);
    };
});
