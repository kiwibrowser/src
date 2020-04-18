(class NodeTracker {
  constructor(dp) {
    this._nodes = new Map();
    dp.DOM.onSetChildNodes(message => message.params.nodes.forEach(node => this._addNode(node)));
  }

  addDocumentNode(documentNode) {
    this._addNode(documentNode);
  }

  _addNode(node) {
    this._nodes.set(node.nodeId, node);
    if (node.children)
      node.children.forEach(node => this._addNode(node));
  }

  nodeForId(nodeId) {
    return this._nodes.get(nodeId) || null;
  }

  nodes() {
    return Array.from(this._nodes.values());
  }

  nodeIds() {
    return Array.from(this._nodes.keys());
  }
})
