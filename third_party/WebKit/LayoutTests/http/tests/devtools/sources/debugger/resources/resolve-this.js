var Foo = (function () {
    function Foo() {
    }
    Foo.prototype.bar = function () {
        var _this = this;
        var test = function () {
            console.log(_this);
            debugger;
        };
        test();
    };
    return Foo;
}());
function testFunction() {
    new Foo().bar();
}
//# sourceMappingURL=resolve-this.js.map