function addElements() {
    var button = document.createElement('button');
    button.id = 'test';
    button.addEventListener('click', handleClick, true);
    document.body.appendChild(button);
    var bar = document.createElement('div');
    bar.id = 'barrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr';
    document.body.appendChild(bar);
}
function handleClick(event) {
    var handler = new ClickHandler();
    handler.handle(event);
}
function ClickHandler() {
}
ClickHandler.prototype.handle = function (event) {
    console.log('button clicked!');
};
//# sourceMappingURL=source-map-2.json