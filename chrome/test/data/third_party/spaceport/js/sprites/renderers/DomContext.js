define([ 'util/ensureCallback' ], function (ensureCallback) {
    function DomContext(sourceData, frameData, onLoad) {
        var onLoadCalled = 0;
        var onLoadExpected = 0;
        var onLoadReady = false;

        function checkOnLoad() {
            if (onLoadReady) {
                if (onLoadCalled === onLoadExpected) {
                    onLoad();
                }
            }
        }

        var sourcePool = [ ];
        var elementPool = [ ];
        var elements = [ ];
        var i, j;

        for (i = 0; i < frameData.length; ++i) {
            var objectTransforms = frameData[i];
            var frameElements = [ ];
            var sourceElements = [ ];

            for (j = 0; j < objectTransforms.length; ++j) {
                var element;
                var img = sourceData.getImage(i);
                var index = sourcePool.indexOf(img);

                if (index >= 0) {
                    // Image is available in the pool; take it
                    element = elementPool.splice(index, 1)[0];
                    sourcePool.splice(index, 1);
                } else {
                    // Image not in the pool; add it
                    element = img.cloneNode(true);
                    element.style.position = 'absolute';
                    element.style.left = '0';
                    element.style.top = '0';

                    if (element instanceof Image) {
                        onLoadExpected += 1;
                        element.onload = function () {
                            onLoadCalled += 1;
                            checkOnLoad();
                        };
                        element.src = element.src;
                    }
                }

                sourceElements.push(img);
                frameElements.push(element);
            }

            sourcePool.push.apply(sourcePool, sourceElements);
            elementPool.push.apply(elementPool, frameElements);
            elements.push(frameElements);
        }

        onLoadReady = true;
        checkOnLoad();

        this.elements = elements;
        this.activeElements = null;

        this.transformData = null;
    }

    DomContext.prototype.unload = function unload() {
        this.activeElements.forEach(function (el) {
            if (el.parentNode) {
                el.parentNode.removeChild(el);
            }
        });

        this.activeElements = null;
    };

    DomContext.prototype.renderFrame = function renderFrame(frameIndex) {
        var transforms = this.transformData[frameIndex];
        var elements = this.elements[frameIndex];

        this.processElements(elements, transforms);

        // Elements no longer displayed must be removed from the DOM
        var activeElements = this.activeElements;
        if (activeElements) {
            var count = activeElements.length;
            var i;
            for (i = 0; i < count; ++i) {
                var element = activeElements[i];
                if (element.parentNode && elements.indexOf(element) < 0) {
                    element.parentNode.removeChild(element);
                }
            }
        }

        this.activeElements = elements;
    };

    return DomContext;
});
