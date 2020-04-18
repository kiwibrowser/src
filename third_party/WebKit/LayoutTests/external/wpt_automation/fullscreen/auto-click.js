(function() {

// Gets the offset of an element in coordinates relative to the top-level frame.
function offset(element) {
  const rect = element.getBoundingClientRect();
  const iframe = element.ownerDocument.defaultView.frameElement;
  if (!iframe) {
    return { left: rect.left, top: rect.top };
  }
  const outerOffset = offset(iframe);
  const style = getComputedStyle(iframe);
  const get = prop => +/[\d.]*/.exec(style[prop])[0];
  return {
    left: outerOffset.left + get('borderLeftWidth') + get('paddingLeft') + rect.left,
    top: outerOffset.top + get('borderTopWidth') + get('paddingTop') + rect.top
  };
}

function click(button) {
  const pos = offset(button);
  const rect = button.getBoundingClientRect();
  eventSender.mouseMoveTo(Math.ceil(pos.left + rect.width / 2), Math.ceil(pos.top + rect.height / 2));
  eventSender.mouseDown();
  eventSender.mouseUp();
}

const observer = new MutationObserver(mutations => {
  for (const mutation of mutations) {
    for (const node of mutation.addedNodes) {
      if (node.localName == 'button')
        click(node);
      else if (node.localName == 'iframe')
        if (node.contentDocument)
          observe(node.contentDocument);
    }
  }
});

function observe(target) {
  observer.observe(target, { childList: true, subtree: true });
}

// Handle what's already in the document.
for (const button of document.getElementsByTagName('button')) {
  click(button);
}
for (const iframe of document.getElementsByTagName('iframe')) {
  if (iframe.contentDocument)
    observe(iframe.contentDocument);
  iframe.addEventListener('load', () => {
    if (iframe.contentDocument)
      observe(iframe.contentDocument);
  });
}

// Observe future changes.
observe(document);

})();
