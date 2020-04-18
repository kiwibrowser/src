function buttonClickListener(e) {
  var localVar = e + 'a';
  debugger;
}


var button = document.getElementById('btn');
if (button) {
  button.addEventListener('click', buttonClickListener, false);
}
