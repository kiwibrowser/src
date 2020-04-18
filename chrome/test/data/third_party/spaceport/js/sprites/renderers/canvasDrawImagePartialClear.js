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
            // We clear with an extra 1px border to clear smoothed edges
            var frameInfo = sourceData.getFrameInfo(this.previousFrameIndex);
            var width = frameInfo.width + 2;
            var height = frameInfo.height + 2;
            for (i = 0; i < count; ++i) {
                var m = previousTransforms[i].matrix;
                context.setTransform(m[0], m[1], m[3], m[4], m[2], m[5]);
                context.clearRect(-1, -1, width, height);
            }
        }

        for (i = 0; i < count; ++i) {
            var m = transforms[i].matrix;
            context.setTransform(m[0], m[1], m[3], m[4], m[2], m[5]);
            sourceData.drawToCanvas(context, 0, 0, frameIndex);
        }

        this.previousTransforms = transforms;
        this.previousFrameIndex = frameIndex;
    };

    return function (element, frameData) {
        return new RenderContext(element, frameData);
    };
});
