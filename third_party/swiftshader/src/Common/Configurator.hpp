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

#ifndef sw_Configurator_hpp
#define sw_Configurator_hpp

#include <string>
#include <vector>

#include <stdlib.h>

namespace sw
{
	class Configurator
	{
	public:
		Configurator(std::string iniPath = "");

		~Configurator();

		std::string getValue(std::string sectionName, std::string valueName, std::string defaultValue = "") const;
		int getInteger(std::string sectionName, std::string valueName, int defaultValue = 0) const;
		bool getBoolean(std::string sectionName, std::string valueName, bool defaultValue = false) const;
		double getFloat(std::string sectionName, std::string valueName, double defaultValue = 0.0) const;
		unsigned int getFormatted(std::string sectionName, std::string valueName, char *format,
		                          void *v1 = 0, void *v2 = 0, void *v3 = 0, void *v4 = 0,
		                          void *v5 = 0, void *v6 = 0, void *v7 = 0, void *v8 = 0,
		                          void *v9 = 0, void *v10 = 0, void *v11 = 0, void *v12 = 0,
		                          void *v13 = 0, void *v14 = 0, void *v15 = 0, void *v16 = 0);

		void addValue(std::string sectionName, std::string valueName, std::string value);

		void writeFile(std::string title = "Configuration File");

	private:
		bool readFile();

		unsigned int addKeyName(std::string sectionName);
		int findKey(std::string sectionName) const;
		int findValue(unsigned int sectionID, std::string valueName) const;

		std::string path;

		struct Section
		{
			std::vector<std::string> names;
			std::vector<std::string> values;
		};

		std::vector<Section> sections;
		std::vector<std::string> names;
	};
}

#endif   // sw_Configurator_hpp
