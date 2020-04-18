
function registerTestingCustomElement(tagName) {
    var definition = function() {};
    definition.prototype = Object.create(HTMLElement.prototype);
    definition.prototype.createdCallback = function() {
        if (typeof this.constructor.ids === "undefined")
            this.constructor.ids = [];
        this.constructor.ids.push(this.id);
     }

    var ctor = document.registerElement(tagName, definition);
    return ctor;
}

function ImportTestLatch(test, count) {
    this.loaded = function() {
        count--;
        if (!count)
            test();
    };
}
