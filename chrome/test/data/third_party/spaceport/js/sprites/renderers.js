(function () {
    var names = [
        'css2dImg',
        'css3dImg',
        'css2dBackground',
        'css3dBackground',
        //'cssMatrixImg',

        'canvasDrawImageFullClear',
        'canvasDrawImageFullClearAlign',
        'canvasDrawImagePartialClear',
        'canvasDrawImagePartialClearAlign',

        'webGLDrawWithUniform',
        'webGLBatchDraw'
    ];

    var filenames = [ ];
    names.forEach(function (name) {
        filenames.push('sprites/renderers/' + name);
    });

    define(filenames, function (/* ... */) {
        var renderers = { };
        var values = arguments;
        names.forEach(function (name, i) {
            renderers[name] = values[i];
        });

        return renderers;
    });
}());
