define([ ], function () {
    var cacheBust = {
        buster: function buster() {
            return String(Math.random()).replace(/[^0-9]/g, '');
        },

        url: function cacheBustUrl(url) {
            if (/\?/.test(url)) {
                return url + '&' + cacheBust.buster();
            } else {
                return url + '?' + cacheBust.buster();
            }
        }
    };

    return cacheBust;
});
