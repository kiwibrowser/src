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

#ifndef _MMAP_INCLUDED_
#define _MMAP_INCLUDED_

//
// Encapsulate memory mapped files
//

class TMMap {
public:
	TMMap(const char* fileName) :
		fSize(-1), // -1 is the error value returned by GetFileSize()
		fp(nullptr),
		fBuff(0)   // 0 is the error value returned by MapViewOfFile()
	{
		if ((fp = fopen(fileName, "r")) == NULL)
			return;
		char c = getc(fp);
		fSize = 0;
		while (c != EOF) {
			fSize++;
			c = getc(fp);
		}
		if (c == EOF)
			fSize++;
		rewind(fp);
		fBuff = (char*)malloc(sizeof(char) * fSize);
		int count = 0;
		c = getc(fp);
		while (c != EOF) {
			fBuff[count++] = c;
			c = getc(fp);
		}
		fBuff[count++] = c;
	}

	char* getData() { return fBuff; }
	int   getSize() { return fSize; }

	~TMMap() {
		if (fp != NULL)
			fclose(fp);
	}

private:
	int             fSize;      // size of file to map in
	FILE *fp;
	char*           fBuff;      // the actual data;
};

#endif // _MMAP_INCLUDED_
