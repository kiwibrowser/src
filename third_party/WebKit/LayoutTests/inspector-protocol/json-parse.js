(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that backend correctly handles unicode in messages');

  var requestId = 100500;
  var command = {
    'method': 'Runtime.evaluate',
    'params': {expression: '\'!!!\''},
    'id': requestId
  };
  var message = JSON.stringify(command).replace('!!!', '\\u041F\\u0440\\u0438\\u0432\\u0435\\u0442 \\u043C\\u0438\\u0440');
  testRunner.log(await session.sendRawCommand(requestId, message));
  testRunner.completeTest();
})
