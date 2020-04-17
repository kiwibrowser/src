'use strict';

function createFrame(url, name) {
  let frame = document.createElement('iframe');
  frame.name = name;
  frame.src = url;
  document.body.appendChild(frame);
}

function createDocWrittenFrame(name, base_url) {
  let iframe_src = `<!DOCTYPE HTML>
  <script src="${base_url}create_frame.js"></script>
  <script src="${base_url}ad_script.js"></script>
`

  let frame = document.createElement('iframe');
  frame.name = name;
  document.body.appendChild(frame);

  frame.contentDocument.open();
  frame.contentDocument.write(iframe_src);
  frame.contentDocument.close();
}
