'use strict';

function createAdFrame(url, name) {
  let frame = document.createElement('iframe');
  frame.name = name;
  frame.src = url;
  document.body.appendChild(frame);
}

async function createDocWrittenAdFrame(name, base_url) {
  let doc_body = await fetch('iframe.html');
  let doc_text = await doc_body.text();

  let frame = document.createElement('iframe');
  frame.name = name;
  document.body.appendChild(frame);

  frame.contentDocument.open();
  frame.contentDocument.write(doc_text);
  frame.contentDocument.close();
}
