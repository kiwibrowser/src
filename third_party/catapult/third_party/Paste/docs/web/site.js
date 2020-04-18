function setup_dropdowns() {
  var els = document.getElementsByTagName('UL');
  for (var i = 0; i < els.length; i++) {
    var el = els[i];
    if (el.className.search(/\bcontents\b/) > -1) {
      enable_dropdown(el);
    }
  }
}

function enable_dropdown(el) {
  var title = el.getElementsByTagName('LI')[0];
  var plus_minus = document.createTextNode('  [-]');
  if (title.childNodes[0].tagName != 'A') {
    anchor = document.createElement('A');
    while (title.childNodes.length) {
      anchor.appendChild(title.childNodes[0]);
    }
    anchor.setAttribute('href', '#');
    anchor.style.padding = '1px';
    title.appendChild(anchor);
  } else {
    anchor = title.childNodes[0];
  }
  anchor.appendChild(plus_minus);
  function show_hide() {
    if (el.sub_hidden) {
      set_sub_li(el, '');
      anchor.removeChild(plus_minus);
      plus_minus = document.createTextNode('  [-]');
      anchor.appendChild(plus_minus);
    } else {
      set_sub_li(el, 'none');
      anchor.removeChild(plus_minus);
      plus_minus = document.createTextNode('  [+]');
      anchor.appendChild(plus_minus);
    }
    el.sub_hidden = ! el.sub_hidden;
    return false;
  }
  anchor.onclick = show_hide;
  show_hide();
}

function set_sub_li(list, display) {
  var sub = list.getElementsByTagName('LI');
  for (var i = 1; i < sub.length; i++) {
    sub[i].style.display = display;
  }
}

function add_onload(func) {
  if (window.onload) {
    var old_onload = window.onload;
    function new_onload() {
      old_onload();
      func();
    }
    window.onload = new_onload;
  } else {
    window.onload = func;
  }
}

add_onload(setup_dropdowns);

      


