import base64
import hashlib
import re
import logging
import json
import sys
import six

from binascii import a2b_base64

from Crypto.PublicKey import RSA
from Crypto.PublicKey.RSA import importKey
from Crypto.PublicKey.RSA import _RSAobj
from Crypto.Util.asn1 import DerSequence

from requests import request

from jwkest import base64url_to_long
from jwkest import as_bytes
from jwkest import base64_to_long
from jwkest import long_to_base64
from jwkest import JWKESTException
from jwkest import b64d
from jwkest import b64e
from jwkest.ecc import NISTEllipticCurve
from jwkest.jwt import b2s_conv

if sys.version > '3':
    long = int
else:
    from __builtin__ import long

__author__ = 'rohe0002'

logger = logging.getLogger(__name__)

PREFIX = "-----BEGIN CERTIFICATE-----"
POSTFIX = "-----END CERTIFICATE-----"


class JWKException(JWKESTException):
    pass


class FormatError(JWKException):
    pass


class SerializationNotPossible(JWKException):
    pass


class DeSerializationNotPossible(JWKException):
    pass


class HeaderError(JWKESTException):
    pass


def dicthash(d):
    return hash(repr(sorted(d.items())))


def intarr2str(arr):
    return "".join([chr(c) for c in arr])


def sha256_digest(msg):
    return hashlib.sha256(as_bytes(msg)).digest()


def sha384_digest(msg):
    return hashlib.sha384(as_bytes(msg)).digest()


def sha512_digest(msg):
    return hashlib.sha512(as_bytes(msg)).digest()


# =============================================================================


def import_rsa_key_from_file(filename):
    return RSA.importKey(open(filename, 'r').read())


def import_rsa_key(key):
    """
    Extract an RSA key from a PEM-encoded certificate

    :param key: RSA key encoded in standard form
    :return: RSA key instance
    """
    return importKey(key)


def der2rsa(der):
    # Extract subjectPublicKeyInfo field from X.509 certificate (see RFC3280)
    cert = DerSequence()
    cert.decode(der)
    tbs_certificate = DerSequence()
    tbs_certificate.decode(cert[0])
    subject_public_key_info = tbs_certificate[6]

    # Initialize RSA key
    return RSA.importKey(subject_public_key_info)


def pem_cert2rsa(pem_file):
    # Convert from PEM to DER
    pem = open(pem_file).read()
    lines = pem.replace(" ", '').split()
    return der2rsa(a2b_base64(''.join(lines[1:-1])))


def der_cert2rsa(der):
    """
    Extract an RSA key from a DER certificate

    @param der: DER-encoded certificate
    @return: RSA instance
    """
    pem = re.sub(r'[^A-Za-z0-9+/]', '', der)
    return der2rsa(base64.b64decode(pem))


def load_x509_cert(url, spec2key):
    """
    Get and transform a X509 cert into a key

    :param url: Where the X509 cert can be found
    :param spec2key: A dictionary over keys already seen
    :return: List of 2-tuples (keytype, key)
    """
    try:
        r = request("GET", url, allow_redirects=True)
        if r.status_code == 200:
            cert = str(r.text)
            try:
                _key = spec2key[cert]
            except KeyError:
                _key = import_rsa_key(cert)
                spec2key[cert] = _key
            return [("rsa", _key)]
        else:
            raise Exception("HTTP Get error: %s" % r.status_code)
    except Exception as err:  # not a RSA key
        logger.warning("Can't load key: %s" % err)
        return []


def rsa_load(filename):
    """Read a PEM-encoded RSA key pair from a file."""
    pem = open(filename, 'r').read()
    return import_rsa_key(pem)


def rsa_eq(key1, key2):
    # Check if two RSA keys are in fact the same
    if key1.n == key2.n and key1.e == key2.e:
        return True
    else:
        return False


def key_eq(key1, key2):
    if type(key1) == type(key2):
        if isinstance(key1, str):
            return key1 == key2
        elif isinstance(key1, RSA):
            return rsa_eq(key1, key2)

    return False


def x509_rsa_load(txt):
    """ So I get the same output format as loads produces
    :param txt:
    :return:
    """
    return [("rsa", import_rsa_key(txt))]


class Key(object):
    """
    Basic JSON Web key class
    """
    members = ["kty", "alg", "use", "kid", "x5c", "x5t", "x5u"]
    longs = []
    public_members = ["kty", "alg", "use", "kid", "x5c", "x5t", "x5u"]

    def __init__(self, kty="", alg="", use="", kid="", key=None, x5c=None,
                 x5t="", x5u="", **kwargs):
        self.key = key
        self.extra_args = kwargs

        # want kty, alg, use and kid to be strings
        if isinstance(kty, six.string_types):
            self.kty = kty
        else:
            self.kty = kty.decode("utf8")

        if isinstance(alg, six.string_types):
            self.alg = alg
        else:
            self.alg = alg.decode("utf8")

        if isinstance(use, six.string_types):
            self.use = use
        else:
            self.use = use.decode("utf8")

        if isinstance(kid, six.string_types):
            self.kid = kid
        else:
            self.kid = kid.decode("utf8")

        self.x5c = x5c or []
        self.x5t = x5t
        self.x5u = x5u
        self.inactive_since = 0

    def to_dict(self):
        """
        A wrapper for to_dict the makes sure that all the private information
        as well as extra arguments are included. This method should *not* be
        used for exporting information about the key.
        """
        res = self.serialize(private=True)
        res.update(self.extra_args)
        return res

    def common(self):
        res = {"kty": self.kty}
        if self.use:
            res["use"] = self.use
        if self.kid:
            res["kid"] = self.kid
        if self.alg:
            res["alg"] = self.alg
        return res

    def __str__(self):
        return str(self.to_dict())

    def deserialize(self):
        """
        Starting with information gathered from the on-the-wire representation
        initiate an appropriate key.
        """
        pass

    def serialize(self, private=False):
        """
        map key characteristics into attribute values that can be used
        to create an on-the-wire representation of the key
        """
        pass

    def get_key(self, **kwargs):
        return self.key

    def verify(self):
        """
        Verify that the information gathered from the on-the-wire
        representation is of the right types.
        This is supposed to be run before the info is deserialized.
        """
        for param in self.longs:
            item = getattr(self, param)
            if not item or isinstance(item, six.integer_types):
                continue

            if isinstance(item, bytes):
                item = item.decode('utf-8')
                setattr(self, param, item)

            try:
                _ = base64url_to_long(item)
            except Exception:
                return False
            else:
                if [e for e in ['+', '/', '='] if e in item]:
                    return False

        if self.kid:
            try:
                assert isinstance(self.kid, six.string_types)
            except AssertionError:
                raise HeaderError("kid of wrong value type")
        return True

    def __eq__(self, other):
        try:
            assert isinstance(other, Key)
            assert list(self.__dict__.keys()) == list(other.__dict__.keys())

            for key in self.public_members:
                assert getattr(other, key) == getattr(self, key)
        except AssertionError:
            return False
        else:
            return True

    def keys(self):
        return list(self.to_dict().keys())


def deser(val):
    if isinstance(val, str):
        _val = val.encode("utf-8")
    else:
        _val = val

    return base64_to_long(_val)


class RSAKey(Key):
    """
    JSON Web key representation of a RSA key
    """
    members = Key.members
    members.extend(["n", "e", "d", "p", "q"])
    longs = ["n", "e", "d", "p", "q", "dp", "dq", "di", "qi"]
    public_members = Key.public_members
    public_members.extend(["n", "e"])

    def __init__(self, kty="RSA", alg="", use="", kid="", key=None,
                 x5c=None, x5t="", x5u="", n="", e="", d="", p="", q="",
                 dp="", dq="", di="", qi="", **kwargs):
        Key.__init__(self, kty, alg, use, kid, key, x5c, x5t, x5u, **kwargs)
        self.n = n
        self.e = e
        self.d = d
        self.p = p
        self.q = q
        self.dp = dp
        self.dq = dq
        self.di = di
        self.qi = qi

        if not self.key and self.n and self.e:
            self.deserialize()
        elif self.key and not (self.n and self.e):
            self._split()

    def deserialize(self):
        if self.n and self.e:
            try:
                for param in self.longs:
                    item = getattr(self, param)
                    if not item or isinstance(item, six.integer_types):
                        continue
                    else:
                        try:
                            val = long(deser(item))
                        except Exception:
                            raise
                        else:
                            setattr(self, param, val)

                lst = [self.n, self.e]
                if self.d:
                    lst.append(self.d)
                if self.p:
                    lst.append(self.p)
                    if self.q:
                        lst.append(self.q)
                    self.key = RSA.construct(tuple(lst))
                else:
                    self.key = RSA.construct(lst)
            except ValueError as err:
                raise DeSerializationNotPossible("%s" % err)
        elif self.x5c:
            if self.x5t:  # verify the cert
                pass

            cert = "\n".join([PREFIX, str(self.x5c[0]), POSTFIX])
            self.key = import_rsa_key(cert)
            self._split()
            if len(self.x5c) > 1:  # verify chain
                pass
        else:
            raise DeSerializationNotPossible()

    def serialize(self, private=False):
        if not self.key:
            raise SerializationNotPossible()

        res = self.common()

        public_longs = list(set(self.public_members) & set(self.longs))
        for param in public_longs:
            item = getattr(self, param)
            if item:
                res[param] = long_to_base64(item)

        if private:
            for param in self.longs:
                if not private and param in ["d", "p", "q", "dp", "dq", "di",
                                             "qi"]:
                    continue
                item = getattr(self, param)
                if item:
                    res[param] = long_to_base64(item)
        return res

    def _split(self):
        self.n = self.key.n
        self.e = self.key.e
        try:
            self.d = self.key.d
        except AttributeError:
            pass
        else:
            for param in ["p", "q"]:
                try:
                    val = getattr(self.key, param)
                except AttributeError:
                    pass
                else:
                    if val:
                        setattr(self, param, val)

    def load(self, filename):
        """
        Load the key from a file.

        :param filename: File name
        """
        self.key = rsa_load(filename)
        self._split()
        return self

    def load_key(self, key):
        """
        Use this RSA key

        :param key: An RSA key instance
        """
        self.key = key
        self._split()
        return self

    def encryption_key(self, **kwargs):
        """
        Make sure there is a key instance present that can be used for
        encrypting/signing.
        """
        if not self.key:
            self.deserialize()

        return self.key


class ECKey(Key):
    """
    JSON Web key representation of a Elliptic curve key
    """
    members = ["kty", "alg", "use", "kid", "crv", "x", "y", "d"]
    longs = ['x', 'y', 'd']
    public_members = ["kty", "alg", "use", "kid", "crv", "x", "y"]

    def __init__(self, kty="EC", alg="", use="", kid="", key=None,
                 crv="", x="", y="", d="", curve=None, **kwargs):
        Key.__init__(self, kty, alg, use, kid, key, **kwargs)
        self.crv = crv
        self.x = x
        self.y = y
        self.d = d
        self.curve = curve

        # Initiated guess as to what state the key is in
        # To be usable for encryption/signing/.. it has to be deserialized
        if self.crv and not self.curve:
            self.verify()
            self.deserialize()

    def deserialize(self):
        """
        Starting with information gathered from the on-the-wire representation
        of an elliptic curve key initiate an Elliptic Curve.
        """
        try:
            if not isinstance(self.x, six.integer_types):
                self.x = deser(self.x)
            if not isinstance(self.y, six.integer_types):
                self.y = deser(self.y)
        except TypeError:
            raise DeSerializationNotPossible()
        except ValueError as err:
            raise DeSerializationNotPossible("%s" % err)

        self.curve = NISTEllipticCurve.by_name(self.crv)
        if self.d:
            try:
                if isinstance(self.d, six.string_types):
                    self.d = deser(self.d)
            except ValueError as err:
                raise DeSerializationNotPossible(str(err))

    def get_key(self, private=False, **kwargs):
        if private:
            return self.d
        else:
            return self.x, self.y

    def serialize(self, private=False):
        if not self.crv and not self.curve:
            raise SerializationNotPossible()

        res = self.common()
        res.update({
            "crv": self.curve.name(),
            "x": long_to_base64(self.x),
            "y": long_to_base64(self.y)
        })

        if private and self.d:
            res["d"] = long_to_base64(self.d)

        return res

    def load_key(self, key):
        self.curve = key
        self.d, (self.x, self.y) = key.key_pair()
        return self

    def decryption_key(self):
        return self.get_key(private=True)

    def encryption_key(self, private=False, **kwargs):
        # both for encryption and decryption.
        return self.get_key(private=private)


ALG2KEYLEN = {
    "A128KW": 16,
    "A192KW": 24,
    "A256KW": 32,
    "HS256": 32,
    "HS384": 48,
    "HS512": 64
}


class SYMKey(Key):
    members = ["kty", "alg", "use", "kid", "k"]
    public_members = members[:]

    def __init__(self, kty="oct", alg="", use="", kid="", key=None,
                 x5c=None, x5t="", x5u="", k="", mtrl="", **kwargs):
        Key.__init__(self, kty, alg, use, kid, as_bytes(key), x5c, x5t, x5u, **kwargs)
        self.k = k
        if not self.key and self.k:
            if isinstance(self.k, str):
                self.k = self.k.encode("utf-8")
            self.key = b64d(bytes(self.k))

    def deserialize(self):
        self.key = b64d(bytes(self.k))

    def serialize(self, private=True):
        res = self.common()
        res["k"] = b64e(bytes(self.key))
        return res

    def encryption_key(self, alg, **kwargs):
        if not self.key:
            self.deserialize()

        tsize = ALG2KEYLEN[alg]
        _keylen = len(self.key)

        if _keylen <= 32:
            # SHA256
            _enc_key = sha256_digest(self.key)[:tsize]
        elif _keylen <= 48:
            # SHA384
            _enc_key = sha384_digest(self.key)[:tsize]
        elif _keylen <= 64:
            # SHA512
            _enc_key = sha512_digest(self.key)[:tsize]
        else:
            raise JWKException("No support for symmetric keys > 512 bits")

        return _enc_key

# -----------------------------------------------------------------------------


def keyitems2keyreps(keyitems):
    keys = []
    for key_type, _keys in list(keyitems.items()):
        if key_type.upper() == "RSA":
            keys.extend([RSAKey(key=k) for k in _keys])
        elif key_type.lower() == "oct":
            keys.extend([SYMKey(key=k) for k in _keys])
        elif key_type.upper() == "EC":
            keys.extend([ECKey(key=k) for k in _keys])
        else:
            keys.extend([Key(key=k) for k in _keys])
    return keys


def keyrep(kspec, enc="utf-8"):
    """
    Instantiate a Key given a set of key/word arguments

    :param kspec: Key specification, arguments to the Key initialization
    :param enc: The encoding of the strings. If it's JSON which is the default
     the encoding is utf-8.
    :return: Key instance
    """
    if enc:
        _kwargs = {}
        for key, val in kspec.items():
            if isinstance(val, str):
                _kwargs[key] = val.encode(enc)
            else:
                _kwargs[key] = val
    else:
        _kwargs = kspec

    if kspec["kty"] == "RSA":
        item = RSAKey(**_kwargs)
    elif kspec["kty"] == "oct":
        item = SYMKey(**_kwargs)
    elif kspec["kty"] == "EC":
        item = ECKey(**_kwargs)
    else:
        item = Key(**_kwargs)
    return item


def jwk_wrap(key, use="", kid=""):
    """
    Instantiated a Key instance with the given key

    :param key: The keys to wrap
    :param use: What the key are expected to be use for
    :param kid: A key id
    :return: The Key instance
    """
    if isinstance(key, _RSAobj):
        kspec = RSAKey(use=use, kid=kid).load_key(key)
    elif isinstance(key, str):
        kspec = SYMKey(key=key, use=use, kid=kid)
    elif isinstance(key, NISTEllipticCurve):
        kspec = ECKey(use=use, kid=kid).load_key(key)
    else:
        raise Exception("Unknown key type:key="+str(type(key)))

    kspec.serialize()
    return kspec


class KEYS(object):
    def __init__(self):
        self._keys = []

    def load_dict(self, dikt):
        for kspec in dikt["keys"]:
            self._keys.append(keyrep(kspec))

    def load_jwks(self, jwks):
        """
        Load and create keys from a JWKS JSON representation

        Expects something on this form::

            {"keys":
                [
                    {"kty":"EC",
                     "crv":"P-256",
                     "x":"MKBCTNIcKUSDii11ySs3526iDZ8AiTo7Tu6KPAqv7D4",
                    "y":"4Etl6SRW2YiLUrN5vfvVHuhp7x8PxltmWWlbbM4IFyM",
                    "use":"enc",
                    "kid":"1"},

                    {"kty":"RSA",
                    "n": "0vx7agoebGcQSuuPiLJXZptN9nndrQmbXEps2aiAFb....."
                    "e":"AQAB",
                    "kid":"2011-04-29"}
                ]
            }

        :param jwks: The JWKS JSON string representation
        :return: list of 2-tuples containing key, type
        """
        return self.load_dict(json.loads(jwks))

    def dump_jwks(self):
        """
        :return: A JWKS representation of the held keys
        """
        res = []
        for key in self._keys:
            res.append(b2s_conv(key.serialize()))

        return json.dumps({"keys": res})

    def load_from_url(self, url, verify=True):
        """
        Get and transform a JWKS into keys

        :param url: Where the JWKS can be found
        :param verify: SSL cert verification
        :return: list of keys
        """

        r = request("GET", url, allow_redirects=True, verify=verify)
        if r.status_code == 200:
            return self.load_jwks(r.text)
        else:
            raise Exception("HTTP Get error: %s" % r.status_code)

    def __getitem__(self, item):
        """
        Get all keys of a specific key type

        :param kty: Key type
        :return: list of keys
        """
        kty = item.lower()
        return [k for k in self._keys if k.kty.lower() == kty]

    def __iter__(self):
        for k in self._keys:
            yield k

    def __len__(self):
        return len(self._keys)

    def keys(self):
        return list(set([k.kty for k in self._keys]))

    def __repr__(self):
        return self.dump_jwks()

    def __str__(self):
        return self.__repr__()

    def kids(self):
        return [k.kid for k in self._keys if k.kid]

    def by_kid(self, kid):
        return [k for k in self._keys if kid == k.kid]

    def wrap_add(self, keyinst, use="", kid=''):
        self._keys.append(jwk_wrap(keyinst, use, kid))

    def as_dict(self):
        _res = {}
        for kty, k in [(k.kty, k) for k in self._keys]:
            if kty not in ["RSA", "EC", "oct"]:
                if kty in ["rsa", "ec"]:
                    kty = kty.upper()
                else:
                    kty = kty.lower()

            try:
                _res[kty].append(k)
            except KeyError:
                _res[kty] = [k]
        return _res

    def add(self, item, enc="utf-8"):
        self._keys.append(keyrep(item, enc))
