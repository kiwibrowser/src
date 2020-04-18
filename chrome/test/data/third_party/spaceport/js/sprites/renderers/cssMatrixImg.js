define([ 'util/ensureCallback', 'features', 'sprites/renderers/DomContext', 'util/create', 'sprites/container' ], function (ensureCallback, features, DomContext, create, container) {
    var CSSMatrix = features.CSSMatrix;

    function RenderContext(sourceData, frameData) {
        if (!CSSMatrix) {
            return;
        }

        this.loadPromise = quickPromise();
        DomContext.call(this, sourceData, frameData, this.loadPromise.resolve);

        this.elements.forEach(function (frameElements) {
            frameElements.forEach(function (element) {
                element.style[features.transformOriginStyleProperty] = '0 0';
            });
        });

        this.transformData = frameData.map(function (objectTransforms) {
            return objectTransforms.map(function (t) {
                var m = new CSSMatrix();
                m.a = t.matrix[0];
                m.b = t.matrix[1];
                m.c = t.matrix[3];
                m.d = t.matrix[4];
                m.e = t.matrix[2];
                m.f = t.matrix[5];
                return m;
            });
        });

        this.containerElement = container();
    }

    RenderContext.prototype = create(DomContext.prototype);

    RenderContext.prototype.load = function load(callback) {
        callback = ensureCallback(callback);

        if (!CSSMatrix) {
            callback(new Error('Not supported'));
            return;
        }

        document.body.appendChild(this.containerElement);

        this.loadPromise.then(function () {
            callback(null);
        });
    };

    RenderContext.prototype.unload = function unload() {
        this.containerElement.parentNode.removeChild(this.containerElement);
        DomContext.prototype.unload.call(this);
    };

    var transformStyleProperty = features.transformStyleProperty;

    RenderContext.prototype.processElements = function processElements(elements, transforms) {
        var count = transforms.length;
        var i;
        for (i = 0; i < count; ++i) {
            var element = elements[i];
            element.style[transformStyleProperty] = transforms[i];
            element.zIndex = i;

            // Elements not in the DOM need to be added
            if (!element.parentNode) {
                this.containerElement.appendChild(element);
            }
        }
    };

    return function (element, frameData) {
        return new RenderContext(element, frameData);
    };
});
