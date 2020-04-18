define([ ], function () {
    // Benchmarks fn until maxTime ms has passed.  Returns approximate number
    // of operations performed in that time ('score').
    function bench(maxTime, fn) {
        if (typeof fn !== 'function') {
            throw new TypeError('Argument must be a function');
        }

        var operationCount = 0;
        var startTime = Date.now();
        var endTime;
        while (true) {
            fn(operationCount);
            ++operationCount;

            endTime = Date.now();
            if (endTime - startTime >= maxTime) {
                break;
            }
        }

        return operationCount / (endTime - startTime) * maxTime;
    }

    return bench;
});
