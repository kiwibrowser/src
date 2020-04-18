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
 * \brief Log wrapper
 *//*--------------------------------------------------------------------*/

package com.drawelements.deqp.testercore;

public class Log {

	private static final boolean	LOG_DEBUG		= android.util.Log.isLoggable("dEQP", android.util.Log.DEBUG);
	private static final boolean	LOG_INFO		= true;
	private static final boolean	LOG_WARNING		= true;
	private static final boolean	LOG_ERROR		= true;

	public static void d (String tag, String msg) {
		if (LOG_DEBUG)
			android.util.Log.d(tag, msg);
	}

	public static void i (String tag, String msg) {
		if (LOG_INFO)
			android.util.Log.i(tag, msg);
	}

	public static void w (String tag, String msg) {
		if (LOG_WARNING)
			android.util.Log.w(tag, msg);
	}

	public static void w (String tag, String msg, Throwable tr) {
		if (LOG_WARNING)
			android.util.Log.w(tag, msg, tr);
	}

	public static void e (String tag, String msg) {
		if (LOG_ERROR)
			android.util.Log.e(tag, msg);
	}

	public static void e (String tag, String msg, Throwable tr) {
		if (LOG_ERROR)
			android.util.Log.e(tag, msg, tr);
	}
}
