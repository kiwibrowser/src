define([ 'util/ensureCallback', 'features', 'Modernizr', 'sprites/container' ], function (ensureCallback, features, Modernizr, container) {
    function RenderContext(sourceData, frameData) {
        if (!Modernizr.csstransforms) {
            return;
        }

        this.sourceData = sourceData;

        this.transformData = frameData.map(function (objectTransforms) {
            return objectTransforms.map(function (t) {
                return t.cssTransform2d;
            });
        });

        this.elements = frameData[0].map(function () {
            var el = document.createElement('div');
            el.style.position = 'absolute';
            el.style.top = '0';
            el.style.left = '0';
            el.style.display = 'block';
            el.style[features.transformOriginStyleProperty] = '0 0';
            return el;
        });

        this.containerElement = container();
    }

    RenderContext.prototype.load = function load(callback) {
        callback = ensureCallback(callback);

        if (!Modernizr.csstransforms) {
            callback(new Error('Not supported'));
            return;
        }

        this.elements.forEach(function (element) {
            this.containerElement.appendChild(element);
        }, this);

        document.body.appendChild(this.containerElement);

        callback(null);
    };

    RenderContext.prototype.unload = function unload() {
        this.containerElement.parentNode.removeChild(this.containerElement);

        this.elements.forEach(function (element) {
            if (element.parentNode) {
                element.parentNode.removeChild(element);
            }
        });
    };

    var transformStyleProperty = features.transformStyleProperty;

    RenderContext.prototype.renderFrame = function renderFrame(frameIndex) {
        var transforms = this.transformData[frameIndex];
        var elements = this.elements;
        var count = elements.length;

        var frameInfo = this.sourceData.getFrameInfo(frameIndex);

        var i;
        for (i = 0; i < count; ++i) {
            var element = elements[i];

            var backgroundImage = 'url(' + frameInfo.sheetImage.src + ')';
            if (element.style.backgroundImage !== backgroundImage) {
                // Chrome has flickering issues without this check
                element.style.backgroundImage = backgroundImage;
            }

            element.style[transformStyleProperty] = transforms[i];
            element.style.backgroundPosition = -frameInfo.x + 'px ' + -frameInfo.y + 'px';
            element.style.width = frameInfo.width + 'px';
            element.style.height = frameInfo.height + 'px';
        }
    };

    return function (sourceData, frameData) {
        return new RenderContext(sourceData, frameData);
    };
});
