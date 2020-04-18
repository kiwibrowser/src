function setInnerFrame() {
  var inner_frame = window.dialogArguments;
  if (inner_frame) {
    document.getElementById('ifr').src = inner_frame;
  }
}

function checkAccept(f) {
  if (f.accept.checked) {
    window.returnValue = 6;
  } else {
    window.returnValue = 1;
  }
  window.close();
}

function resize() {
  var ifr = document.getElementById('ifr');
  var footer = document.getElementById('footer');
  
  ifr.height = footer.offsetTop - ifr.offsetTop;
  setInnerFrame();
}

window.onresize = resize;
window.onload = resize;