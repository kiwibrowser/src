importScripts('../../resources/testharness.js');
importScripts('file:///gen/layout_test_data/mojo/public/js/mojo_bindings.js');
importScripts('file:///gen/content/test/data/mojo_layouttest_test.mojom.js');
importScripts('helpers.js');

promise_test(async () => {
  let helperImpl = new TestHelperImpl;
  let interceptor =
      new MojoInterfaceInterceptor(content.mojom.MojoLayoutTestHelper.name);
  interceptor.oninterfacerequest = e => {
    helperImpl.bindRequest(e.handle);
  };
  interceptor.start();

  let helper = new content.mojom.MojoLayoutTestHelperPtr;
  Mojo.bindInterface(content.mojom.MojoLayoutTestHelper.name,
                     mojo.makeRequest(helper).handle);

  let response = await helper.reverse('the string');
  assert_equals(response.reversed, kTestReply);
  assert_equals(helperImpl.getLastString(), 'the string');
}, 'Can implement a Mojo service and intercept it from a worker');

test(t => {
  assert_throws(
      'NotSupportedError',
      () => {
        new MojoInterfaceInterceptor(content.mojom.MojoLayoutTestHelper.name,
                                     "process");
      });
}, 'Cannot create a MojoInterfaceInterceptor with process scope');

// done() is needed because the testharness is running as if explicit_done
// was specified.
done();
