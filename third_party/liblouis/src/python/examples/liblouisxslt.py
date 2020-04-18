#! /usr/bin/python -u
#
# This is a very simple example on how to extend libxslt to be able to
# invoke liblouis from xslt. See also the accompanying
# dtbook2brldtbook.xsl in the same directory which simpy copies a dtbook
# xml and translates all the text node into Braille.

import louis
import libxml2
import libxslt
import sys
import getopt
from optparse import OptionParser

nodeName = None

emphasisMap = {
    'plain_text' : louis.plain_text, 
    'italic' : louis.italic, 
    'underline' : louis.underline, 
    'bold' : louis.bold, 
    'computer_braille' : louis.computer_braille}

def translate(ctx, str, translation_table, emphasis=None):
    global nodeName
    
    try:
        pctxt = libxslt.xpathParserContext(_obj=ctx)
        ctxt = pctxt.context()
        tctxt = ctxt.transformContext()
        nodeName = tctxt.insertNode().name
    except:
        pass

    typeform = len(str)*[emphasisMap[emphasis]] if emphasis else None
    braille = louis.translate([translation_table], str.decode('utf-8'), typeform=typeform)[0]
    return braille.encode('utf-8')

def xsltProcess(styleFile, inputFile, outputFile):
    """Transform an xml inputFile to an outputFile using the given styleFile"""
    styledoc = libxml2.parseFile(styleFile)
    style = libxslt.parseStylesheetDoc(styledoc)
    doc = libxml2.parseFile(inputFile)
    result = style.applyStylesheet(doc, None)
    style.saveResultToFilename(outputFile, result, 0)
    style.freeStylesheet()
    doc.freeDoc()
    result.freeDoc()

libxslt.registerExtModuleFunction("translate", "http://liblouis.org/liblouis", translate)

def main():
    usage = "Usage: %prog [options] styleFile inputFile outputFile"
    parser = OptionParser(usage)
    (options, args) = parser.parse_args()
    if len(args) != 3:
        parser.error("incorrect number of arguments")
    xsltProcess(args[0], args[1], args[2])

if __name__ == "__main__":
    main()
