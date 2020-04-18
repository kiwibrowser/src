define([ 'util/ensureCallback' ], function (ensureCallback) {
    function quickPromise() {
        var thens = [ ];
        var resolved = false;

        function resolve() {
            if (resolved) {
                throw new Error("Already resolved");
            }
            resolved = true;

            while (thens.length) {
                var fn = thens.pop();
                fn();
            }
        }

        function then(fn) {
            fn = ensureCallback(fn);
            if (resolved) {
                fn();
            } else {
                thens.push(fn);
            }
        }

        return {
            resolve: resolve,
            then: then
        };
    }

    return quickPromise;
});
