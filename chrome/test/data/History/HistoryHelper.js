//
// This script provides some mechanics for testing History
//
function onSuccess(name, id)
{
  setTimeout(onFinished, 0, name, id, "OK");
}

function onFailure(name, id, status)
{
  setTimeout(onFinished, 0, name, id, status);
}

// Finish running a test by setting the status 
// and the cookie.
function onFinished(name, id, result)
{
  var statusPanel = document.getElementById("statusPanel");
  if (statusPanel) {
    statusPanel.innerHTML = result;
  }

  if (result == "OK")
    document.title = "OK";
  else
    document.title = "FAIL";
}

function readCookie(name) {
  var cookie_name = name + "=";
  var ca = document.cookie.split(';');

  for(var i = 0 ; i < ca.length ; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') {
      c = c.substring(1,c.length);
    }
    if (c.indexOf(cookie_name) == 0) {
      return c.substring(cookie_name.length, c.length);
    }
  }
  return null;
}

function createCookie(name,value,days) {
  var expires = "";
  if (days) {
    var date = new Date();
    date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
    expires = "; expires=" + date.toGMTString();
  }
  document.cookie = name+"="+value+expires+"; path=/";
}

function eraseCookie(name) {
  createCookie(name, "", -1);
}

var navigate_backward_cookie = "Navigate_Backward_Cookie";
var navigate_forward_cookie = "Navigate_Forward_Cookie";
