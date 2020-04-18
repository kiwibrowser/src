function doFoo() {
    console.log('foo');
}
window.foo = console.log.bind(console, 'foo');
