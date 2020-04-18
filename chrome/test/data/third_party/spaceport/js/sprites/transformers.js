(function () {
    var names = [
        'scale',
        'translate',
        'rotate'
    ];

    var filenames = [ ];
    names.forEach(function (name) {
        filenames.push('sprites/transformers/' + name);
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
