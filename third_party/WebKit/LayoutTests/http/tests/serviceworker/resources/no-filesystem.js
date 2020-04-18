function expect_exception(func) {
  try {
    func();
  } catch (ex) {
    return;
  }
  throw Error("No exception seen");
}

expect_exception(_ => webkitRequestFileSystem(TEMPORARY, 1024));
expect_exception(_ => webkitRequestFileSystemSync(TEMPORARY, 1024));
