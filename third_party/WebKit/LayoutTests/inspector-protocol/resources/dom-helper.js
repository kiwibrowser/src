(class DOMHelper {
  static attributes(node) {
    var attr = new Map();
    if (!node.attributes)
      return attr;
    for (var i = 0; i < node.attributes.length; i += 2)
      attr.set(node.attributes[i], node.attributes[i + 1]);
    return attr;
  }
})
