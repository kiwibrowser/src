define([ 'util/ensureCallback' ], function (ensureCallback) {
    var requestAnimationFrame
         = window.requestAnimationFrame
        || window.webkitRequestAnimationFrame
        || window.mozRequestAnimationFrame
        || window.oRequestAnimationFrame
        || window.msRequestAnimationFrame;

    // Benchmarks fn until maxTime ms has passed.  Returns an object:
    //
    // {
    //   "score" -- approximate number of operations performed in that time ('score')
    // }
    function benchAsync(maxTime, fn, callback) {
        if (typeof fn !== 'function') {
            throw new TypeError('Argument must be a function');
        }

        var startTime;
        var running = true;
        
        function checkDone(endTime) {
            if (!running) {
                return;
            }

            if (endTime - startTime >= maxTime) {
                running = false;

                var elapsed = endTime - startTime;
                var timeoutScore = timeoutTimes.length / elapsed * maxTime;
                var rafScore = rafTimes.length / elapsed * maxTime;
                return callback(null, {
                    startTime: startTime,
                    timeoutScore: timeoutScore,
                    rafScore: rafScore,
                    score: requestAnimationFrame ? rafScore : timeoutScore,
                    elapsed: elapsed
                });
            }
        }

        function rafUpdate() {
            var now = Date.now();
            rafTimes.push(now);
            checkDone(now);
            if (running) {
                requestAnimationFrame(rafUpdate);
            }
        }
        
        var timeoutTimes = [ ];
        var rafTimes = [ ];
        
        function next() {
            fn(timeoutTimes.length, function () {
                var now = Date.now();
                timeoutTimes.push(now);
                checkDone(now);
                if (running) {
                    next();
                }
            });
        }

        setTimeout(function () {
            startTime = Date.now();

            if (requestAnimationFrame) {
                requestAnimationFrame(rafUpdate);
            }

            next();
        }, 0);
    }

    return benchAsync;
});
