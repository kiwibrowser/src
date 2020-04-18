define([ 'util/ensureCallback', 'features', 'Modernizr', 'sprites/renderers/DomContext', 'util/create', 'sprites/container', 'util/quickPromise' ], function (ensureCallback, features, Modernizr, DomContext, create, container, quickPromise) {
    function RenderContext(sourceData, frameData) {
        if (!Modernizr.csstransforms) {
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
                return t.cssTransform2d;
            });
        });

        this.containerElement = container();
    }

    RenderContext.prototype = create(DomContext.prototype);

    RenderContext.prototype.load = function load(callback) {
        callback = ensureCallback(callback);

        if (!Modernizr.csstransforms) {
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
        var count = elements.length;
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
