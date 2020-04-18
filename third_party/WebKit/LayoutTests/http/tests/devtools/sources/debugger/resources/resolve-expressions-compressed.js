function foo(o,p){var r=new ClassA;r.prop1=o;this.prop2=p;r["prop3"]="property";debugger;return r.prop1+this.prop2}function testFunction(){foo.call({},"param1","param2")}function ClassA(){}
//# sourceMappingURL=resolve-expressions.js.map
