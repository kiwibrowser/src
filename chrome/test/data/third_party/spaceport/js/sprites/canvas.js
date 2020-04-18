define([ ], function () {
    function canvas() {
        var canvas = document.createElement('canvas');
        canvas.width = 512;
        canvas.height = 512;
        canvas.style.background = '#FFFFFF';
        canvas.style.position = 'absolute';
        canvas.style.left = '0';
        canvas.style.top = '0';
        return canvas;
    }

    return canvas;
});
