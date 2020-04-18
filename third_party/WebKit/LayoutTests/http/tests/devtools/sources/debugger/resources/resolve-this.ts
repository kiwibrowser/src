class Foo {
    constructor() {
    }

    bar() {
        let test = () => {
            console.log(this);
            debugger;
        };
        test();
    }
}

function testFunction() {
    new Foo().bar();
}
