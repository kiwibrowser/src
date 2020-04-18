/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.test;

import android.os.HandlerThread;
import android.os.Looper;
import android.telephony.Rlog;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.List;

// Also in ATChannel.java
class LineReader
{
    /**
     * Not threadsafe
     * Assumes input is ASCII
     */

    //***** Constants

    // For what it's worth, this is also the size of an
    // OMAP CSMI mailbox
    static final int BUFFER_SIZE = 0x1000;

    // just to prevent constant allocations
    byte mBuffer[] = new byte[BUFFER_SIZE];

    //***** Instance Variables

    InputStream mInStream;

    LineReader (InputStream s)
    {
        mInStream = s;
    }

    String
    getNextLine()
    {
        return getNextLine(false);
    }

    String
    getNextLineCtrlZ()
    {
        return getNextLine(true);
    }

    /**
     * Note: doesn't return the last incomplete line read on EOF, since
     * it doesn't typically matter anyway
     *
     * Returns NULL on EOF
     */

    String
    getNextLine(boolean ctrlZ)
    {
        int i = 0;

        try {
            for (;;) {
                int result;

                result = mInStream.read();

                if (result < 0) {
                    return null;
                }

                if (ctrlZ && result == 0x1a) {
                    break;
                } else if (result == '\r' || result == '\n') {
                    if (i == 0) {
                        // Skip leading cr/lf
                        continue;
                    } else {
                        break;
                    }
                }

                mBuffer[i++] = (byte)result;
            }
        } catch (IOException ex) {
            return null;
        } catch (IndexOutOfBoundsException ex) {
            System.err.println("ATChannel: buffer overflow");
        }

        try {
            return new String(mBuffer, 0, i, "US-ASCII");
        } catch (UnsupportedEncodingException ex) {
            System.err.println("ATChannel: implausable UnsupportedEncodingException");
            return null;
        }
    }
}



class InterpreterEx extends Exception
{
    public
    InterpreterEx (String result)
    {
        mResult = result;
    }

    String mResult;
}

public class ModelInterpreter
            implements Runnable, SimulatedRadioControl
{
    static final int MAX_CALLS = 6;

    /** number of msec between dialing -> alerting and alerting->active */
    static final int CONNECTING_PAUSE_MSEC = 5 * 100;

    static final String LOG_TAG = "ModelInterpreter";

    //***** Instance Variables

    InputStream mIn;
    OutputStream mOut;
    LineReader mLineReader;
    ServerSocket mSS;

    private String mFinalResponse;

    SimulatedGsmCallState mSimulatedCallState;

    HandlerThread mHandlerThread;

    int mPausedResponseCount;
    Object mPausedResponseMonitor = new Object();

    //***** Events

    static final int PROGRESS_CALL_STATE        = 1;

    //***** Constructor

    public
    ModelInterpreter (InputStream in, OutputStream out)
    {
        mIn = in;
        mOut = out;

        init();
    }

    public
    ModelInterpreter (InetSocketAddress sa) throws java.io.IOException
    {
        mSS = new ServerSocket();

        mSS.setReuseAddress(true);
        mSS.bind(sa);

        init();
    }

    private void
    init()
    {
        new Thread(this, "ModelInterpreter").start();
        mHandlerThread = new HandlerThread("ModelInterpreter");
        mHandlerThread.start();
        Looper looper = mHandlerThread.getLooper();
        mSimulatedCallState = new SimulatedGsmCallState(looper);
    }

    //***** Runnable Implementation

    @Override
    public void run()
    {
        for (;;) {
            if (mSS != null) {
                Socket s;

                try {
                    s = mSS.accept();
                } catch (java.io.IOException ex) {
                    Rlog.w(LOG_TAG,
                        "IOException on socket.accept(); stopping", ex);
                    return;
                }

                try {
                    mIn = s.getInputStream();
                    mOut = s.getOutputStream();
                } catch (java.io.IOException ex) {
                    Rlog.w(LOG_TAG,
                        "IOException on accepted socket(); re-listening", ex);
                    continue;
                }

                Rlog.i(LOG_TAG, "New connection accepted");
            }


            mLineReader = new LineReader (mIn);

            println ("Welcome");

            for (;;) {
                String line;

                line = mLineReader.getNextLine();

                //System.out.println("MI<< " + line);

                if (line == null) {
                    break;
                }

                synchronized(mPausedResponseMonitor) {
                    while (mPausedResponseCount > 0) {
                        try {
                            mPausedResponseMonitor.wait();
                        } catch (InterruptedException ex) {
                        }
                    }
                }

                synchronized (this) {
                    try {
                        mFinalResponse = "OK";
                        processLine(line);
                        println(mFinalResponse);
                    } catch (InterpreterEx ex) {
                        println(ex.mResult);
                    } catch (RuntimeException ex) {
                        ex.printStackTrace();
                        println("ERROR");
                    }
                }
            }

            Rlog.i(LOG_TAG, "Disconnected");

            if (mSS == null) {
                // no reconnect in this case
                break;
            }
        }
    }


    //***** Instance Methods

    /** Start the simulated phone ringing */
    @Override
    public void
    triggerRing(String number)
    {
        synchronized (this) {
            boolean success;

            success = mSimulatedCallState.triggerRing(number);

            if (success) {
                println ("RING");
            }
        }
    }

    /** If a call is DIALING or ALERTING, progress it to the next state */
    @Override
    public void
    progressConnectingCallState()
    {
        mSimulatedCallState.progressConnectingCallState();
    }


    /** If a call is DIALING or ALERTING, progress it all the way to ACTIVE */
    @Override
    public void
    progressConnectingToActive()
    {
        mSimulatedCallState.progressConnectingToActive();
    }

    /** automatically progress mobile originated calls to ACTIVE.
     *  default to true
     */
    @Override
    public void
    setAutoProgressConnectingCall(boolean b)
    {
        mSimulatedCallState.setAutoProgressConnectingCall(b);
    }

    @Override
    public void
    setNextDialFailImmediately(boolean b)
    {
        mSimulatedCallState.setNextDialFailImmediately(b);
    }

    @Override
    public void setNextCallFailCause(int gsmCause)
    {
        //FIXME implement
    }


    /** hangup ringing, dialing, or actuve calls */
    @Override
    public void
    triggerHangupForeground()
    {
        boolean success;

        success = mSimulatedCallState.triggerHangupForeground();

        if (success) {
            println ("NO CARRIER");
        }
    }

    /** hangup holding calls */
    @Override
    public void
    triggerHangupBackground()
    {
        boolean success;

        success = mSimulatedCallState.triggerHangupBackground();

        if (success) {
            println ("NO CARRIER");
        }
    }

    /** hangup all */

    @Override
    public void
    triggerHangupAll()
    {
        boolean success;

        success = mSimulatedCallState.triggerHangupAll();

        if (success) {
            println ("NO CARRIER");
        }
    }

    public void
    sendUnsolicited (String unsol)
    {
        synchronized (this) {
            println(unsol);
        }
    }

    @Override
    public void triggerSsn(int a, int b) {}
    @Override
    public void triggerIncomingUssd(String statusCode, String message) {}

    @Override
    public void
    triggerIncomingSMS(String message)
    {
/**************
        StringBuilder pdu = new StringBuilder();

        pdu.append ("00");      //SMSC address - 0 bytes

        pdu.append ("04");      // Message type indicator

        // source address: +18005551212
        pdu.append("918100551521F0");

        // protocol ID and data coding scheme
        pdu.append("0000");

        Calendar c = Calendar.getInstance();

        pdu.append (c.



        synchronized (this) {
            println("+CMT: ,1\r" + pdu.toString());
        }

**************/
    }

    @Override
    public void
    pauseResponses()
    {
        synchronized(mPausedResponseMonitor) {
            mPausedResponseCount++;
        }
    }

    @Override
    public void
    resumeResponses()
    {
        synchronized(mPausedResponseMonitor) {
            mPausedResponseCount--;

            if (mPausedResponseCount == 0) {
                mPausedResponseMonitor.notifyAll();
            }
        }
    }

    //***** Private Instance Methods

    private void
    onAnswer() throws InterpreterEx
    {
        boolean success;

        success = mSimulatedCallState.onAnswer();

        if (!success) {
            throw new InterpreterEx("ERROR");
        }
    }

    private void
    onHangup() throws InterpreterEx
    {
        boolean success = false;

        success = mSimulatedCallState.onAnswer();

        if (!success) {
            throw new InterpreterEx("ERROR");
        }

        mFinalResponse = "NO CARRIER";
    }

    private void
    onCHLD(String command) throws InterpreterEx
    {
        // command starts with "+CHLD="
        char c0;
        char c1 = 0;
        boolean success;

        c0 = command.charAt(6);

        if (command.length() >= 8) {
            c1 = command.charAt(7);
        }

        success = mSimulatedCallState.onChld(c0, c1);

        if (!success) {
            throw new InterpreterEx("ERROR");
        }
    }

    private void
    onDial(String command) throws InterpreterEx
    {
        boolean success;

        success = mSimulatedCallState.onDial(command.substring(1));

        if (!success) {
            throw new InterpreterEx("ERROR");
        }
    }

    private void
    onCLCC()
    {
        List<String> lines;

        lines = mSimulatedCallState.getClccLines();

        for (int i = 0, s = lines.size() ; i < s ; i++) {
            println (lines.get(i));
        }
    }

    private void
    onSMSSend(String command)
    {
        String pdu;

        print ("> ");
        pdu = mLineReader.getNextLineCtrlZ();

        println("+CMGS: 1");
    }

    void
    processLine (String line) throws InterpreterEx
    {
        String[] commands;

        commands = splitCommands(line);

        for (int i = 0; i < commands.length ; i++) {
            String command = commands[i];

            if (command.equals("A")) {
                onAnswer();
            } else if (command.equals("H")) {
                onHangup();
            } else if (command.startsWith("+CHLD=")) {
                onCHLD(command);
            } else if (command.equals("+CLCC")) {
                onCLCC();
            } else if (command.startsWith("D")) {
                onDial(command);
            } else if (command.startsWith("+CMGS=")) {
                onSMSSend(command);
            } else {
                boolean found = false;

                for (int j = 0; j < sDefaultResponses.length ; j++) {
                    if (command.equals(sDefaultResponses[j][0])) {
                        String r = sDefaultResponses[j][1];
                        if (r != null) {
                            println(r);
                        }
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    throw new InterpreterEx ("ERROR");
                }
            }
        }
    }


    String[]
    splitCommands(String line) throws InterpreterEx
    {
        if (!line.startsWith ("AT")) {
            throw new InterpreterEx("ERROR");
        }

        if (line.length() == 2) {
            // Just AT by itself
            return new String[0];
        }

        String ret[] = new String[1];

        //TODO fix case here too
        ret[0] = line.substring(2);

        return ret;
/****
        try {
            // i = 2 to skip over AT
            for (int i = 2, s = line.length() ; i < s ; i++) {
                // r"|([A-RT-Z]\d?)" # Normal commands eg ATA or I0
                // r"|(&[A-Z]\d*)" # & commands eg &C
                // r"|(S\d+(=\d+)?)" # S registers
                // r"((\+|%)\w+(\?|=([^;]+(;|$)))?)" # extended command eg +CREG=2


            }
        } catch (StringIndexOutOfBoundsException ex) {
            throw new InterpreterEx ("ERROR");
        }
***/
    }

    void
    println (String s)
    {
        synchronized(this) {
            try {
                byte[] bytes =  s.getBytes("US-ASCII");

                //System.out.println("MI>> " + s);

                mOut.write(bytes);
                mOut.write('\r');
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }

    void
    print (String s)
    {
        synchronized(this) {
            try {
                byte[] bytes =  s.getBytes("US-ASCII");

                //System.out.println("MI>> " + s + " (no <cr>)");

                mOut.write(bytes);
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }


    @Override
    public void
    shutdown()
    {
        Looper looper = mHandlerThread.getLooper();
        if (looper != null) {
            looper.quit();
        }

        try {
            mIn.close();
        } catch (IOException ex) {
        }
        try {
            mOut.close();
        } catch (IOException ex) {
        }
    }


    static final String [][] sDefaultResponses = {
        {"E0Q0V1",   null},
        {"+CMEE=2",  null},
        {"+CREG=2",  null},
        {"+CGREG=2", null},
        {"+CCWA=1",  null},
        {"+COPS=0",  null},
        {"+CFUN=1",  null},
        {"+CGMI",    "+CGMI: Android Model AT Interpreter\r"},
        {"+CGMM",    "+CGMM: Android Model AT Interpreter\r"},
        {"+CGMR",    "+CGMR: 1.0\r"},
        {"+CGSN",    "000000000000000\r"},
        {"+CIMI",    "320720000000000\r"},
        {"+CSCS=?",  "+CSCS: (\"HEX\",\"UCS2\")\r"},
        {"+CFUN?",   "+CFUN: 1\r"},
        {"+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?",
                "+COPS: 0,0,\"Android\"\r"
                + "+COPS: 0,1,\"Android\"\r"
                + "+COPS: 0,2,\"310995\"\r"},
        {"+CREG?",   "+CREG: 2,5, \"0113\", \"6614\"\r"},
        {"+CGREG?",  "+CGREG: 2,0\r"},
        {"+CSQ",     "+CSQ: 16,99\r"},
        {"+CNMI?",   "+CNMI: 1,2,2,1,1\r"},
        {"+CLIR?",   "+CLIR: 1,3\r"},
        {"%CPVWI=2", "%CPVWI: 0\r"},
        {"+CUSD=1,\"#646#\"",  "+CUSD=0,\"You have used 23 minutes\"\r"},
        {"+CRSM=176,12258,0,0,10", "+CRSM: 144,0,981062200050259429F6\r"},
        {"+CRSM=192,12258,0,0,15", "+CRSM: 144,0,0000000A2FE204000FF55501020000\r"},

        /* EF[ADN] */
        {"+CRSM=192,28474,0,0,15", "+CRSM: 144,0,0000005a6f3a040011f5220102011e\r"},
        {"+CRSM=178,28474,1,4,30", "+CRSM: 144,0,437573746f6d65722043617265ffffff07818100398799f7ffffffffffff\r"},
        {"+CRSM=178,28474,2,4,30", "+CRSM: 144,0,566f696365204d61696cffffffffffff07918150367742f3ffffffffffff\r"},
        {"+CRSM=178,28474,3,4,30", "+CRSM: 144,0,4164676a6dffffffffffffffffffffff0b918188551512c221436587ff01\r"},
        {"+CRSM=178,28474,4,4,30", "+CRSM: 144,0,810101c1ffffffffffffffffffffffff068114455245f8ffffffffffffff\r"},
        /* EF[EXT1] */
        {"+CRSM=192,28490,0,0,15", "+CRSM: 144,0,000000416f4a040011f5550102010d\r"},
        {"+CRSM=178,28490,1,4,13", "+CRSM: 144,0,0206092143658709ffffffffff\r"}
    };
}
