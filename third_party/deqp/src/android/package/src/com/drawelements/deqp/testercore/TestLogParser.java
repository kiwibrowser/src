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
 * \brief Test log parser for instrumentation
 *//*--------------------------------------------------------------------*/

package com.drawelements.deqp.testercore;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public class TestLogParser
{
	static {
		System.loadLibrary("deqp");
	}

	private long				m_nativePointer;
	private DeqpInstrumentation	m_instrumentation;
	private FileInputStream		m_log;
	private String				m_logFileName;
	private byte[]				m_buffer;
	private long				m_logRead;

	public TestLogParser ()
	{
		m_nativePointer		= 0;
		m_instrumentation	= null;
		m_log				= null;
		m_logRead			= 0;
		m_buffer			= null;
	}

	public void init (DeqpInstrumentation instrumentation, String logFileName, boolean logData) throws FileNotFoundException
	{
		assert instrumentation != null;
		assert m_instrumentation == null;
		assert m_nativePointer == 0;
		assert m_log == null;

		m_logFileName		= logFileName;
		m_instrumentation	= instrumentation;
		m_nativePointer		= nativeCreate(logData);
		m_buffer			= new byte[4*1024*1024];
		m_log				= new FileInputStream(m_logFileName);
	}

	public void deinit () throws IOException
	{
		assert m_nativePointer != 0;
		assert m_instrumentation != null;
		assert m_log != null;

		nativeDestroy(m_nativePointer);

		if (m_log != null)
			m_log.close();

		m_nativePointer		= 0;
		m_instrumentation	= null;
		m_log				= null;
		m_buffer			= null;
	}

	public boolean parse () throws IOException
	{
		assert m_nativePointer != 0;
		assert m_instrumentation != null;
		assert m_log != null;

		boolean	gotData	= false;

		while (true)
		{
			final int numAvailable = m_log.available();

			if (numAvailable <= 0)
				break;

			final int numRead = m_log.read(m_buffer, 0, Math.min(numAvailable, m_buffer.length));

			assert numRead > 0;

			m_logRead += numRead;

			gotData = true;
			nativeParse(m_nativePointer, m_instrumentation, m_buffer, numRead);
		}

		return gotData;
	}

	private static native long	nativeCreate	(boolean logData);
	private static native void	nativeDestroy	(long nativePointer);
	private static native void	nativeParse		(long nativePointer, DeqpInstrumentation instrumentation, byte[] buffer, int size);
}
