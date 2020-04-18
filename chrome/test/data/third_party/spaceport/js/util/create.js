define([ ], function () {
    // Simplified Object.create shim for easy prototypal inheritence.
    if (Object.create) {
        return function create(proto) {
            return Object.create(proto);
        };
    } else {
        return function create(proto) {
            function TempClass() { }
            TempClass.prototype = proto;
            return new TempClass();
        };
    }
});
