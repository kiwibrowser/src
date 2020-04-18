(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that willSendRequest contains the correct mixed content status for not-mixed content.`);

  function addIframeWithMixedContent() {
    var iframe = document.createElement('iframe');
    iframe.src = 'https://127.0.0.1:8443/inspector-protocol/resources/no-mixed-content-iframe.html';
    document.body.appendChild(iframe);
  }

  var helper = await testRunner.loadScript('./resources/mixed-content-type-test.js');
  helper(testRunner, session, addIframeWithMixedContent);
})
