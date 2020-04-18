var ws = new WebSocket('ws://localhost:8880/workers/resources/stall');
// FIXME: Find a way to guarantee we've reached a stable stalled state before
// posting the message.
postMessage("stalled");
