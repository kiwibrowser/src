self.onmessage = (event) => {
  fetch(new Request(event.data.url, event.data.init)).then(
      (response) => {
        return response.text().then((text) => self.postMessage(text));
      },
      () => self.postMessage('FETCH_FAILED'));
};
