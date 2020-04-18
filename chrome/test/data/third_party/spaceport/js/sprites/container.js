define([ ], function () {
    function container() {
        var el = document.createElement('div');
        el.style.overflow = 'hidden';
        el.style.position = 'absolute';
        el.style.left = '0';
        el.style.top = '0';
        el.style.width = '512px';
        el.style.height = '512px';
        el.style.background = '#FFFFFF';
        return el;
    }

    return container;
});
