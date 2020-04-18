function dumpMatrix(matrix) {
    return "[" + matrix.a.toFixed(1)
         + " " + matrix.b.toFixed(1)
         + " " + matrix.c.toFixed(1)
         + " " + matrix.d.toFixed(1)
         + " " + matrix.e.toFixed(1)
         + " " + matrix.f.toFixed(1)
         + "]";
}

SVGTransform.prototype.toString = function()
{
    var transformTypes = {
        "0": "SVG_TRANSFORM_UNKNOWN",
        "1": "SVG_TRANSFORM_MATRIX",
        "2": "SVG_TRANSFORM_TRANSLATE",
        "3": "SVG_TRANSFORM_SCALE",
        "4": "SVG_TRANSFORM_ROTATE",
        "5": "SVG_TRANSFORM_SKEWX",
        "6": "SVG_TRANSFORM_SKEWY"
    };

    return "type=" + transformTypes[this.type] + " matrix=" + dumpMatrix(this.matrix);
}