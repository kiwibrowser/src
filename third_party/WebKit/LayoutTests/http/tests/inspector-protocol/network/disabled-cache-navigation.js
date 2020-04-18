(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that browser-initiated navigation honors Network.setCacheDisabled`);

  async function navigateAndGetResponse() {
    dp.Page.navigate({url: testRunner.url('resources/cached.php')});
    var response = (await dp.Network.onceResponseReceived()).params;
    await dp.Network.onceLoadingFinished();
    var content = (await dp.Network.getResponseBody({requestId: response.requestId})).result.body;
    if (typeof content != 'string' || !content.startsWith('<html>'))
      testRunner.fail(`Invalid response: ${content}`);
    return {status: response.response.status, content: content};
  }
  await dp.Page.enable();
  await dp.Network.enable();
  let response = await navigateAndGetResponse();
  testRunner.log(`Original navigation, should not be cached: ${response.status}`);

  await dp.Page.navigate({url: 'about:blank'});
  let response2 = await navigateAndGetResponse();
  testRunner.log(`Second navigation, should be cached: ${response2.status} cached: ${response.content === response2.content}`);

  await dp.Network.setCacheDisabled({cacheDisabled: true});
  await dp.Page.navigate({url: 'about:blank'});
  let response3 = await navigateAndGetResponse();

  testRunner.log(`Navigation with cache disabled: ${response3.status} cached: ${response3.content === response2.content}`);
  testRunner.completeTest();
})
