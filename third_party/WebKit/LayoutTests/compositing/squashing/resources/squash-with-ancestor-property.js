function addDivElement(id, parentId) {
  var element = document.createElement('div');
  element.setAttribute('id', id);
  if (parentId === 'body')
    document.body.appendChild(element);
  else
    document.getElementById(parentId).appendChild(element);
}

function generateDom() {
  addDivElement('container', 'body');
  addDivElement('composited-background', 'container');
  document.getElementById('composited-background').innerText = 'Text behind the orange box.';
  addDivElement('ancestor', 'container');
  addDivElement('composited-overlap-child', 'ancestor');
}
