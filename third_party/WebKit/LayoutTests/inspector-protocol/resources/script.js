(function () {
  var xhr = new XMLHttpRequest();
  xhr.responseType = "blob";
  xhr.open("GET", "../../media/content/greenbox.png", true);
  xhr.send();
})();
