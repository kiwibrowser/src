define([ 'util/ensureCallback', 'util/chainAsync' ], function (ensureCallback, chainAsync) {
    var testRunner = {
        run: function run(name, test, callbacks) {
            var stepCallback = callbacks.step || function () { };
            var doneCallback = ensureCallback(callbacks.done);

            if (typeof test === 'function') {
                // We run the test twice (sadly): once to warm up the JIT and once for the actual test.
                //test(function (_, _) {
                    test(function (err, results) {
                        stepCallback(err, name, results);
                        doneCallback(err, results);
                    });
                //});
            } else {
                var allResults = { };

                var subTests = Object.keys(test).map(function (subName) {
                    var newName = name ? name + '.' + subName : subName;
                    return function (next) {
                        testRunner.run(newName, test[subName], {
                            step: stepCallback,
                            done: function (err, results) {
                                if (!err) {
                                    allResults[subName] = results;
                                }

                                next();
                            }
                        });
                    };
                });

                chainAsync(subTests.concat([
                    function () {
                        doneCallback(null, allResults);
                    }
                ]));
            }
        }
    };

    return testRunner;
});
