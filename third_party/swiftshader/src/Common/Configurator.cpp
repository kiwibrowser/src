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

#include "Configurator.hpp"

#include <iostream>
#include <fstream>

using namespace std;

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(__unix__)
#include <unistd.h>
#endif

namespace sw
{
	Configurator::Configurator(string iniPath)
	{
		path = iniPath;

		readFile();
	}

	Configurator::~Configurator()
	{
	}

	bool Configurator::readFile()
	{
		#if defined(__unix__)
			if(access(path.c_str(), R_OK) != 0)
			{
				return false;
			}
		#endif

		fstream file(path.c_str(), ios::in);
		if(file.fail()) return false;

		string line;
		string keyName;

		while(getline(file, line))
		{
			if(line.length())
			{
				if(line[line.length() - 1] == '\r')
				{
					line = line.substr(0, line.length() - 1);
				}

				if(!isprint(line[0]))
				{
					printf("Failing on char %d\n", line[0]);
					file.close();
					return false;
				}

				string::size_type pLeft = line.find_first_of(";#[=");

				if(pLeft != string::npos)
				{
					switch(line[pLeft])
					{
					case '[':
						{
							string::size_type pRight = line.find_last_of("]");

							if(pRight != string::npos && pRight > pLeft)
							{
								keyName = line.substr(pLeft + 1, pRight - pLeft - 1);
								addKeyName(keyName);
							}
						}
						break;
					case '=':
						{
							string valueName = line.substr(0, pLeft);
							string value = line.substr(pLeft + 1);
							addValue(keyName, valueName, value);
						}
						break;
					case ';':
					case '#':
						// Ignore comments
						break;
					}
				}
			}
		}

		file.close();

		if(names.size())
		{
			return true;
		}

		return false;
	}

	void Configurator::writeFile(std::string title)
	{
		#if defined(__unix__)
			if(access(path.c_str(), W_OK) != 0)
			{
				return;
			}
		#endif

		fstream file(path.c_str(), ios::out);
		if(file.fail()) return;

		file << "; " << title << endl << endl;

		for(unsigned int keyID = 0; keyID < sections.size(); keyID++)
		{
			file << "[" << names[keyID] << "]" << endl;

			for(unsigned int valueID = 0; valueID < sections[keyID].names.size(); valueID++)
			{
				file << sections[keyID].names[valueID] << "=" << sections[keyID].values[valueID] << endl;
			}

			file << endl;
		}

		file.close();
	}

	int Configurator::findKey(string keyName) const
	{
		for(unsigned int keyID = 0; keyID < names.size(); keyID++)
		{
			if(names[keyID] == keyName)
			{
				return keyID;
			}
		}

		return -1;
	}

	int Configurator::findValue(unsigned int keyID, string valueName) const
	{
		if(!sections.size() || keyID >= sections.size())
		{
			return -1;
		}

		for(unsigned int valueID = 0; valueID < sections[keyID].names.size(); ++valueID)
		{
			if(sections[keyID].names[valueID] == valueName)
			{
				return valueID;
			}
		}

		return -1;
	}

	unsigned int Configurator::addKeyName(string keyName)
	{
		names.resize(names.size() + 1, keyName);
		sections.resize(sections.size() + 1);
		return (unsigned int)names.size() - 1;
	}

	void Configurator::addValue(string const keyName, string const valueName, string const value)
	{
		int keyID = findKey(keyName);

		if(keyID == -1)
		{
			keyID = addKeyName(keyName);
		}

		int valueID = findValue(keyID, valueName);

		if(valueID == -1)
		{
			sections[keyID].names.resize(sections[keyID].names.size() + 1, valueName);
			sections[keyID].values.resize(sections[keyID].values.size() + 1, value);
		}
		else
		{
			sections[keyID].values[valueID] = value;
		}
	}

	string Configurator::getValue(string keyName, string valueName, string defaultValue) const
	{
		int keyID = findKey(keyName);
		if(keyID == -1) return defaultValue;
		int valueID = findValue((unsigned int)keyID, valueName);
		if(valueID == -1) return defaultValue;

		return sections[keyID].values[valueID];
	}

	int Configurator::getInteger(string keyName, string valueName, int defaultValue) const
	{
		char svalue[256];

		sprintf(svalue, "%d", defaultValue);

		return atoi(getValue(keyName, valueName, svalue).c_str());
	}

	bool Configurator::getBoolean(string keyName, string valueName, bool defaultValue) const
	{
		return getInteger(keyName, valueName, (int)defaultValue) != 0;
	}

	double Configurator::getFloat(string keyName, string valueName, double defaultValue) const
	{
		char svalue[256];

		sprintf(svalue, "%f", defaultValue);

		return atof(getValue(keyName, valueName, svalue).c_str());
	}

	unsigned int Configurator::getFormatted(string keyName, string valueName, char *format,
											void *v1, void *v2, void *v3, void *v4,
											void *v5, void *v6, void *v7, void *v8,
											void *v9, void *v10, void *v11, void *v12,
											void *v13, void *v14, void *v15, void *v16)
	{
		string value = getValue(keyName, valueName);

		if(!value.length()) return false;

		unsigned int nVals = sscanf(value.c_str(), format,
									v1, v2, v3, v4, v5, v6, v7, v8,
									v9, v10, v11, v12, v13, v14, v15, v16);

		return nVals;
	}
}
