window.addEventListener("message", function (event) {
  if (event.data.command == "send-fetch-referrer") {
    fetch(event.data.url).then(function (response) {
      response.text().then(function (responseText) {
        window.top.postMessage({
          test: "send-fetch-referrer",
          referrer: responseText
        }, "*");
      });
    });
  }
});

window.top.postMessage({
  test: '<?php echo $_GET["test"] ?>',
  referrer: '<?php echo $_SERVER["HTTP_REFERER"] ?>'
}, "*");