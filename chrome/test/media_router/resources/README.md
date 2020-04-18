The document shows how to create a test data file which is used for the
integration test to inject test data before running test.
----------------------------------------------------------------------------
The test data is a JSON string and here is full example with description:

{
  // Define the return value for getAvailableSinks API defined in
  // TestProvider.js.
  // The value is a map from source urn to a list of sinks as following. Default
  // value is for test source urn only and is:
  // [{"id": "id1", "friendlyName": "test-sink-1"},
  //  {"id": "id2", "friendlyName": "test-sink-2"}]
  "getAvailableSinks": {
    "http://www.google.com/": [
      {"id": "id1", "friendlyName": "test-device-1"},
      {"id": "id2", "friendlyName": "test-device-2"}
    ]
  },

  // Define the return value for canRoute API, the return value should be
  // either 'true' or 'false'. The default value is 'true'.
  "canRoute": "true",

  // Define the return value for createRoute API. Since the return type of
  // createRoute is a Promise, here we just need to define
  // if return successful result or error.
  // The value for 'passed' should be either 'true' or 'false'.
  // If it is 'false', you also need to give the corresponding
  // error message. If it is 'true', the error message will be ignored.
  // TODO(leilei): Change keyword 'passed' to 'success'.
  "createRoute": {"passed": "false", "errorMessage": "Unknown sink"}
}
