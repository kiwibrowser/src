The *-default-buffers.html tests in this directory are run with the default MSE
buffer sizes (150MB for video, 12MB for audio).
The *-1mb-buffers.html tests are run in a separate virtual test suite with
--mse-audio-buffer-size-limit=1048576 and --mse-video-buffer-size-limit=1048576
command-line switches and thus with 1MB MSE buffer sizes for both audio and
video. See LayoutTests/TestExpectations and LayoutTests/VirtualTestSuites.
