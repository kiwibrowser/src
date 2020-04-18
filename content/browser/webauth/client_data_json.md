# Advice to sites regarding `clientDataJSON`

When performing a registration or authentication with webauthn, it's critical that sites confirm that the returned [`clientDataJSON`](https://w3c.github.io/webauthn/#dom-authenticatorresponse-clientdatajson) contains the [challenge](https://w3c.github.io/webauthn/#cryptographic-challenges) originally provided. Otherwise old messages can be replayed. Likewise, sites must check the `origin` member to confirm that the action is not being proxied by another site.

In order to implement this, sites should parse the JSON as a [`CollectedClientData`](https://w3c.github.io/webauthn/#dictdef-collectedclientdata) structure and confirm that the `type`, `challenge`, and `origin` members (at least) are as expected.

Sites should _not_ implement this by comparing the unparsed value of `clientDataJSON` against a template with the `challenge` value filled in. This would fail when new members are added to `CollectedClientData` in the future as the template would no longer be correct.

In order to guide sites away from doing this, Chromium will sometimes, randomly insert an extra member into `clientDataJSON` which references this documentation.
