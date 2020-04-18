/*-------------------------------------------------------------------------
 * drawElements Quality Program Helper Library
 * -------------------------------------------
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
 * \brief XML Writer.
 *//*--------------------------------------------------------------------*/

#include "qpXmlWriter.h"

#include "deMemory.h"
#include "deInt32.h"

/*------------------------------------------------------------------------
 * qpXmlWriter stand-alone implementation.
 *----------------------------------------------------------------------*/

#include "deMemPool.h"
#include "dePoolArray.h"

struct qpXmlWriter_s
{
	FILE*				outputFile;
	deBool				flushAfterWrite;

	deBool				xmlPrevIsStartElement;
	deBool				xmlIsWriting;
	int					xmlElementDepth;
};

static deBool writeEscaped (qpXmlWriter* writer, const char* str)
{
	char		buf[256 + 10];
	char*		d		= &buf[0];
	const char*	s		= str;
	deBool		isEOS	= DE_FALSE;

	do
	{
		/* Check for characters that need to be escaped. */
		const char* repl = DE_NULL;
		switch (*s)
		{
			case 0:		isEOS = DE_TRUE;		break;
			case '<':	repl = "&lt;";			break;
			case '>':	repl = "&gt;";			break;
			case '&':	repl = "&amp;";			break;
			case '\'':	repl = "&apos;";		break;
			case '"':	repl = "&quot;";		break;

			/* Non-printable characters. */
			case 1:		repl = "&lt;SOH&gt;";	break;
			case 2:		repl = "&lt;STX&gt;";	break;
			case 3:		repl = "&lt;ETX&gt;";	break;
			case 4:		repl = "&lt;EOT&gt;";	break;
			case 5:		repl = "&lt;ENQ&gt;";	break;
			case 6:		repl = "&lt;ACK&gt;";	break;
			case 7:		repl = "&lt;BEL&gt;";	break;
			case 8:		repl = "&lt;BS&gt;";	break;
			case 11:	repl = "&lt;VT&gt;";	break;
			case 12:	repl = "&lt;FF&gt;";	break;
			case 14:	repl = "&lt;SO&gt;";	break;
			case 15:	repl = "&lt;SI&gt;";	break;
			case 16:	repl = "&lt;DLE&gt;";	break;
			case 17:	repl = "&lt;DC1&gt;";	break;
			case 18:	repl = "&lt;DC2&gt;";	break;
			case 19:	repl = "&lt;DC3&gt;";	break;
			case 20:	repl = "&lt;DC4&gt;";	break;
			case 21:	repl = "&lt;NAK&gt;";	break;
			case 22:	repl = "&lt;SYN&gt;";	break;
			case 23:	repl = "&lt;ETB&gt;";	break;
			case 24:	repl = "&lt;CAN&gt;";	break;
			case 25:	repl = "&lt;EM&gt;";	break;
			case 26:	repl = "&lt;SUB&gt;";	break;
			case 27:	repl = "&lt;ESC&gt;";	break;
			case 28:	repl = "&lt;FS&gt;";	break;
			case 29:	repl = "&lt;GS&gt;";	break;
			case 30:	repl = "&lt;RS&gt;";	break;
			case 31:	repl = "&lt;US&gt;";	break;

			default:	/* nada */				break;
		}

		/* Write out char or escape sequence. */
		if (repl)
		{
			s++;
			strcpy(d, repl);
			d += strlen(repl);
		}
		else
			*d++ = *s++;

		/* Write buffer if EOS or buffer full. */
		if (isEOS || ((d - &buf[0]) >= 4))
		{
			*d = 0;
			fprintf(writer->outputFile, "%s", buf);
			d = &buf[0];
		}
	} while (!isEOS);

	if (writer->flushAfterWrite)
		fflush(writer->outputFile);
	DE_ASSERT(d == &buf[0]); /* buffer must be empty */
	return DE_TRUE;
}

qpXmlWriter* qpXmlWriter_createFileWriter (FILE* outputFile, deBool useCompression, deBool flushAfterWrite)
{
	qpXmlWriter* writer = (qpXmlWriter*)deCalloc(sizeof(qpXmlWriter));
	if (!writer)
		return DE_NULL;

	DE_UNREF(useCompression); /* no compression supported. */

	writer->outputFile = outputFile;
	writer->flushAfterWrite = flushAfterWrite;

	return writer;
}

void qpXmlWriter_destroy (qpXmlWriter* writer)
{
	DE_ASSERT(writer);

	deFree(writer);
}

static deBool closePending (qpXmlWriter* writer)
{
	if (writer->xmlPrevIsStartElement)
	{
		fprintf(writer->outputFile, ">\n");
		writer->xmlPrevIsStartElement = DE_FALSE;
	}

	return DE_TRUE;
}

void qpXmlWriter_flush (qpXmlWriter* writer)
{
	closePending(writer);
}

deBool qpXmlWriter_startDocument (qpXmlWriter* writer)
{
	DE_ASSERT(writer && !writer->xmlIsWriting);
	writer->xmlIsWriting			= DE_TRUE;
	writer->xmlElementDepth			= 0;
	writer->xmlPrevIsStartElement	= DE_FALSE;
	fprintf(writer->outputFile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	return DE_TRUE;
}

static const char* getIndentStr (int indentLevel)
{
	static const char	s_indentStr[33]	= "                                ";
	static const int	s_indentStrLen	= 32;
	return &s_indentStr[s_indentStrLen - deMin32(s_indentStrLen, indentLevel)];
}

deBool qpXmlWriter_endDocument (qpXmlWriter* writer)
{
	DE_ASSERT(writer);
	DE_ASSERT(writer->xmlIsWriting);
	DE_ASSERT(writer->xmlElementDepth == 0);
	closePending(writer);
	writer->xmlIsWriting = DE_FALSE;
	return DE_TRUE;
}

deBool qpXmlWriter_writeString (qpXmlWriter* writer, const char* str)
{
	if (writer->xmlPrevIsStartElement)
	{
		fprintf(writer->outputFile, ">");
		writer->xmlPrevIsStartElement = DE_FALSE;
	}

	return writeEscaped(writer, str);
}

deBool qpXmlWriter_startElement(qpXmlWriter* writer, const char* elementName, int numAttribs, const qpXmlAttribute* attribs)
{
	int ndx;

	closePending(writer);

	fprintf(writer->outputFile, "%s<%s", getIndentStr(writer->xmlElementDepth), elementName);

	for (ndx = 0; ndx < numAttribs; ndx++)
	{
		const qpXmlAttribute* attrib = &attribs[ndx];
		fprintf(writer->outputFile, " %s=\"", attrib->name);
		switch (attrib->type)
		{
			case QP_XML_ATTRIBUTE_STRING:
				writeEscaped(writer, attrib->stringValue);
				break;

			case QP_XML_ATTRIBUTE_INT:
			{
				char buf[64];
				sprintf(buf, "%d", attrib->intValue);
				writeEscaped(writer, buf);
				break;
			}

			case QP_XML_ATTRIBUTE_BOOL:
				writeEscaped(writer, attrib->boolValue ? "True" : "False");
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}
		fprintf(writer->outputFile, "\"");
	}

	writer->xmlElementDepth++;
	writer->xmlPrevIsStartElement = DE_TRUE;
	return DE_TRUE;
}

deBool qpXmlWriter_endElement (qpXmlWriter* writer, const char* elementName)
{
	DE_ASSERT(writer && writer->xmlElementDepth > 0);
	writer->xmlElementDepth--;

	if (writer->xmlPrevIsStartElement) /* leave flag as-is */
	{
		fprintf(writer->outputFile, " />\n");
		writer->xmlPrevIsStartElement = DE_FALSE;
	}
	else
		fprintf(writer->outputFile, "</%s>\n", /*getIndentStr(writer->xmlElementDepth),*/ elementName);

	return DE_TRUE;
}

deBool qpXmlWriter_writeBase64 (qpXmlWriter* writer, const deUint8* data, size_t numBytes)
{
	static const char s_base64Table[64] =
	{
		'A','B','C','D','E','F','G','H','I','J','K','L','M',
		'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
		'a','b','c','d','e','f','g','h','i','j','k','l','m',
		'n','o','p','q','r','s','t','u','v','w','x','y','z',
		'0','1','2','3','4','5','6','7','8','9','+','/'
	};

	int			numWritten	= 0;
	size_t		srcNdx		= 0;
	deBool		writeIndent	= DE_TRUE;
	const char*	indentStr	= getIndentStr(writer->xmlElementDepth);

	DE_ASSERT(writer && data && (numBytes > 0));

	/* Close and pending writes. */
	closePending(writer);

	/* Loop all input chars. */
	while (srcNdx < numBytes)
	{
		size_t	numRead = (size_t)deMin32(3, (int)(numBytes - srcNdx));
		deUint8	s0 = data[srcNdx];
		deUint8	s1 = (numRead >= 2) ? data[srcNdx+1] : 0;
		deUint8	s2 = (numRead >= 3) ? data[srcNdx+2] : 0;
		char	d[5];

		srcNdx += numRead;

		d[0] = s_base64Table[s0 >> 2];
		d[1] = s_base64Table[((s0&0x3)<<4) | (s1>>4)];
		d[2] = s_base64Table[((s1&0xF)<<2) | (s2>>6)];
		d[3] = s_base64Table[s2&0x3F];
		d[4] = 0;

		if (numRead < 3) d[3] = '=';
		if (numRead < 2) d[2] = '=';

		/* Write indent (if needed). */
		if (writeIndent)
		{
			fprintf(writer->outputFile, "%s", indentStr);
			writeIndent = DE_FALSE;
		}

		/* Write data. */
		fprintf(writer->outputFile, "%s", &d[0]);

		/* EOL every now and then. */
		numWritten += 4;
		if (numWritten >= 64)
		{
			fprintf(writer->outputFile, "\n");
			numWritten = 0;
			writeIndent = DE_TRUE;
		}
	}

	/* Last EOL. */
	if (numWritten > 0)
		fprintf(writer->outputFile, "\n");

	DE_ASSERT(srcNdx == numBytes);
	return DE_TRUE;
}

/* Common helper functions. */

deBool qpXmlWriter_writeStringElement (qpXmlWriter* writer, const char* elementName, const char* elementContent)
{
	if (!qpXmlWriter_startElement(writer, elementName, 0, DE_NULL) ||
		(elementContent && !qpXmlWriter_writeString(writer, elementContent)) ||
		!qpXmlWriter_endElement(writer, elementName))
		return DE_FALSE;

	return DE_TRUE;
}
