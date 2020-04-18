<!DOCTYPE html>
<script>
  function getTheListener() {
    return postMessage.bind(this, "respond", "*", undefined);
  }
  document.domain = "web-platform.test";
  onmessage = function (e) {
    if (e.data == "sendclick") {
      document.body.click();
    } else {
      parent.postMessage(
        {
          actual: e.origin,
          expected: location.protocol + "//www1.web-platform.test:8080",
          reason: "Incumbent should have been the caller of addEventListener()"
        },
        "*")
    };
  }
</script>
