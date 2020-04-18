define([ 'util/ensureCallback', 'util/bench' ], function (ensureCallback, bench) {
    var texts = [ 'hello world', 'other text' ];
    var CANVAS_WIDTH = 640;
    var CANVAS_HEIGHT = 640;

    var fontFamilies = {
        sans: 'sans-serif',
        serif: 'serif',
        monospace: 'monospace'
    };

    var fontSizes = [ 8, 10, 12, 14, 16, 24 ];

    var stylePreparers = {
        outline: function (context) {
            context.strokeStyle = 'red';
        },

        fill: function (context) {
            context.fillStyle = 'green';
        },

        fillOutline: function (context) {
            context.strokeStyle = 'red';
            context.fillStyle = 'green';
        }
    };

    var styleExecutors = {
        outline: function (context, text) {
            context.strokeText(text, 0, 0);
        },

        fill: function (context, text) {
            context.fillText(text, 0, 0);
        },

        fillOutline: function (context, text) {
            context.fillText(text, 0, 0);
            context.strokeText(text, 0, 0);
        }
    };

    var styles = Object.keys(styleExecutors);

    function runTest(fontFamily, fontSize, style, callback) {
        callback = ensureCallback(callback);

        var canvas = document.createElement('canvas');
        canvas.width = CANVAS_WIDTH;
        canvas.height = CANVAS_HEIGHT;

        var context = canvas.getContext('2d');
        context.font = fontSize + 'pt ' + fontFamily;
        context.textBaseline = 'top';
        context.textAlign = 'left';
        stylePreparers[style](context);

        var execute = styleExecutors[style];
        var score = bench(100, function (i) {
            execute(context, texts[i % texts.length]);

            // FIXME I'd like to be able to verify that the drawing operation
            // actually executed (and didn't defer), but getImageData (below)
            // is too slow to run in a loop like this.
            //context.getImageData(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT)[0];
        });

        callback(null, {
            score: score.toFixed(2)
        });
    }

    // family => size => style => test
    var tests = { };
    Object.keys(fontFamilies).forEach(function (fontFamilyKey) {
        var fontFamily = fontFamilies[fontFamilyKey];

        var subTests = { };
        tests[fontFamilyKey] = subTests;
        fontSizes.forEach(function (fontSize) {
            var subSubTests = { };
            subTests[fontSize] = subSubTests;
            styles.forEach(function (style) {
                subSubTests[style] = function (callback) {
                    runTest(fontFamily, fontSize, style, callback);
                };
            });
        });
    });

    return tests;
});
