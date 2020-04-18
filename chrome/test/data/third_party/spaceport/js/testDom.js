define([ 'util/report' ], function (report) {
    function classes(el) {
        return el.className.split(/\s+/g);
    }

    function addClass(el, className) {
        el.className += ' ' + className;
    }

    function removeClass(el, className) {
        // wtb classList (damnit Safari!)
        el.className = classes(el).filter(function (c) {
            return c !== className;
        }).join(' ');
    }

    function testResultNameAccept(name) {
        return name !== 'pass';
    }

    var testDom = {
        endTest: function (domId, err, results) {
            var el = document.getElementById(domId);
            if (!el) {
                throw new Error('Could not find element #' + domId);
            }

            function findSlot(name) {
                var elements = el.querySelectorAll('[data-property]');
                var i;
                for (i = 0; i < elements.length; ++i) {
                    var element = elements[i];
                    if (element.getAttribute('data-property') === name) {
                        return element;
                    }
                }

                return null;
            }

            function fillSlots(name, value) {
                if (!testResultNameAccept(name)) {
                    return;
                }

                if (value === null || typeof value === 'undefined') {
                    return;
                }

                if (typeof value === 'object') {
                    Object.keys(value).forEach(function (subName) {
                        fillSlots(subName, value[subName]);
                    });
                } else {
                    var slot = findSlot(name);
                    if (slot) {
                        slot.textContent = value;
                    } else {
                        //console.warn('Could not find slot ' + name + ' for ' + domId, value);
                    }
                }
            }

            fillSlots('', results);

            removeClass(el, 'pass');
            removeClass(el, 'fail');
            removeClass(el, 'error');

            if (err) {
                addClass(el, 'error');

                var errorMessageEl = el.querySelector('.error-message');
                if (errorMessageEl) {
                    errorMessageEl.textContent = err;
                }
            } else if (results && results.passed) {
                addClass(el, 'pass');
            } else {
                addClass(el, 'fail');
            }

        }
    };

    return testDom;
});
