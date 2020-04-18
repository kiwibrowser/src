#!/usr/bin/python

import sys
import html5lib

htmlfilename = sys.argv[1]
htmlfile = open(htmlfilename)
try:
    doc = html5lib.parse(htmlfile, treebuilder="dom")
finally:
    htmlfile.close()

def elementHasClass(el, classArg):
    """
    Return true if and only if classArg is one of the classes of el
    """
    classes = [ c for c in el.getAttribute("class").split(" ") if c is not "" ]
    return classArg in classes

def elementTextContent(el):
    """
    Implementation of DOM Core's .textContent
    """
    textContent = ""
    for child in el.childNodes:
        if child.nodeType == 3: # Node.TEXT_NODE
            textContent += child.data
        elif child.nodeType == 1: # Node.ELEMENT_NODE
            textContent += elementTextContent(child)
        else:
            # Other nodes are ignored
            pass
    return textContent

preList = doc.getElementsByTagName("pre")
idlList = [elementTextContent(p) for p in preList if elementHasClass(p, "idl") ]
print "\n\n".join(idlList)
