/*-------------------------------------------------------------------------
 * drawElements Quality Program Platform Utilites
 * ----------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief dEQP platform capability query instrumentation
 *//*--------------------------------------------------------------------*/

package com.drawelements.deqp.platformutil;

import android.app.Instrumentation;
import android.os.Bundle;

public class DeqpPlatformCapabilityQueryInstrumentation extends Instrumentation
{
	static
	{
		System.loadLibrary("deqp");
	}

	private static final String	LOG_TAG							= "dEQP/PlatformCapabilityQueryInstrumentation";
	private static final int	CONFIGQUERYRESULT_SUPPORTED		= 0;
	private static final int	CONFIGQUERYRESULT_NOT_SUPPORTED	= 1;
	private static final int	CONFIGQUERYRESULT_GENERIC_ERROR	= -1;

	private String				m_cmdLine;
	private String				m_queryType;

	@Override
	public void onCreate (Bundle arguments) {
		super.onCreate(arguments);

		m_queryType = arguments.getString("deqpQueryType");
		m_cmdLine = arguments.getString("deqpCmdLine");

		start();
	}

	@Override
	public void onStart () {
		super.onStart();

		Bundle resultInfo;
		int resultCode = 0;

		try
		{
			if ("renderConfigSupported".equals(m_queryType))
				resultInfo = doRenderConfigSupportedQuery();
			else
			{
				resultInfo = new Bundle();
				resultInfo.putString("Error", "unknown query");
				resultInfo.putString("QueryType", m_queryType);
				resultInfo.putString("CmdLine", m_cmdLine);
				resultCode = 2;
			}
		}
		catch (Exception e)
		{
			resultInfo = new Bundle();
			resultInfo.putString("Error", e.getMessage());
			resultCode = 1;
		}

		finish(resultCode, resultInfo);
	}

	private Bundle doRenderConfigSupportedQuery ()
	{
		if (m_cmdLine == null)
			throw new RuntimeException("missing command line");

		final int result = nativeRenderConfigSupportedQuery(m_cmdLine);

		if (result == CONFIGQUERYRESULT_SUPPORTED)
		{
			final Bundle resultInfo = new Bundle();
			resultInfo.putString("Supported", "Yes");
			return resultInfo;
		}
		else if (result == CONFIGQUERYRESULT_NOT_SUPPORTED)
		{
			final Bundle resultInfo = new Bundle();
			resultInfo.putString("Supported", "No");
			return resultInfo;
		}
		else if (result == CONFIGQUERYRESULT_GENERIC_ERROR)
			throw new RuntimeException("platform query reported failure");
		else
			throw new RuntimeException("platform query returned out-of-range result");
	}

	private static native int nativeRenderConfigSupportedQuery (String cmdLine);
}
