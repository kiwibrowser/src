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
 * \brief TesterCore remote interface.
 *//*--------------------------------------------------------------------*/

package com.drawelements.deqp.testercore;

import android.app.ActivityManager;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Process;

import java.util.List;

public class RemoteAPI {

	private static final String			LOG_TAG				= "dEQP";

	private Context						m_context;
	private String						m_processName;
	private String						m_logFileName;
	private boolean						m_canBeRunning;

	public RemoteAPI (Context context, String logFileName) {
		m_context			= context;
		m_processName		= m_context.getPackageName() + ":testercore";
		m_logFileName		= logFileName;
		m_canBeRunning		= false;
	}

	private ComponentName getDefaultTesterComponent () {
		return new ComponentName(m_context.getPackageName(), "android.app.NativeActivity");
	}

	private ComponentName getTesterComponent (String testerName) {
		if (testerName != null && !testerName.equals("")) {
			ComponentName component = ComponentName.unflattenFromString(testerName);
			if (component == null) {
				Log.e(LOG_TAG, "Invalid component name supplied (" + testerName + "), using default");
				component = getDefaultTesterComponent();
			}
			return component;
		} else {
			return getDefaultTesterComponent();
		}
	}

	public boolean start (String testerName, String cmdLine, String caseList) {

		// Choose component
		ComponentName component = getTesterComponent(testerName);

		Intent testIntent = new Intent();
		testIntent.setComponent(component);
		testIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		// Add all data to cmdLine
		cmdLine = testerName + " " + cmdLine + " --deqp-log-filename=" + m_logFileName;

		if (caseList != null)
			cmdLine = cmdLine + " --deqp-caselist=" + caseList;

		cmdLine = cmdLine.replaceAll("  ", " ");
		testIntent.putExtra("cmdLine", cmdLine);

		// Try to resolve intent.
		boolean	isActivity	= m_context.getPackageManager().resolveActivity(testIntent, 0) != null;
		boolean isService	= m_context.getPackageManager().resolveService(testIntent, 0) != null;

		if (!isActivity && !isService) {
			Log.e(LOG_TAG, "Can't resolve component as activity or service (" + component.flattenToString() + "), using default");
			component = getDefaultTesterComponent();
		}

		Log.d(LOG_TAG, "Starting activity " + component.flattenToString());

		try {
			if (isService)
				m_context.startService(testIntent);
			else
				m_context.startActivity(testIntent);
		} catch (Exception e) {
			Log.e(LOG_TAG, "Failed to start tester", e);
			return false;
		}

		m_canBeRunning = true;
		return true;
	}

	public boolean kill () {
		ActivityManager.RunningAppProcessInfo processInfo = findProcess(m_processName);

		// \note not mutating m_canBeRunning yet since process does not die immediately

		if (processInfo != null) {
			Log.d(LOG_TAG, "Killing " + m_processName);
			Process.killProcess(processInfo.pid);
			return true;
		} else {
			return false;
		}

		// \todo [2010-11-01 pyry] Block until tester process is not running?
	}

	public boolean isRunning () {
		if (!m_canBeRunning) {
			return false;
		} else if (isProcessRunning(m_processName)) {
			return true;
		} else {
			// Cache result. Safe, because only start() can spawn the process
			m_canBeRunning = false;
			return false;
		}
	}

	public String getLogFileName () {
		return m_logFileName;
	}

	private ActivityManager.RunningAppProcessInfo findProcess (String name) {
		ActivityManager activityMgr = (ActivityManager)m_context.getSystemService(Context.ACTIVITY_SERVICE);
		List<ActivityManager.RunningAppProcessInfo> processes = activityMgr.getRunningAppProcesses();

		for (ActivityManager.RunningAppProcessInfo info : processes) {
			// Log.v(LOG_TAG, "Found proc : " + info.processName + " " + info.pid);
			if (info.processName.equals(name))
				return info;
		}

		return null;
	}

	private boolean isProcessRunning (String processName) {
		// Log.d(LOG_TAG, "isProcessRunning(): " + processName);
		return (findProcess(processName) != null);
	}
}
