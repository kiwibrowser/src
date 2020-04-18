function getStyle(elem, name) {
  if (document.defaultView && document.defaultView.getComputedStyle) {
    name = name.replace(/([A-Z])/g, '-$1');
    name = name.toLowerCase();

    try {
      var s = document.defaultView.getComputedStyle(elem, '');
      return s && s.getPropertyValue(name);
    } catch (ex) {
      return null;
    }
  } else {
    return null;
  }
}

var bElement = document.getElementById('test1');
var display = getStyle(bElement, 'display').toLowerCase();
document.title = display;
