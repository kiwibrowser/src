define([ 'util/ensureCallback' ], function (ensureCallback) {
    return function timeout(duration, fn, callback) {
        callback = ensureCallback(callback);

        var id = setTimeout(function () {
            callback(new Error('Operation timed out'));
        }, duration);

        fn(function () {
            clearTimeout(id);
            callback.apply(this, arguments);
        });
    };
});
