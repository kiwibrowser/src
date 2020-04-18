package java.net;

import junit.framework.TestCase;

import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Tests for race conditions between {@link ServerSocket#close()} and
 * {@link ServerSocket#accept()}.
 */
public class ServerSocketConcurrentCloseTest extends TestCase {
    private static final String TAG = ServerSocketConcurrentCloseTest.class.getSimpleName();

    /**
     * The implementation of {@link ServerSocket#accept()} checks closed state before
     * delegating to the {@link ServerSocket#implAccept(Socket)}, however this is not
     * sufficient for correctness because the socket might be closed after the check.
     * This checks that implAccept() itself also detects closed sockets and throws
     * SocketException.
     */
    public void testImplAccept_detectsClosedState() throws Exception {
        /** A ServerSocket that exposes implAccept() */
        class ExposedServerSocket extends ServerSocket {
            public ExposedServerSocket() throws IOException {
                super(0 /* allocate port number automatically */);
            }

            public void implAcceptExposedForTest(Socket socket) throws IOException {
                implAccept(socket);
            }
        }
        final ExposedServerSocket serverSocket = new ExposedServerSocket();
        serverSocket.close();
        try {
            // Hack: Need to subclass to access the protected constructor without reflection
            Socket socket = new Socket((SocketImpl) null) { };
            serverSocket.implAcceptExposedForTest(socket);
            fail("accepting on a closed socket should throw");
        } catch (SocketException expected) {
            // expected
        } catch (IOException e) {
            throw new AssertionError(e);
        }
    }

    /**
     * Test for b/27763633.
     */
    public void testConcurrentServerSocketCloseReliablyThrows() {
        int numIterations = 200;
        for (int i = 0; i < numIterations; i++) {
            checkConnectIterationAndCloseSocket("Iteration " + (i+1) + " of " + numIterations,
                    /* msecPerIteration */ 50);
        }
    }

    /**
     * Checks that a concurrent {@link ServerSocket#close()} reliably causes
     * {@link ServerSocket#accept()} to throw {@link SocketException}.
     *
     * <p>Spawns a server and client thread that continuously connect to each other
     * for {@code msecPerIteration} msec. Then, closes the {@link ServerSocket} and
     * verifies that the server quickly shuts down.
     */
    private void checkConnectIterationAndCloseSocket(String iterationName, int msecPerIteration) {
        ServerSocket serverSocket;
        try {
            serverSocket = new ServerSocket(0 /* allocate port number automatically */);
        } catch (IOException e) {
            fail("Abort: " + e);
            throw new AssertionError("unreachable");
        }
        final CountDownLatch shutdownLatch = new CountDownLatch(1);
        ServerRunnable serverRunnable = new ServerRunnable(serverSocket, shutdownLatch);
        Thread serverThread = new Thread(serverRunnable, TAG + " (server)");
        ClientRunnable clientRunnable = new ClientRunnable(
                serverSocket.getLocalSocketAddress(), shutdownLatch);
        Thread clientThread = new Thread(clientRunnable, TAG + " (client)");
        serverThread.start();
        clientThread.start();
        try {
            if (shutdownLatch.getCount() == 0) {
                fail("Server prematurely shut down");
            }
            // Let server and client keep connecting for some time, then close the socket.
            Thread.sleep(msecPerIteration);
            try {
                serverSocket.close();
            } catch (IOException e) {
                throw new AssertionError("serverSocket.close() failed: ", e);
            }
            // Check that the server shut down quickly in response to the socket closing.
            long hardLimitSeconds = 5;
            boolean serverShutdownReached = shutdownLatch.await(hardLimitSeconds, TimeUnit.SECONDS);
            if (!serverShutdownReached) { // b/27763633
                shutdownLatch.countDown();
                String serverStackTrace = stackTraceAsString(serverThread.getStackTrace());
                fail("Server took > " + hardLimitSeconds + "sec to react to serverSocket.close(). "
                        + "Server thread's stackTrace: " + serverStackTrace);
            }
            // Guard against the test passing as a false positive if no connections were actually
            // established. If the test was running for much longer then this would fail during
            // later iterations because TCP connections cannot be closed immediately (they stay
            // in TIME_WAIT state for a few minutes) and only some number (tens of thousands?)
            // can be open at a time. If this assertion turns out flaky in future, consider
            // reducing msecPerIteration or number of iterations.
            assertTrue(String.format(Locale.US, "%s: No connections in %d msec.",
                    iterationName, msecPerIteration),
                    serverRunnable.numSuccessfulConnections > 0);

            assertEquals(0, shutdownLatch.getCount());
            // Sanity check to ensure the threads don't live into the next iteration. This should
            // be quick because we only get here if shutdownLatch reached 0 within the time limit.
            serverThread.join();
            clientThread.join();
        } catch (InterruptedException e) {
            throw new AssertionError("Unexpected interruption", e);
        }
    }

    /**
     * Repeatedly tries to connect to and disconnect from a SocketAddress until
     * it observes {@code shutdownLatch} reaching 0. Does not read/write any
     * data from/to the socket.
     */
    static class ClientRunnable implements Runnable {
        private final SocketAddress socketAddress;
        private final CountDownLatch shutdownLatch;

        public ClientRunnable(
                SocketAddress socketAddress, CountDownLatch shutdownLatch) {
            this.socketAddress = socketAddress;
            this.shutdownLatch = shutdownLatch;
        }

        @Override
        public void run() {
            while (shutdownLatch.getCount() != 0) { // check if server is shutting down
                try {
                    Socket socket = new Socket();
                    socket.connect(socketAddress, /* timeout (msec) */ 10);
                    socket.close();
                } catch (IOException e) {
                    // harmless, as long as enough connections are successful
                }
            }
        }
    }

    /**
     * Repeatedly accepts connections from a ServerSocket and immediately closes them.
     * When it encounters a SocketException, it counts down the CountDownLatch and exits.
     */
    static class ServerRunnable implements Runnable {
        private final ServerSocket serverSocket;
        volatile int numSuccessfulConnections;
        private final CountDownLatch shutdownLatch;

        ServerRunnable(ServerSocket serverSocket, CountDownLatch shutdownLatch) {
            this.serverSocket = serverSocket;
            this.shutdownLatch = shutdownLatch;
        }

        @Override
        public void run() {
            int numSuccessfulConnections = 0;
            while (true) {
                try {
                    Socket socket = serverSocket.accept();
                    numSuccessfulConnections++;
                    socket.close();
                } catch (SocketException e) {
                    this.numSuccessfulConnections = numSuccessfulConnections;
                    shutdownLatch.countDown();
                    return;
                } catch (IOException e) {
                    // harmless, as long as enough connections are successful
                }
            }
        }
    }

    private static String stackTraceAsString(StackTraceElement[] stackTraceElements) {
        StringBuilder sb = new StringBuilder();
        for (StackTraceElement stackTraceElement : stackTraceElements) {
            sb.append("\n\t at ").append(stackTraceElement);
        }
        return sb.toString();
    }

}
