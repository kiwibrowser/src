// Normalize a cookie string
function normalizeCookie(cookie) {
  // Split the cookie string, sort it and then put it back together.
  return cookie.split('; ').sort().join('; ');
}

function clear(server) {
  return new Promise(resolve => {
    const ws = new WebSocket(server + '/set-cookie?clear=1');
    ws.onopen = () => {
      ws.close();
    };
    ws.onclose = resolve;
  });
}

function setCookie(server) {
  return new Promise(resolve => {
    const ws = new WebSocket(server + '/set-cookie');
    ws.onopen = () => {
      ws.close();
    };
    ws.onclose = resolve;
  });
}
