# Authors: 
#   Trevor Perrin
#   Dimitris Moraitis - Anon ciphersuites
#
# See the LICENSE file for legal information regarding use of this file.

"""
A helper class for using TLS Lite with stdlib clients
(httplib, xmlrpclib, imaplib, poplib).
"""

from tlslite.checker import Checker

class ClientHelper(object):
    """This is a helper class used to integrate TLS Lite with various
    TLS clients (e.g. poplib, smtplib, httplib, etc.)"""

    def __init__(self,
              username=None, password=None,
              certChain=None, privateKey=None,
              checker=None,
              settings = None, 
              anon = False):
        """
        For client authentication, use one of these argument
        combinations:
         - username, password (SRP)
         - certChain, privateKey (certificate)

        For server authentication, you can either rely on the
        implicit mutual authentication performed by SRP,
        or you can do certificate-based server
        authentication with one of these argument combinations:
         - x509Fingerprint

        Certificate-based server authentication is compatible with
        SRP or certificate-based client authentication.

        The constructor does not perform the TLS handshake itself, but
        simply stores these arguments for later.  The handshake is
        performed only when this class needs to connect with the
        server.  Then you should be prepared to handle TLS-specific
        exceptions.  See the client handshake functions in
        L{tlslite.TLSConnection.TLSConnection} for details on which
        exceptions might be raised.

        @type username: str
        @param username: SRP username.  Requires the
        'password' argument.

        @type password: str
        @param password: SRP password for mutual authentication.
        Requires the 'username' argument.

        @type certChain: L{tlslite.x509certchain.X509CertChain}
        @param certChain: Certificate chain for client authentication.
        Requires the 'privateKey' argument.  Excludes the SRP arguments.

        @type privateKey: L{tlslite.utils.rsakey.RSAKey}
        @param privateKey: Private key for client authentication.
        Requires the 'certChain' argument.  Excludes the SRP arguments.

        @type checker: L{tlslite.checker.Checker}
        @param checker: Callable object called after handshaking to 
        evaluate the connection and raise an Exception if necessary.

        @type settings: L{tlslite.handshakesettings.HandshakeSettings}
        @param settings: Various settings which can be used to control
        the ciphersuites, certificate types, and SSL/TLS versions
        offered by the client.
        """

        self.username = None
        self.password = None
        self.certChain = None
        self.privateKey = None
        self.checker = None
        self.anon = anon

        #SRP Authentication
        if username and password and not \
                (certChain or privateKey):
            self.username = username
            self.password = password

        #Certificate Chain Authentication
        elif certChain and privateKey and not \
                (username or password):
            self.certChain = certChain
            self.privateKey = privateKey

        #No Authentication
        elif not password and not username and not \
                certChain and not privateKey:
            pass

        else:
            raise ValueError("Bad parameters")

        self.checker = checker
        self.settings = settings

        self.tlsSession = None

    def _handshake(self, tlsConnection):
        if self.username and self.password:
            tlsConnection.handshakeClientSRP(username=self.username,
                                             password=self.password,
                                             checker=self.checker,
                                             settings=self.settings,
                                             session=self.tlsSession)
        elif self.anon:
            tlsConnection.handshakeClientAnonymous(session=self.tlsSession,
                                                settings=self.settings,
                                                checker=self.checker)
        else:
            tlsConnection.handshakeClientCert(certChain=self.certChain,
                                              privateKey=self.privateKey,
                                              checker=self.checker,
                                              settings=self.settings,
                                              session=self.tlsSession)
        self.tlsSession = tlsConnection.session