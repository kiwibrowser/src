/*
** Copyright (c) 2015 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

package org.khronos.portforwarder;

import android.util.Log;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.RuntimeException;
import java.lang.Thread;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashSet;

/** Forwards a TCP port from device to host computer.
 *
 * A class that forwards one TCP port from device to host computer.
 * The forwarding is done using adb.
 *
 * Forwards the a port on device to the same port on the host running
 * the adb connection.
 *
 * The forwarding is stopped when either of the sides closes the connection.
 * The thread that exits when the forwarding stops.
 */
public class AdbPortForwarder extends Thread {
    private static final String LOG_TAG = "AdbPortForwarder";

    private static final String ADB_OK = "OKAY";
    private static final int ADB_PORT = 5037;
    private static final String ADB_HOST = "127.0.0.1";
    private static final int ADB_RESPONSE_SIZE = 4;

    private static final String REMOTE_ADDRESS = "127.0.0.1";

    private static final int FORWARDING_BUFFER_SIZE = 16384;

    private ServerSocket mServerSocket;
    private boolean mIsShuttingDown;
    private int mPort;
    private HashSet<PairedSockets> mRunningPairedSockets;

    public AdbPortForwarder(int port) {
        mPort = port;
    }

    synchronized public void beginForwarding() {
        if (mServerSocket != null)
            return;

        start();

        while (mServerSocket == null) {
            try {
                this.wait();
            } catch (InterruptedException e) {}
        }
    }

    synchronized public void endForwarding() {
        if (mServerSocket == null)
            return;

        try {
            mIsShuttingDown = true;
            mServerSocket.close();
        } catch (IOException e) {}

        while (mServerSocket != null) {
            try {
                this.wait();
            } catch (InterruptedException e) {}
        }

        HashSet<PairedSockets> runningPairs = mRunningPairedSockets;
        mRunningPairedSockets = null;

        if (runningPairs != null) {
            // Close the running sockets. This notifies the threads to stop too.
            for (PairedSockets runningPair: runningPairs)
                runningPair.stopForwarding();
        }
    }

    @Override
    public void run() {
        try {
            ServerSocket s = new ServerSocket(mPort);
            s.setReceiveBufferSize(FORWARDING_BUFFER_SIZE);
            synchronized(this) {
                mServerSocket = s;
                this.notify();
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Unable to start port " + mPort + " forwarding to port " + mPort);
            return;
        }


        while (true) {
            synchronized (this) {
                if (mIsShuttingDown)
                    break;
            }

            Socket localQuerySocket = null;
            Socket adbSocket = null;
            try {
                localQuerySocket = mServerSocket.accept();
                localQuerySocket.setSendBufferSize(FORWARDING_BUFFER_SIZE);
                adbSocket = createAdbSocket();

                pairSocketsAndStart(localQuerySocket, adbSocket);

            } catch (Exception e) {
                try {
                    if (localQuerySocket != null)
                        localQuerySocket.close();
                } catch (IOException ioe) {}

                try {
                    if (adbSocket != null)
                        adbSocket.close();
                } catch (IOException ioe) {}
                break;
            }
        }

        synchronized (this) {
            mServerSocket = null;
            this.notify();
        }

    }

    private Socket createAdbSocket() throws IOException {
        Socket adbSocket = new Socket(ADB_HOST, ADB_PORT);
        adbSocket.setReceiveBufferSize(FORWARDING_BUFFER_SIZE);
        adbSocket.setSendBufferSize(FORWARDING_BUFFER_SIZE);
        String cmd = "tcp:" + mPort + ":" + REMOTE_ADDRESS;
        cmd = String.format("%04X", cmd.length()) + cmd;

        byte[] buf = new byte[ADB_RESPONSE_SIZE];
        adbSocket.getOutputStream().write(cmd.getBytes());
        adbSocket.getOutputStream().flush();
        int read = adbSocket.getInputStream().read(buf);
        if (read != ADB_RESPONSE_SIZE || !ADB_OK.equals(new String(buf))) {
            adbSocket.close();
            throw new RuntimeException("Unable to start port " + mPort + " forwarding to port " + mPort);
        }
        return adbSocket;
    }

    synchronized private void pairSocketsAndStart(Socket a, Socket b) {
        if (mRunningPairedSockets == null)
            mRunningPairedSockets = new HashSet<PairedSockets>();

        PairedSockets pair = new PairedSockets(a, b, this);
        mRunningPairedSockets.add(pair);
        pair.startForwarding();
    }

    synchronized private void pairedSocketsForwardingStopped(PairedSockets pair) {
        if (mRunningPairedSockets != null)
            mRunningPairedSockets.remove(pair);
    }

    private class PairedSockets {
        Socket mA;
        Socket mB;
        AdbPortForwarder mOwner;

        public PairedSockets(Socket a, Socket b, AdbPortForwarder owner) {
            mA = a;
            mB = b;
            mOwner = owner;
        }

        /** Starts forwarding for the sockets.
         *
         * Closing the socket will notify the thread to stop.  Closing either socket will cause both
         * threads to stop.
         */
        public void startForwarding() {
            final Socket a = mA;
            final Socket b = mB;

            new Thread() {
                @Override
                public void run() {
                    forwardUntilEnd(a, b);
                }
            }.start();

            new Thread() {
                @Override
                public void run() {
                    forwardUntilEnd(b, a);
                }
            }.start();
        }

        /** Forwards data from input to output.
         *
         * Stops immediately when either of the streams fail to deliver data.
         */
        private void forwardUntilEnd(Socket input, Socket output) {
            try {
                InputStream i = input.getInputStream();
                OutputStream o = output.getOutputStream();

                byte[] buffer = new byte[FORWARDING_BUFFER_SIZE];
                int length;
                while (true) {
                    if ((length = i.read(buffer)) < 0)
                        break;

                    o.write(buffer, 0, length);
                    o.flush();
                }
            } catch (IOException e) { }

            /* Close both of the connections immediately when either of them closes.
             * This is important when a test times out and server closes HTTP keep-alive
             * connection. If we don't close the local socket, the client will
             * send the next query to this socket and it will be lost.
             */

            if (stopForwarding()) {
                // Inform the owner that these sockets do not need closing.
                mOwner.pairedSocketsForwardingStopped(this);
            }
        }

        synchronized public boolean stopForwarding() {
            if (mA != null) {
                try {
                    if (!mA.isClosed())
                        mA.close();
                } catch (IOException e) {}

                try {
                    if (!mB.isClosed())
                        mB.close();
                } catch (IOException e) {}

                mA = null;
                mB = null;
                return true;
            }
            return false;
        }
    }
}

