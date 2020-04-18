define([ ], function () {
    // CSS transform feature detection based off of
    // http://andrew-hoyer.com/experiments/rain/
    // Public domain

    var style = document.createElement('div').style;

    function getFirstIn(object, propertyNames) {
        return propertyNames.filter(function(name) {
            return name in object;
        }).shift();
    }

    var transformOriginStyleProperty = getFirstIn(style, [
        'transformOrigin',
        'WebkitTransformOrigin',
        'MozTransformOrigin',
        'msTransformOrigin',
        'OTransformOrigin'
    ]);

    var transformStyleProperty = getFirstIn(style, [
        'transform',
        'WebkitTransform',
        'MozTransform',
        'msTransform',
        'OTransform'
    ]);

    var CSSMatrix = window[getFirstIn(window, [
        'CSSMatrix',
        'WebKitCSSMatrix',
        'WebkitCSSMatrix'
    ])];

    // Firefox has a bug where it requires 'px' for translate matrix
    // elements (where it should accept plain numbers).
    var matrixTranslateSuffix = transformStyleProperty === 'MozTransform' ? 'px' : '';

    return {
        transformOriginStyleProperty: transformOriginStyleProperty,
        transformStyleProperty: transformStyleProperty,
        matrixTranslateSuffix: matrixTranslateSuffix,
        CSSMatrix: CSSMatrix
    }
});
