// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "SwiftConfig.hpp"

#include "Config.hpp"
#include "Common/Configurator.hpp"
#include "Common/Debug.hpp"
#include "Common/Version.h"

#include <sstream>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <algorithm>

namespace sw
{
	extern Profiler profiler;

	std::string itoa(int number)
	{
		std::stringstream ss;
		ss << number;
		return ss.str();
	}

	std::string ftoa(double number)
	{
		std::stringstream ss;
		ss << number;
		return ss.str();
	}

	SwiftConfig::SwiftConfig(bool disableServerOverride) : listenSocket(0)
	{
		readConfiguration(disableServerOverride);

		if(!disableServerOverride)
		{
			writeConfiguration();
		}

		receiveBuffer = 0;

		if(!config.disableServer)
		{
			createServer();
		}
	}

	SwiftConfig::~SwiftConfig()
	{
		destroyServer();
	}

	void SwiftConfig::createServer()
	{
		bufferLength = 16 * 1024;
		receiveBuffer = new char[bufferLength];

		Socket::startup();
		listenSocket = new Socket("localhost", "8080");
		listenSocket->listen();

		terminate = false;
		serverThread = new Thread(serverRoutine, this);
	}

	void SwiftConfig::destroyServer()
	{
		if(receiveBuffer)
		{
			terminate = true;
			serverThread->join();
			delete serverThread;

			delete listenSocket;
			listenSocket = 0;

			Socket::cleanup();

			delete[] receiveBuffer;
			receiveBuffer = 0;
		}
	}

	bool SwiftConfig::hasNewConfiguration(bool reset)
	{
		bool value = newConfig;

		if(reset)
		{
			newConfig = false;
		}

		return value;
	}

	void SwiftConfig::getConfiguration(Configuration &configuration)
	{
		criticalSection.lock();
		configuration = config;
		criticalSection.unlock();
	}

	void SwiftConfig::serverRoutine(void *parameters)
	{
		SwiftConfig *swiftConfig = (SwiftConfig*)parameters;

		swiftConfig->serverLoop();
	}

	void SwiftConfig::serverLoop()
	{
		readConfiguration();

		while(!terminate)
		{
			if(listenSocket->select(100000))
			{
				Socket *clientSocket = listenSocket->accept();
				int bytesReceived = 1;

				while(bytesReceived > 0 && !terminate)
				{
					if(clientSocket->select(10))
					{
						bytesReceived = clientSocket->receive(receiveBuffer, bufferLength);

						if(bytesReceived > 0)
						{
							receiveBuffer[bytesReceived] = 0;

							respond(clientSocket, receiveBuffer);
						}
					}
				}

				delete clientSocket;
			}
		}
	}

	bool match(const char **url, const char *string)
	{
		size_t length = strlen(string);

		if(strncmp(*url, string, length) == 0)
		{
			*url += length;

			return true;
		}

		return false;
	}

	void SwiftConfig::respond(Socket *clientSocket, const char *request)
	{
		if(match(&request, "GET /"))
		{
			if(match(&request, "swiftshader") || match(&request, "swiftconfig"))
			{
				if(match(&request, " ") || match(&request, "/ "))
				{
					return send(clientSocket, OK, page());
				}
			}
		}
		else if(match(&request, "POST /"))
		{
			if(match(&request, "swiftshader") || match(&request, "swiftconfig"))
			{
				if(match(&request, " ") || match(&request, "/ "))
				{
					criticalSection.lock();

					const char *postData = strstr(request, "\r\n\r\n");
					postData = postData ? postData + 4 : 0;

					if(postData && strlen(postData) > 0)
					{
						parsePost(postData);
					}
					else   // POST data in next packet
					{
						int bytesReceived = clientSocket->receive(receiveBuffer, bufferLength);

						if(bytesReceived > 0)
						{
							receiveBuffer[bytesReceived] = 0;
							parsePost(receiveBuffer);
						}
					}

					writeConfiguration();
					newConfig = true;

					if(config.disableServer)
					{
						destroyServer();
					}

					criticalSection.unlock();

					return send(clientSocket, OK, page());
				}
				else if(match(&request, "/profile "))
				{
					return send(clientSocket, OK, profile());
				}
			}
		}

		return send(clientSocket, NotFound);
	}

	std::string SwiftConfig::page()
	{
		std::string html;

		const std::string selected = "selected='selected'";
		const std::string checked = "checked='checked'";
		const std::string empty = "";

		html += "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
		html += "<html>\n";
		html += "<head>\n";
		html += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>\n";
		html += "<title>SwiftShader Configuration Panel</title>\n";
		html += "</head>\n";
		html += "<body>\n";
		html += "<script type='text/javascript'>\n";
		html += "request();\n";
		html += "function request()\n";
		html += "{\n";
		html += "var xhr = new XMLHttpRequest();\n";
		html += "xhr.open('POST', '/swiftshader/profile', true);\n";
		html += "xhr.onreadystatechange = function()\n";
		html += "{\n";
		html += "if(xhr.readyState == 4 && xhr.status == 200)\n";
		html += "{\n";
		html += "document.getElementById('profile').innerHTML = xhr.responseText;\n";
		html += "setTimeout('request()', 1000);\n";
		html += "}\n";
		html += "}\n";
		html += "xhr.send();\n";
		html += "}\n";
		html += "</script>\n";
		html += "<form method='POST' action=''>\n";
		html += "<h1>SwiftShader Configuration Panel</h1>\n";
		html += "<div id='profile'>" + profile() + "</div>\n";
		html += "<hr><p>\n";
		html += "<input type='submit' value='Apply changes' title='Click to apply all settings.'>\n";
	//	html += "<input type='reset' value='Reset changes' title='Click to reset your changes to the previous value.'>\n";
		html += "</p><hr>\n";
		html += "<h2><em>Device capabilities</em></h2>\n";
		html += "<table>\n";
		html += "<tr><td>Build revision:</td><td>" REVISION_STRING "</td></tr>\n";
		html += "<tr><td>Pixel shader model:</td><td><select name='pixelShaderVersion' title='The highest version of pixel shader supported by SwiftShader. Lower versions might be faster if supported by the application. Only effective after restarting the application.'>\n";
		html += "<option value='0'"  + (config.pixelShaderVersion ==  0 ? selected : empty) + ">0.0</option>\n";
		html += "<option value='11'" + (config.pixelShaderVersion == 11 ? selected : empty) + ">1.1</option>\n";
		html += "<option value='12'" + (config.pixelShaderVersion == 12 ? selected : empty) + ">1.2</option>\n";
		html += "<option value='13'" + (config.pixelShaderVersion == 13 ? selected : empty) + ">1.3</option>\n";
		html += "<option value='14'" + (config.pixelShaderVersion == 14 ? selected : empty) + ">1.4</option>\n";
		html += "<option value='20'" + (config.pixelShaderVersion == 20 ? selected : empty) + ">2.0</option>\n";
		html += "<option value='21'" + (config.pixelShaderVersion == 21 ? selected : empty) + ">2.x</option>\n";
		html += "<option value='30'" + (config.pixelShaderVersion == 30 ? selected : empty) + ">3.0 (default)</option>\n";
		html += "</select></td></tr>\n";
		html += "<tr><td>Vertex shader model:</td><td><select name='vertexShaderVersion' title='The highest version of vertex shader supported by SwiftShader. Lower versions might be faster if supported by the application. Only effective after restarting the application.'>\n";
		html += "<option value='0'"  + (config.vertexShaderVersion ==  0 ? selected : empty) + ">0.0</option>\n";
		html += "<option value='11'" + (config.vertexShaderVersion == 11 ? selected : empty) + ">1.1</option>\n";
		html += "<option value='20'" + (config.vertexShaderVersion == 20 ? selected : empty) + ">2.0</option>\n";
		html += "<option value='21'" + (config.vertexShaderVersion == 21 ? selected : empty) + ">2.x</option>\n";
		html += "<option value='30'" + (config.vertexShaderVersion == 30 ? selected : empty) + ">3.0 (default)</option>\n";
		html += "</select></td></tr>\n";
		html += "<tr><td>Texture memory:</td><td><select name='textureMemory' title='The maximum amount of memory used for textures and other resources.'>\n";
		html += "<option value='128'"  + (config.textureMemory == 128  ? selected : empty) + ">128 MB</option>\n";
		html += "<option value='256'"  + (config.textureMemory == 256  ? selected : empty) + ">256 MB (default)</option>\n";
		html += "<option value='512'"  + (config.textureMemory == 512  ? selected : empty) + ">512 MB</option>\n";
		html += "<option value='1024'" + (config.textureMemory == 1024 ? selected : empty) + ">1024 MB</option>\n";
		html += "<option value='2048'" + (config.textureMemory == 2048 ? selected : empty) + ">2048 MB</option>\n";
		html += "</select></td></tr>\n";
		html += "<tr><td>Device identifier:</td><td><select name='identifier' title='The information used by some applications to determine device capabilities.'>\n";
		html += "<option value='0'" + (config.identifier == 0 ? selected : empty) + ">Google SwiftShader (default)</option>\n";
		html += "<option value='1'" + (config.identifier == 1 ? selected : empty) + ">NVIDIA GeForce 7900 GS</option>\n";
		html += "<option value='2'" + (config.identifier == 2 ? selected : empty) + ">ATI Mobility Radeon X1600</option>\n";
		html += "<option value='3'" + (config.identifier == 3 ? selected : empty) + ">Intel GMA X3100</option>\n";
		html += "<option value='4'" + (config.identifier == 4 ? selected : empty) + ">System device</option>\n";
		html += "</select></td></tr>\n";
		html += "</table>\n";
		html += "<h2><em>Cache sizes</em></h2>\n";
		html += "<table>\n";
		html += "<tr><td>Vertex routine cache size:</td><td><select name='vertexRoutineCacheSize' title='The number of dynamically generated vertex processing routines being cached for reuse. Lower numbers save memory but require more routines to be regenerated.'>\n";
		html += "<option value='64'"   + (config.vertexRoutineCacheSize == 64   ? selected : empty) + ">64</option>\n";
		html += "<option value='128'"  + (config.vertexRoutineCacheSize == 128  ? selected : empty) + ">128</option>\n";
		html += "<option value='256'"  + (config.vertexRoutineCacheSize == 256  ? selected : empty) + ">256</option>\n";
		html += "<option value='512'"  + (config.vertexRoutineCacheSize == 512  ? selected : empty) + ">512</option>\n";
		html += "<option value='1024'" + (config.vertexRoutineCacheSize == 1024 ? selected : empty) + ">1024 (default)</option>\n";
		html += "<option value='2048'" + (config.vertexRoutineCacheSize == 2048 ? selected : empty) + ">2048</option>\n";
		html += "<option value='4096'" + (config.vertexRoutineCacheSize == 4096 ? selected : empty) + ">4096</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Pixel routine cache size:</td><td><select name='pixelRoutineCacheSize' title='The number of dynamically generated pixel processing routines being cached for reuse. Lower numbers save memory but require more routines to be regenerated.'>\n";
		html += "<option value='64'"   + (config.pixelRoutineCacheSize == 64   ? selected : empty) + ">64</option>\n";
		html += "<option value='128'"  + (config.pixelRoutineCacheSize == 128  ? selected : empty) + ">128</option>\n";
		html += "<option value='256'"  + (config.pixelRoutineCacheSize == 256  ? selected : empty) + ">256</option>\n";
		html += "<option value='512'"  + (config.pixelRoutineCacheSize == 512  ? selected : empty) + ">512</option>\n";
		html += "<option value='1024'" + (config.pixelRoutineCacheSize == 1024 ? selected : empty) + ">1024 (default)</option>\n";
		html += "<option value='2048'" + (config.pixelRoutineCacheSize == 2048 ? selected : empty) + ">2048</option>\n";
		html += "<option value='4096'" + (config.pixelRoutineCacheSize == 4096 ? selected : empty) + ">4096</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Setup routine cache size:</td><td><select name='setupRoutineCacheSize' title='The number of dynamically generated primitive setup routines being cached for reuse. Lower numbers save memory but require more routines to be regenerated.'>\n";
		html += "<option value='64'"   + (config.setupRoutineCacheSize == 64   ? selected : empty) + ">64</option>\n";
		html += "<option value='128'"  + (config.setupRoutineCacheSize == 128  ? selected : empty) + ">128</option>\n";
		html += "<option value='256'"  + (config.setupRoutineCacheSize == 256  ? selected : empty) + ">256</option>\n";
		html += "<option value='512'"  + (config.setupRoutineCacheSize == 512  ? selected : empty) + ">512</option>\n";
		html += "<option value='1024'" + (config.setupRoutineCacheSize == 1024 ? selected : empty) + ">1024 (default)</option>\n";
		html += "<option value='2048'" + (config.setupRoutineCacheSize == 2048 ? selected : empty) + ">2048</option>\n";
		html += "<option value='4096'" + (config.setupRoutineCacheSize == 4096 ? selected : empty) + ">4096</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Vertex cache size:</td><td><select name='vertexCacheSize' title='The number of processed vertices being cached for reuse. Lower numbers save memory but require more vertices to be reprocessed.'>\n";
		html += "<option value='64'"   + (config.vertexCacheSize == 64   ? selected : empty) + ">64 (default)</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "</table>\n";
		html += "<h2><em>Quality</em></h2>\n";
		html += "<table>\n";
		html += "<tr><td>Maximum texture sampling quality:</td><td><select name='textureSampleQuality' title='The maximum texture filtering quality. Lower settings can be faster but cause visual artifacts.'>\n";
		html += "<option value='0'" + (config.textureSampleQuality == 0 ? selected : empty) + ">Point</option>\n";
		html += "<option value='1'" + (config.textureSampleQuality == 1 ? selected : empty) + ">Linear</option>\n";
		html += "<option value='2'" + (config.textureSampleQuality == 2 ? selected : empty) + ">Anisotropic (default)</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Maximum mipmapping quality:</td><td><select name='mipmapQuality' title='The maximum mipmap filtering quality. Higher settings can be more visually appealing but are slower.'>\n";
		html += "<option value='0'" + (config.mipmapQuality == 0 ? selected : empty) + ">Point</option>\n";
		html += "<option value='1'" + (config.mipmapQuality == 1 ? selected : empty) + ">Linear (default)</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Perspective correction:</td><td><select name='perspectiveCorrection' title='Enables or disables perspective correction. Disabling it is faster but can causes distortion. Recommended for 2D applications only.'>\n";
		html += "<option value='0'" + (config.perspectiveCorrection == 0 ? selected : empty) + ">Off</option>\n";
		html += "<option value='1'" + (config.perspectiveCorrection == 1 ? selected : empty) + ">On (default)</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Transcendental function precision:</td><td><select name='transcendentalPrecision' title='The precision at which log/exp/pow/rcp/rsq/nrm shader instructions are computed. Lower settings can be faster but cause visual artifacts.'>\n";
		html += "<option value='0'" + (config.transcendentalPrecision == 0 ? selected : empty) + ">Approximate</option>\n";
		html += "<option value='1'" + (config.transcendentalPrecision == 1 ? selected : empty) + ">Partial</option>\n";
		html += "<option value='2'" + (config.transcendentalPrecision == 2 ? selected : empty) + ">Accurate (default)</option>\n";
		html += "<option value='3'" + (config.transcendentalPrecision == 3 ? selected : empty) + ">WHQL</option>\n";
		html += "<option value='4'" + (config.transcendentalPrecision == 4 ? selected : empty) + ">IEEE</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td>Transparency anti-aliasing:</td><td><select name='transparencyAntialiasing' title='The technique used to anti-alias alpha-tested transparent textures.'>\n";
		html += "<option value='0'" + (config.transparencyAntialiasing == 0 ? selected : empty) + ">None (default)</option>\n";
		html += "<option value='1'" + (config.transparencyAntialiasing == 1 ? selected : empty) + ">Alpha-to-Coverage</option>\n";
		html += "</select></td>\n";
		html += "</table>\n";
		html += "<h2><em>Processor settings</em></h2>\n";
		html += "<table>\n";
		html += "<tr><td>Number of threads:</td><td><select name='threadCount' title='The number of rendering threads to be used.'>\n";
		html += "<option value='-1'" + (config.threadCount == -1 ? selected : empty) + ">Core count</option>\n";
		html += "<option value='0'"  + (config.threadCount == 0  ? selected : empty) + ">Process affinity (default)</option>\n";
		html += "<option value='1'"  + (config.threadCount == 1  ? selected : empty) + ">1</option>\n";
		html += "<option value='2'"  + (config.threadCount == 2  ? selected : empty) + ">2</option>\n";
		html += "<option value='3'"  + (config.threadCount == 3  ? selected : empty) + ">3</option>\n";
		html += "<option value='4'"  + (config.threadCount == 4  ? selected : empty) + ">4</option>\n";
		html += "<option value='5'"  + (config.threadCount == 5  ? selected : empty) + ">5</option>\n";
		html += "<option value='6'"  + (config.threadCount == 6  ? selected : empty) + ">6</option>\n";
		html += "<option value='7'"  + (config.threadCount == 7  ? selected : empty) + ">7</option>\n";
		html += "<option value='8'"  + (config.threadCount == 8  ? selected : empty) + ">8</option>\n";
		html += "<option value='9'"  + (config.threadCount == 9  ? selected : empty) + ">9</option>\n";
		html += "<option value='10'" + (config.threadCount == 10 ? selected : empty) + ">10</option>\n";
		html += "<option value='11'" + (config.threadCount == 11 ? selected : empty) + ">11</option>\n";
		html += "<option value='12'" + (config.threadCount == 12 ? selected : empty) + ">12</option>\n";
		html += "<option value='13'" + (config.threadCount == 13 ? selected : empty) + ">13</option>\n";
		html += "<option value='14'" + (config.threadCount == 14 ? selected : empty) + ">14</option>\n";
		html += "<option value='15'" + (config.threadCount == 15 ? selected : empty) + ">15</option>\n";
		html += "<option value='16'" + (config.threadCount == 16 ? selected : empty) + ">16</option>\n";
		html += "</select></td></tr>\n";
		html += "<tr><td>Enable SSE:</td><td><input name = 'enableSSE' type='checkbox'" + (config.enableSSE ? checked : empty) + " disabled='disabled' title='If checked enables the use of SSE instruction set extentions if supported by the CPU.'></td></tr>";
		html += "<tr><td>Enable SSE2:</td><td><input name = 'enableSSE2' type='checkbox'" + (config.enableSSE2 ? checked : empty) + " title='If checked enables the use of SSE2 instruction set extentions if supported by the CPU.'></td></tr>";
		html += "<tr><td>Enable SSE3:</td><td><input name = 'enableSSE3' type='checkbox'" + (config.enableSSE3 ? checked : empty) + " title='If checked enables the use of SSE3 instruction set extentions if supported by the CPU.'></td></tr>";
		html += "<tr><td>Enable SSSE3:</td><td><input name = 'enableSSSE3' type='checkbox'" + (config.enableSSSE3 ? checked : empty) + " title='If checked enables the use of SSSE3 instruction set extentions if supported by the CPU.'></td></tr>";
		html += "<tr><td>Enable SSE4.1:</td><td><input name = 'enableSSE4_1' type='checkbox'" + (config.enableSSE4_1 ? checked : empty) + " title='If checked enables the use of SSE4.1 instruction set extentions if supported by the CPU.'></td></tr>";
		html += "</table>\n";
		html += "<h2><em>Compiler optimizations</em></h2>\n";
		html += "<table>\n";

		for(int pass = 0; pass < 10; pass++)
		{
			html += "<tr><td>Optimization pass " + itoa(pass + 1) + ":</td><td><select name='optimization" + itoa(pass + 1) + "' title='An optimization pass for the shader compiler.'>\n";
			html += "<option value='0'"  + (config.optimization[pass] == 0  ? selected : empty) + ">Disabled" + (pass > 0 ? " (default)" : "") + "</option>\n";
			html += "<option value='1'"  + (config.optimization[pass] == 1  ? selected : empty) + ">Instruction Combining" + (pass == 0 ? " (default)" : "") + "</option>\n";
			html += "<option value='2'"  + (config.optimization[pass] == 2  ? selected : empty) + ">Control Flow Simplification</option>\n";
			html += "<option value='3'"  + (config.optimization[pass] == 3  ? selected : empty) + ">Loop Invariant Code Motion</option>\n";
			html += "<option value='4'"  + (config.optimization[pass] == 4  ? selected : empty) + ">Aggressive Dead Code Elimination</option>\n";
			html += "<option value='5'"  + (config.optimization[pass] == 5  ? selected : empty) + ">Global Value Numbering</option>\n";
			html += "<option value='6'"  + (config.optimization[pass] == 6  ? selected : empty) + ">Commutative Expressions Reassociation</option>\n";
			html += "<option value='7'"  + (config.optimization[pass] == 7  ? selected : empty) + ">Dead Store Elimination</option>\n";
			html += "<option value='8'"  + (config.optimization[pass] == 8  ? selected : empty) + ">Sparse Conditional Copy Propagation</option>\n";
			html += "<option value='9'"  + (config.optimization[pass] == 9  ? selected : empty) + ">Scalar Replacement of Aggregates</option>\n";
			html += "</select></td></tr>\n";
		}

		html += "</table>\n";
		html += "<h2><em>Testing & Experimental</em></h2>\n";
		html += "<table>\n";
		html += "<tr><td>Disable SwiftConfig server:</td><td><input name = 'disableServer' type='checkbox'" + (config.disableServer == true ? checked : empty) + " title='If checked disables the web browser based control panel.'></td></tr>";
		html += "<tr><td>Force windowed mode:</td><td><input name = 'forceWindowed' type='checkbox'" + (config.forceWindowed == true ? checked : empty) + " title='If checked prevents the application from switching to full-screen mode.'></td></tr>";
		html += "<tr><td>Complementary depth buffer:</td><td><input name = 'complementaryDepthBuffer' type='checkbox'" + (config.complementaryDepthBuffer == true ? checked : empty) + " title='If checked causes 1 - z to be stored in the depth buffer.'></td></tr>";
		html += "<tr><td>Post alpha blend sRGB conversion:</td><td><input name = 'postBlendSRGB' type='checkbox'" + (config.postBlendSRGB == true ? checked : empty) + " title='If checked alpha blending is performed in linear color space.'></td></tr>";
		html += "<tr><td>Exact color rounding:</td><td><input name = 'exactColorRounding' type='checkbox'" + (config.exactColorRounding == true ? checked : empty) + " title='If checked color rounding is done at high accuracy.'></td></tr>";
		html += "<tr><td>Disable alpha display formats:</td><td><input name = 'disableAlphaMode' type='checkbox'" + (config.disableAlphaMode == true ? checked : empty) + " title='If checked the device does not advertise the A8R8G8B8 display mode.'></td></tr>";
		html += "<tr><td>Disable 10-bit display formats:</td><td><input name = 'disable10BitMode' type='checkbox'" + (config.disable10BitMode == true ? checked : empty) + " title='If checked the device does not advertise the A2R10G10B10 display mode.'></td></tr>";
		html += "<tr><td>Frame-buffer API:</td><td><select name='frameBufferAPI' title='The API used for displaying the rendered result on screen (requires restart).'>\n";
		html += "<option value='0'" + (config.frameBufferAPI == 0 ? selected : empty) + ">DirectDraw (default)</option>\n";
		html += "<option value='1'" + (config.frameBufferAPI == 1 ? selected : empty) + ">GDI</option>\n";
		html += "</select></td>\n";
		html += "<tr><td>DLL precaching:</td><td><input name = 'precache' type='checkbox'" + (config.precache == true ? checked : empty) + " title='If checked dynamically generated routines will be stored in a DLL for faster loading on application restart.'></td></tr>";
		html += "<tr><td>Shadow mapping extensions:</td><td><select name='shadowMapping' title='Features that may accelerate or improve the quality of shadow mapping.'>\n";
		html += "<option value='0'" + (config.shadowMapping == 0 ? selected : empty) + ">None</option>\n";
		html += "<option value='1'" + (config.shadowMapping == 1 ? selected : empty) + ">Fetch4</option>\n";
		html += "<option value='2'" + (config.shadowMapping == 2 ? selected : empty) + ">DST</option>\n";
		html += "<option value='3'" + (config.shadowMapping == 3 ? selected : empty) + ">Fetch4 & DST (default)</option>\n";
		html += "</select></td>\n";
		html += "<tr><td>Force clearing registers that have no default value:</td><td><input name = 'forceClearRegisters' type='checkbox'" + (config.forceClearRegisters == true ? checked : empty) + " title='Initializes shader register values to 0 even if they have no default.'></td></tr>";
		html += "</table>\n";
	#ifndef NDEBUG
		html += "<h2><em>Debugging</em></h2>\n";
		html += "<table>\n";
		html += "<tr><td>Minimum primitives:</td><td><input type='text' size='10' maxlength='10' name='minPrimitives' value='" + itoa(config.minPrimitives) + "'></td></tr>\n";
		html += "<tr><td>Maximum primitives:</td><td><input type='text' size='10' maxlength='10' name='maxPrimitives' value='" + itoa(config.maxPrimitives) + "'></td></tr>\n";
		html += "</table>\n";
	#endif
		html += "<hr><p>\n";
		html += "<span style='font-size:10pt'>Hover the mouse pointer over a control to get additional information.</span><br>\n";
		html += "<span style='font-size:10pt'>Some settings can be applied interactively, some need a restart of the application.</span><br>\n";
		html += "<span style='font-size:10pt'>Removing the SwiftShader.ini file results in resetting the options to their default.</span></p>\n";
		html += "</form>\n";
		html += "</body>\n";
		html += "</html>\n";

		profiler.reset();

		return html;
	}

	std::string SwiftConfig::profile()
	{
		std::string html;

		html += "<p>FPS: " + ftoa(profiler.FPS) + "</p>\n";
		html += "<p>Frame: " + itoa(profiler.framesTotal) + "</p>\n";

		#if PERF_PROFILE
			int texTime = (int)(1000 * profiler.cycles[PERF_TEX] / profiler.cycles[PERF_PIXEL] + 0.5);
			int shaderTime = (int)(1000 * profiler.cycles[PERF_SHADER] / profiler.cycles[PERF_PIXEL] + 0.5);
			int pipeTime = (int)(1000 * profiler.cycles[PERF_PIPE] / profiler.cycles[PERF_PIXEL] + 0.5);
			int ropTime = (int)(1000 * profiler.cycles[PERF_ROP] / profiler.cycles[PERF_PIXEL] + 0.5);
			int interpTime = (int)(1000 * profiler.cycles[PERF_INTERP] / profiler.cycles[PERF_PIXEL] + 0.5);
			int rastTime = 1000 - pipeTime;

			pipeTime -= shaderTime + ropTime + interpTime;
			shaderTime -= texTime;

			double texTimeF = (double)texTime / 10;
			double shaderTimeF = (double)shaderTime / 10;
			double pipeTimeF = (double)pipeTime / 10;
			double ropTimeF = (double)ropTime / 10;
			double interpTimeF = (double)interpTime / 10;
			double rastTimeF = (double)rastTime / 10;

			double averageRopOperations = profiler.ropOperationsTotal / std::max(profiler.framesTotal, 1) / 1.0e6f;
			double averageCompressedTex = profiler.compressedTexTotal / std::max(profiler.framesTotal, 1) / 1.0e6f;
			double averageTexOperations = profiler.texOperationsTotal / std::max(profiler.framesTotal, 1) / 1.0e6f;

			html += "<p>Raster operations (million): " + ftoa(profiler.ropOperationsFrame / 1.0e6f) + " (current), " + ftoa(averageRopOperations) + " (average)</p>\n";
			html += "<p>Texture operations (million): " + ftoa(profiler.texOperationsFrame / 1.0e6f) + " (current), " + ftoa(averageTexOperations) + " (average)</p>\n";
			html += "<p>Compressed texture operations (million): " + ftoa(profiler.compressedTexFrame / 1.0e6f) + " (current), " + ftoa(averageCompressedTex) + " (average)</p>\n";
			html += "<div id='profile' style='position:relative; width:1010px; height:50px; background-color:silver;'>";
			html += "<div style='position:relative; width:1000px; height:40px; background-color:white; left:5px; top:5px;'>";
			html += "<div style='position:relative; float:left; width:" + itoa(rastTime)   + "px; height:40px; border-style:none; text-align:center; line-height:40px; background-color:#FFFF7F; overflow:hidden;'>" + ftoa(rastTimeF)   + "% rast</div>\n";
			html += "<div style='position:relative; float:left; width:" + itoa(pipeTime)   + "px; height:40px; border-style:none; text-align:center; line-height:40px; background-color:#FF7F7F; overflow:hidden;'>" + ftoa(pipeTimeF)   + "% pipe</div>\n";
			html += "<div style='position:relative; float:left; width:" + itoa(interpTime) + "px; height:40px; border-style:none; text-align:center; line-height:40px; background-color:#7FFFFF; overflow:hidden;'>" + ftoa(interpTimeF) + "% interp</div>\n";
			html += "<div style='position:relative; float:left; width:" + itoa(shaderTime) + "px; height:40px; border-style:none; text-align:center; line-height:40px; background-color:#7FFF7F; overflow:hidden;'>" + ftoa(shaderTimeF) + "% shader</div>\n";
			html += "<div style='position:relative; float:left; width:" + itoa(texTime)    + "px; height:40px; border-style:none; text-align:center; line-height:40px; background-color:#FF7FFF; overflow:hidden;'>" + ftoa(texTimeF)    + "% tex</div>\n";
			html += "<div style='position:relative; float:left; width:" + itoa(ropTime)    + "px; height:40px; border-style:none; text-align:center; line-height:40px; background-color:#7F7FFF; overflow:hidden;'>" + ftoa(ropTimeF)    + "% rop</div>\n";
			html += "</div></div>\n";

			for(int i = 0; i < PERF_TIMERS; i++)
			{
				profiler.cycles[i] = 0;
			}
		#endif

		return html;
	}

	void SwiftConfig::send(Socket *clientSocket, Status code, std::string body)
	{
		std::string status;
		char header[1024];

		switch(code)
		{
		case OK:       status += "HTTP/1.1 200 OK\r\n";        break;
		case NotFound: status += "HTTP/1.1 404 Not Found\r\n"; break;
		}

		sprintf(header, "Content-Type: text/html; charset=UTF-8\r\n"
						"Content-Length: %zd\r\n"
						"Host: localhost\r\n"
						"\r\n", body.size());

		std::string message = status + header + body;
		clientSocket->send(message.c_str(), (int)message.length());
	}

	void SwiftConfig::parsePost(const char *post)
	{
		// Only enabled checkboxes appear in the POST
		config.enableSSE = true;
		config.enableSSE2 = false;
		config.enableSSE3 = false;
		config.enableSSSE3 = false;
		config.enableSSE4_1 = false;
		config.disableServer = false;
		config.forceWindowed = false;
		config.complementaryDepthBuffer = false;
		config.postBlendSRGB = false;
		config.exactColorRounding = false;
		config.disableAlphaMode = false;
		config.disable10BitMode = false;
		config.precache = false;
		config.forceClearRegisters = false;

		while(*post != 0)
		{
			int integer;
			int index;

			if(sscanf(post, "pixelShaderVersion=%d", &integer))
			{
				config.pixelShaderVersion = integer;
			}
			else if(sscanf(post, "vertexShaderVersion=%d", &integer))
			{
				config.vertexShaderVersion = integer;
			}
			else if(sscanf(post, "textureMemory=%d", &integer))
			{
				config.textureMemory = integer;
			}
			else if(sscanf(post, "identifier=%d", &integer))
			{
				config.identifier = integer;
			}
			else if(sscanf(post, "vertexRoutineCacheSize=%d", &integer))
			{
				config.vertexRoutineCacheSize = integer;
			}
			else if(sscanf(post, "pixelRoutineCacheSize=%d", &integer))
			{
				config.pixelRoutineCacheSize = integer;
			}
			else if(sscanf(post, "setupRoutineCacheSize=%d", &integer))
			{
				config.setupRoutineCacheSize = integer;
			}
			else if(sscanf(post, "vertexCacheSize=%d", &integer))
			{
				config.vertexCacheSize = integer;
			}
			else if(sscanf(post, "textureSampleQuality=%d", &integer))
			{
				config.textureSampleQuality = integer;
			}
			else if(sscanf(post, "mipmapQuality=%d", &integer))
			{
				config.mipmapQuality = integer;
			}
			else if(sscanf(post, "perspectiveCorrection=%d", &integer))
			{
				config.perspectiveCorrection = integer != 0;
			}
			else if(sscanf(post, "transcendentalPrecision=%d", &integer))
			{
				config.transcendentalPrecision = integer;
			}
			else if(sscanf(post, "transparencyAntialiasing=%d", &integer))
			{
				config.transparencyAntialiasing = integer;
			}
			else if(sscanf(post, "threadCount=%d", &integer))
			{
				config.threadCount = integer;
			}
			else if(sscanf(post, "frameBufferAPI=%d", &integer))
			{
				config.frameBufferAPI = integer;
			}
			else if(sscanf(post, "shadowMapping=%d", &integer))
			{
				config.shadowMapping = integer;
			}
			else if(strstr(post, "enableSSE=on"))
			{
				config.enableSSE = true;
			}
			else if(strstr(post, "enableSSE2=on"))
			{
				if(config.enableSSE)
				{
					config.enableSSE2 = true;
				}
			}
			else if(strstr(post, "enableSSE3=on"))
			{
				if(config.enableSSE2)
				{
					config.enableSSE3 = true;
				}
			}
			else if(strstr(post, "enableSSSE3=on"))
			{
				if(config.enableSSE3)
				{
					config.enableSSSE3 = true;
				}
			}
			else if(strstr(post, "enableSSE4_1=on"))
			{
				if(config.enableSSSE3)
				{
					config.enableSSE4_1 = true;
				}
			}
			else if(sscanf(post, "optimization%d=%d", &index, &integer))
			{
				config.optimization[index - 1] = (Optimization)integer;
			}
			else if(strstr(post, "disableServer=on"))
			{
				config.disableServer = true;
			}
			else if(strstr(post, "forceWindowed=on"))
			{
				config.forceWindowed = true;
			}
			else if(strstr(post, "complementaryDepthBuffer=on"))
			{
				config.complementaryDepthBuffer = true;
			}
			else if(strstr(post, "postBlendSRGB=on"))
			{
				config.postBlendSRGB = true;
			}
			else if(strstr(post, "exactColorRounding=on"))
			{
				config.exactColorRounding = true;
			}
			else if(strstr(post, "disableAlphaMode=on"))
			{
				config.disableAlphaMode = true;
			}
			else if(strstr(post, "disable10BitMode=on"))
			{
				config.disable10BitMode = true;
			}
			else if(strstr(post, "precache=on"))
			{
				config.precache = true;
			}
			else if(strstr(post, "forceClearRegisters=on"))
			{
				config.forceClearRegisters = true;
			}
		#ifndef NDEBUG
			else if(sscanf(post, "minPrimitives=%d", &integer))
			{
				config.minPrimitives = integer;
			}
			else if(sscanf(post, "maxPrimitives=%d", &integer))
			{
				config.maxPrimitives = integer;
			}
		#endif
			else
			{
				ASSERT(false);
			}

			do
			{
				post++;
			}
			while(post[-1] != '&' && *post != 0);
		}
	}

	void SwiftConfig::readConfiguration(bool disableServerOverride)
	{
		Configurator ini("SwiftShader.ini");

		config.pixelShaderVersion = ini.getInteger("Capabilities", "PixelShaderVersion", 30);
		config.vertexShaderVersion = ini.getInteger("Capabilities", "VertexShaderVersion", 30);
		config.textureMemory = ini.getInteger("Capabilities", "TextureMemory", 256);
		config.identifier = ini.getInteger("Capabilities", "Identifier", 0);
		config.vertexRoutineCacheSize = ini.getInteger("Caches", "VertexRoutineCacheSize", 1024);
		config.pixelRoutineCacheSize = ini.getInteger("Caches", "PixelRoutineCacheSize", 1024);
		config.setupRoutineCacheSize = ini.getInteger("Caches", "SetupRoutineCacheSize", 1024);
		config.vertexCacheSize = ini.getInteger("Caches", "VertexCacheSize", 64);
		config.textureSampleQuality = ini.getInteger("Quality", "TextureSampleQuality", 2);
		config.mipmapQuality = ini.getInteger("Quality", "MipmapQuality", 1);
		config.perspectiveCorrection = ini.getBoolean("Quality", "PerspectiveCorrection", true);
		config.transcendentalPrecision = ini.getInteger("Quality", "TranscendentalPrecision", 2);
		config.transparencyAntialiasing = ini.getInteger("Quality", "TransparencyAntialiasing", 0);
		config.threadCount = ini.getInteger("Processor", "ThreadCount", DEFAULT_THREAD_COUNT);
		config.enableSSE = ini.getBoolean("Processor", "EnableSSE", true);
		config.enableSSE2 = ini.getBoolean("Processor", "EnableSSE2", true);
		config.enableSSE3 = ini.getBoolean("Processor", "EnableSSE3", true);
		config.enableSSSE3 = ini.getBoolean("Processor", "EnableSSSE3", true);
		config.enableSSE4_1 = ini.getBoolean("Processor", "EnableSSE4_1", true);

		for(int pass = 0; pass < 10; pass++)
		{
			config.optimization[pass] = (Optimization)ini.getInteger("Optimization", "OptimizationPass" + itoa(pass + 1), pass == 0 ? InstructionCombining : Disabled);
		}

		config.disableServer = ini.getBoolean("Testing", "DisableServer", false);
		config.forceWindowed = ini.getBoolean("Testing", "ForceWindowed", false);
		config.complementaryDepthBuffer = ini.getBoolean("Testing", "ComplementaryDepthBuffer", false);
		config.postBlendSRGB = ini.getBoolean("Testing", "PostBlendSRGB", false);
		config.exactColorRounding = ini.getBoolean("Testing", "ExactColorRounding", true);
		config.disableAlphaMode = ini.getBoolean("Testing", "DisableAlphaMode", false);
		config.disable10BitMode = ini.getBoolean("Testing", "Disable10BitMode", false);
		config.frameBufferAPI = ini.getInteger("Testing", "FrameBufferAPI", 0);
		config.precache = ini.getBoolean("Testing", "Precache", false);
		config.shadowMapping = ini.getInteger("Testing", "ShadowMapping", 3);
		config.forceClearRegisters = ini.getBoolean("Testing", "ForceClearRegisters", false);

	#ifndef NDEBUG
		config.minPrimitives = 1;
		config.maxPrimitives = 1 << 21;
	#endif

		struct stat status;
		int lastModified = ini.getInteger("LastModified", "Time", 0);

		bool noConfig = stat("SwiftShader.ini", &status) != 0;
		newConfig = !noConfig && abs((int)status.st_mtime - lastModified) > 1;

		if(disableServerOverride)
		{
			config.disableServer = true;
		}
	}

	void SwiftConfig::writeConfiguration()
	{
		Configurator ini("SwiftShader.ini");

		ini.addValue("Capabilities", "PixelShaderVersion", itoa(config.pixelShaderVersion));
		ini.addValue("Capabilities", "VertexShaderVersion", itoa(config.vertexShaderVersion));
		ini.addValue("Capabilities", "TextureMemory", itoa(config.textureMemory));
		ini.addValue("Capabilities", "Identifier", itoa(config.identifier));
		ini.addValue("Caches", "VertexRoutineCacheSize", itoa(config.vertexRoutineCacheSize));
		ini.addValue("Caches", "PixelRoutineCacheSize", itoa(config.pixelRoutineCacheSize));
		ini.addValue("Caches", "SetupRoutineCacheSize", itoa(config.setupRoutineCacheSize));
		ini.addValue("Caches", "VertexCacheSize", itoa(config.vertexCacheSize));
		ini.addValue("Quality", "TextureSampleQuality", itoa(config.textureSampleQuality));
		ini.addValue("Quality", "MipmapQuality", itoa(config.mipmapQuality));
		ini.addValue("Quality", "PerspectiveCorrection", itoa(config.perspectiveCorrection));
		ini.addValue("Quality", "TranscendentalPrecision", itoa(config.transcendentalPrecision));
		ini.addValue("Quality", "TransparencyAntialiasing", itoa(config.transparencyAntialiasing));
		ini.addValue("Processor", "ThreadCount", itoa(config.threadCount));
	//	ini.addValue("Processor", "EnableSSE", itoa(config.enableSSE));
		ini.addValue("Processor", "EnableSSE2", itoa(config.enableSSE2));
		ini.addValue("Processor", "EnableSSE3", itoa(config.enableSSE3));
		ini.addValue("Processor", "EnableSSSE3", itoa(config.enableSSSE3));
		ini.addValue("Processor", "EnableSSE4_1", itoa(config.enableSSE4_1));

		for(int pass = 0; pass < 10; pass++)
		{
			ini.addValue("Optimization", "OptimizationPass" + itoa(pass + 1), itoa(config.optimization[pass]));
		}

		ini.addValue("Testing", "DisableServer", itoa(config.disableServer));
		ini.addValue("Testing", "ForceWindowed", itoa(config.forceWindowed));
		ini.addValue("Testing", "ComplementaryDepthBuffer", itoa(config.complementaryDepthBuffer));
		ini.addValue("Testing", "PostBlendSRGB", itoa(config.postBlendSRGB));
		ini.addValue("Testing", "ExactColorRounding", itoa(config.exactColorRounding));
		ini.addValue("Testing", "DisableAlphaMode", itoa(config.disableAlphaMode));
		ini.addValue("Testing", "Disable10BitMode", itoa(config.disable10BitMode));
		ini.addValue("Testing", "FrameBufferAPI", itoa(config.frameBufferAPI));
		ini.addValue("Testing", "Precache", itoa(config.precache));
		ini.addValue("Testing", "ShadowMapping", itoa(config.shadowMapping));
		ini.addValue("Testing", "ForceClearRegisters", itoa(config.forceClearRegisters));
		ini.addValue("LastModified", "Time", itoa((int)time(0)));

		ini.writeFile("SwiftShader Configuration File\n"
		              ";\n"
					  "; To get an overview of the valid settings and their meaning,\n"
					  "; run the application in windowed mode and open the\n"
					  "; SwiftConfig application or go to http://localhost:8080/swiftconfig.");
	}
}
