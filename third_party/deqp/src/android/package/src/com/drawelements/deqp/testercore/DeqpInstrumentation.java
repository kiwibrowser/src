/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *
 *//*!
 * \file
 * \brief dEQP instrumentation
 *//*--------------------------------------------------------------------*/

package com.drawelements.deqp.testercore;

import android.app.Instrumentation;
import android.os.Bundle;

import java.lang.Thread;
import java.io.File;

public class DeqpInstrumentation extends Instrumentation
{
	private static final String	LOG_TAG					= "dEQP/Instrumentation";
	private static final long	LAUNCH_TIMEOUT_MS		= 10000;
	private static final long	NO_DATA_TIMEOUT_MS		= 5000;
	private static final long	NO_ACTIVITY_SLEEP_MS	= 100;
	private static final long	REMOTE_DEAD_SLEEP_MS	= 100;

	private String				m_cmdLine;
	private String				m_logFileName;
	private boolean				m_logData;

	@Override
	public void onCreate (Bundle arguments) {
		super.onCreate(arguments);

		m_cmdLine		= arguments.getString("deqpCmdLine");
		m_logFileName	= arguments.getString("deqpLogFilename");

		if (m_cmdLine == null)
			m_cmdLine = "";

		if (m_logFileName == null)
			m_logFileName = "/sdcard/TestLog.qpa";

		if (arguments.getString("deqpLogData") != null)
		{
			if (arguments.getString("deqpLogData").compareToIgnoreCase("true") == 0)
				m_logData = true;
			else
				m_logData = false;
		}
		else
			m_logData = false;

		start();
	}

	@Override
	public void onStart () {
		super.onStart();

		final RemoteAPI		remoteApi	= new RemoteAPI(getTargetContext(), m_logFileName);
		final TestLogParser	parser		= new TestLogParser();

		try
		{
			Log.d(LOG_TAG, "onStart");

			final String	testerName		= "";
			final File		logFile			= new File(m_logFileName);

			if (logFile.exists())
				logFile.delete(); // Remove log file left by previous session

			remoteApi.start(testerName, m_cmdLine, null);

			{
				final long startTimeMs = System.currentTimeMillis();

				while (true)
				{
					final long timeSinceStartMs = System.currentTimeMillis()-startTimeMs;

					if (logFile.exists())
						break;
					else if (timeSinceStartMs > LAUNCH_TIMEOUT_MS)
					{
						remoteApi.kill();
						throw new Exception("Timeout while waiting for log file");
					}
					else
						Thread.sleep(NO_ACTIVITY_SLEEP_MS);
				}
			}

			parser.init(this, m_logFileName, m_logData);

			// parse until tester dies
			{
				while (true)
				{
					if (!parser.parse())
					{
						Thread.sleep(NO_ACTIVITY_SLEEP_MS);
						if (!remoteApi.isRunning())
							break;
					}
				}
			}

			// parse remaining messages
			{
				long lastDataMs = System.currentTimeMillis();

				while (true)
				{
					if (parser.parse())
						lastDataMs = System.currentTimeMillis();
					else
					{
						final long timeSinceLastDataMs = System.currentTimeMillis()-lastDataMs;

						if (timeSinceLastDataMs > NO_DATA_TIMEOUT_MS)
							break; // Assume no data is available for reading any more

						// Remote is dead, wait a bit until trying to read again
						Thread.sleep(REMOTE_DEAD_SLEEP_MS);
					}
				}
			}

			finish(0, new Bundle());
		}
		catch (Exception e)
		{
			Log.e(LOG_TAG, "Exception", e);

			Bundle info = new Bundle();
			info.putString("Exception", e.getMessage());
			finish(1, info);
		}
		finally
		{
			try
			{
				parser.deinit();
			}
			catch (Exception e)
			{
				Log.w(LOG_TAG, "Got exception while closing log", e);
			}
			remoteApi.kill();
		}
	}

	public void testCaseResult (String code, String details)
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "TestCaseResult");
		info.putString("dEQP-TestCaseResult-Code", code);
		info.putString("dEQP-TestCaseResult-Details", details);

		sendStatus(0, info);
	}

	public void beginTestCase (String testCase)
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "BeginTestCase");
		info.putString("dEQP-BeginTestCase-TestCasePath", testCase);

		sendStatus(0, info);
	}

	public void endTestCase ()
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "EndTestCase");
		sendStatus(0, info);
	}

	public void testLogData (String log) throws InterruptedException
	{
		if (m_logData)
		{
			final int chunkSize = 4*1024;

			while (log != null)
			{
				String message;

				if (log.length() > chunkSize)
				{
					message = log.substring(0, chunkSize);
					log = log.substring(chunkSize);
				}
				else
				{
					message = log;
					log = null;
				}

				Bundle info = new Bundle();

				info.putString("dEQP-EventType", "TestLogData");
				info.putString("dEQP-TestLogData-Log", message);
				sendStatus(0, info);

				if (log != null)
				{
					Thread.sleep(1); // 1ms
				}
			}
		}
	}

	public void beginSession ()
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "BeginSession");
		sendStatus(0, info);
	}

	public void endSession ()
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "EndSession");
		sendStatus(0, info);
	}

	public void sessionInfo (String name, String value)
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "SessionInfo");
		info.putString("dEQP-SessionInfo-Name", name);
		info.putString("dEQP-SessionInfo-Value", value);

		sendStatus(0, info);
	}

	public void terminateTestCase (String reason)
	{
		Bundle info = new Bundle();

		info.putString("dEQP-EventType", "TerminateTestCase");
		info.putString("dEQP-TerminateTestCase-Reason", reason);

		sendStatus(0, info);
	}

	@Override
	public void onDestroy() {
		Log.e(LOG_TAG, "onDestroy");
		super.onDestroy();
		Log.e(LOG_TAG, "onDestroy");
	}
}
