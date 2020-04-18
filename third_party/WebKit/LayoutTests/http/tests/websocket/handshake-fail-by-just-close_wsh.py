from mod_pywebsocket import handshake


def web_socket_do_extra_handshake(request):
    # Prevents pywebsocket from sending its own handshake message.
    raise handshake.AbortedByUserException('Abort the connection')


def web_socket_transfer_data(request):
    pass
