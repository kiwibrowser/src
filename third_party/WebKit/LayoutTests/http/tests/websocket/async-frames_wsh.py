from mod_pywebsocket import msgutil
import time


# Pause for long enough that frames are processed separately by the browser.
def pause_briefly():
    time.sleep(0.01)


def web_socket_do_extra_handshake(request):
    pass


def web_socket_transfer_data(request):
    # send_message's third argument corresponds to "fin" bit;
    # it is set to True if this frame is the final fragment of a message.

    pause_briefly()
    msgutil.send_message(request, '', False)
    pause_briefly()
    msgutil.send_message(request, '', True)
    pause_briefly()
