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
 * \brief Background ExecServer starter
 *//*--------------------------------------------------------------------*/

package com.drawelements.deqp.execserver;

import android.app.Activity;
import android.os.Bundle;
import com.drawelements.deqp.testercore.Log;
import com.drawelements.deqp.execserver.ExecService;
import android.content.Intent;

public class ServiceStarter extends Activity {

	@Override
	public void onCreate(Bundle savedInstance) {
		super.onCreate(savedInstance);
	}

	@Override
	public void onStart() {
		super.onStart();

		final int port = getIntent().getIntExtra("port", ExecService.DEFAULT_PORT);

		try {
			Intent svc = new Intent(this, ExecService.class);
			svc.putExtra("port", port);
			startService(svc);
		}
		catch (Exception e) {
			Log.e(ExecService.LOG_TAG, "Failed to start ExecService", e);
		}
		finish();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}
}
