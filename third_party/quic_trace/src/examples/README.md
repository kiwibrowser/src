This directory contains example QUIC traces.  Most of those are generated from
the unit tests of gQUIC's BBR implementation.  You can regenerate those by
building `net_unittests` in Chrome and setting `QUIC_TEST_OUTPUT_DIR`
environment variable to the directory where you want those to go.  The
particular file shipped here, `example.qtr`, originally was
`SimpleTransferSmallBuffer.BbrSenderTest.C42.20180910211327.qtr`.
`example.json.gz` is a compressed version of JSON representation of
`example.qtr`.
