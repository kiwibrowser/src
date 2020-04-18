The certificate message files (\*.msg) and the signed exchange files
(\*.htxg) in this directory are generated using the following commands.

gen-certurl and gen-signedexchange are available in [webpackage repository][1].
We're using a fork of [this repository][2] that implements the "Implementation
Checkpoints" of [the signed-exchange spec][3].

[1]: https://github.com/WICG/webpackage
[2]: https://github.com/nyaxt/webpackage
[3]: https://wicg.github.io/webpackage/draft-yasskin-httpbis-origin-signed-exchanges-impl.html

```
# Install gen-certurl command
go get github.com/nyaxt/webpackage/go/signedexchange/cmd/gen-certurl

# Install gen-signedexchange command
go get github.com/nyaxt/webpackage/go/signedexchange/cmd/gen-signedexchange

# Generate the certificate message file of "127.0.0.1.pem".
gen-certurl  \
  ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.pem \
  > 127.0.0.1.pem.msg

# Generate the signed exchange file.
gen-signedexchange \
  -uri https://www.127.0.0.1/test.html \
  -status 200 \
  -content htxg-location.html \
  -certificate ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.pem \
  -certUrl http://localhost:8000/loading/htxg/resources/127.0.0.1.pem.msg \
  -validityUrl http://localhost:8000/loading/htxg/resources/resource.validity.msg \
  -privateKey ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.key \
  -date 2018-04-01T00:00:00Z \
  -expire 168h \
  -o htxg-location.htxg \
  -miRecordSize 100

# Generate the signed exchange file which certificate file is not available.
gen-signedexchange \
  -uri https://www.127.0.0.1/not_found_cert.html \
  -status 200 \
  -content htxg-location.html \
  -certificate ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.pem \
  -certUrl http://localhost:8000/loading/htxg/resources/not_found_cert.pem.msg \
  -validityUrl http://localhost:8000/loading/htxg/resources/not_found_cert.validity.msg \
  -privateKey ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.key \
  -date 2018-04-01T00:00:00Z \
  -expire 168h \
  -o htxg-cert-not-found.htxg \
  -miRecordSize 100

# Install gen-certurl command from the original WICG repository [1].
# (Note: this overwrites gen-certurl fetched from [2] in the above.)
go get github.com/WICG/webpackage/go/signedexchange/cmd/gen-certurl

# Generate the certificate chain CBOR of "127.0.0.1.pem".
gen-certurl  \
  -pem ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.pem \
  > 127.0.0.1.pem.cbor

# Generate the b1 version of signed exchange file.
gen-signedexchange \
  -uri https://www.127.0.0.1/test.html \
  -status 200 \
  -content htxg-location.html \
  -certificate ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.pem \
  -certUrl http://localhost:8000/loading/htxg/resources/127.0.0.1.pem.cbor \
  -validityUrl http://localhost:8000/loading/htxg/resources/resource.validity.msg \
  -privateKey ../../../../../../../blink/tools/blinkpy/third_party/wpt/certs/127.0.0.1.key \
  -date 2018-05-15T00:00:00Z \
  -expire 168h \
  -o htxg-location.sxg \
  -miRecordSize 100
```
