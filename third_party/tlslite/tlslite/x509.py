# Authors: 
#   Trevor Perrin
#   Google - parsing subject field
#
# See the LICENSE file for legal information regarding use of this file.

"""Class representing an X.509 certificate."""

from .utils.asn1parser import ASN1Parser
from .utils.cryptomath import *
from .utils.keyfactory import _createPublicRSAKey
from .utils.pem import *


class X509(object):
    """This class represents an X.509 certificate.

    @type bytes: L{bytearray} of unsigned bytes
    @ivar bytes: The DER-encoded ASN.1 certificate

    @type publicKey: L{tlslite.utils.rsakey.RSAKey}
    @ivar publicKey: The subject public key from the certificate.

    @type subject: L{bytearray} of unsigned bytes
    @ivar subject: The DER-encoded ASN.1 subject distinguished name.
    """

    def __init__(self):
        self.bytes = bytearray(0)
        self.publicKey = None
        self.subject = None

    def parse(self, s):
        """Parse a PEM-encoded X.509 certificate.

        @type s: str
        @param s: A PEM-encoded X.509 certificate (i.e. a base64-encoded
        certificate wrapped with "-----BEGIN CERTIFICATE-----" and
        "-----END CERTIFICATE-----" tags).
        """

        bytes = dePem(s, "CERTIFICATE")
        self.parseBinary(bytes)
        return self

    def parseBinary(self, bytes):
        """Parse a DER-encoded X.509 certificate.

        @type bytes: str or L{bytearray} of unsigned bytes
        @param bytes: A DER-encoded X.509 certificate.
        """

        self.bytes = bytearray(bytes)
        p = ASN1Parser(bytes)

        #Get the tbsCertificate
        tbsCertificateP = p.getChild(0)

        #Is the optional version field present?
        #This determines which index the key is at.
        if tbsCertificateP.value[0]==0xA0:
            subjectPublicKeyInfoIndex = 6
        else:
            subjectPublicKeyInfoIndex = 5

        #Get the subject
        self.subject = tbsCertificateP.getChildBytes(\
                           subjectPublicKeyInfoIndex - 1)

        #Get the subjectPublicKeyInfo
        subjectPublicKeyInfoP = tbsCertificateP.getChild(\
                                    subjectPublicKeyInfoIndex)

        #Get the algorithm
        algorithmP = subjectPublicKeyInfoP.getChild(0)
        rsaOID = algorithmP.value
        if list(rsaOID) != [6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 5, 0]:
            raise SyntaxError("Unrecognized AlgorithmIdentifier")

        #Get the subjectPublicKey
        subjectPublicKeyP = subjectPublicKeyInfoP.getChild(1)

        #Adjust for BIT STRING encapsulation
        if (subjectPublicKeyP.value[0] !=0):
            raise SyntaxError()
        subjectPublicKeyP = ASN1Parser(subjectPublicKeyP.value[1:])

        #Get the modulus and exponent
        modulusP = subjectPublicKeyP.getChild(0)
        publicExponentP = subjectPublicKeyP.getChild(1)

        #Decode them into numbers
        n = bytesToNumber(modulusP.value)
        e = bytesToNumber(publicExponentP.value)

        #Create a public key instance
        self.publicKey = _createPublicRSAKey(n, e)

    def getFingerprint(self):
        """Get the hex-encoded fingerprint of this certificate.

        @rtype: str
        @return: A hex-encoded fingerprint.
        """
        return b2a_hex(SHA1(self.bytes))

    def writeBytes(self):
        return self.bytes


