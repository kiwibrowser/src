# WebSocket API

This directory contains:
- the implementation of
  [the WebSocket API](https://html.spec.whatwg.org/multipage/web-sockets.html).
- the Pepper implementation of the WebSocket API

They use WebSocketChannelImpl to connect to the WebSocket service i.e. the
blink.mojom.WebSocket implementation in content/browser/websockets/.
