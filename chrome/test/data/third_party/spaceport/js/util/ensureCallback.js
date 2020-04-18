define([ ], function () {
    return function ensureCallback(callback) {
        if (typeof callback !== 'function') {
            return function (err) {
                if (err) {
                    console.error(err);
                }
            };
        }

        var called = false;
        return function () {
            if (called) {
                // Disallow calling multiple times
                return;
            }

            called = true;
            var args = arguments;

            setTimeout(function () {
                // Ensure async
                callback.apply(null, args);
            }, 0);
        };
    };
});
