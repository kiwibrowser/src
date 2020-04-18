(function () {
    var names = [
        'image',
        //'spriteSheet'
    ];

    var filenames = [ ];
    names.forEach(function (name) {
        filenames.push('sprites/sources/' + name);
    });

    define(filenames, function (/* ... */) {
        var sources = { };
        var values = arguments;
        names.forEach(function (name, i) {
            sources[name] = values[i];
        });

        return sources;
    });
}());
