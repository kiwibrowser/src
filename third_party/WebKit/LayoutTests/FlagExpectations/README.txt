FlagExpectations stores flag-specific test expectations.  To run layout tests
with a flag, use:

  run_web_tests.py --additional-driver-flag=--name-of-flag

In addition to passing --name-of-flag to the binary, run_web_tests.py will look
for test expectations in

  FlagExpectations/name-of-flag

which will override the main TestExpectations file.
