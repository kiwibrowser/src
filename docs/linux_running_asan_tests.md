# Running Chrome tests with AddressSanitizer (asan) and LeakSanitizer (lsan)

It is relatively straightforward to run asan/lsan tests. The only tricky part
is that some environment variables need to be set.

Changes to args.gn (ie, `out/Release/args.gn`):

```python
is_asan = true
is_lsan = true
```

How to run the test:

```sh
$ export ASAN_OPTIONS="symbolize=1 external_symbolizer_path=./third_party/llvm-build/Release+Asserts/bin/llvm-symbolizer detect_leaks=1 detect_odr_violation=0"
$ export LSAN_OPTIONS=""
$ out/Release/browser_tests
```
