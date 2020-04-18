/* -*- Mode: js2; js2-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40; -*- */
/*
 Copyright (c) 2009  Mozilla Corp

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

if (!("console" in window)) {
  window.console = { log: function(s) {
                       var l = document.getElementById('log');
                       if (l) {
                         l.innerHTML = l.innerHTML + "<span>" + s.toString() + "</span><br>";
                       }
                     }
                   };
}

function nodeText(n) {
  var s = "";
  for (c = n.firstChild;
       c;
       c = c.nextSibling)
  {
    if (c.nodeType != 3)
      break;
    s += c.textContent;
  }

  return s;
}

function parseFloatListString (s) {
  if (s == "")
    return [];

  // this is horrible
  var ss = s.split(/\s+/);
  var res = Array(ss.length);
  for (var i = 0, j = 0; i < ss.length; i++) {
    if (ss[i].length == 0)
      continue;
    res[j++] = parseFloat(ss[i]);
  }
  return res;
}

function parseIntListString (s) {
  if (s == "")
    return [];

  // this is horrible
  var ss = s.split(/\s+/);

  var res = Array(ss.length);
  for (var i = 0, j = 0; i < ss.length; i++) {
    if (ss[i].length == 0)
      continue;
    res[j++] = parseInt(ss[i]);
  }
  return res;
}

function runSoon(f) {
  setTimeout(f, 0);
}

function xpathGetElementById(xmldoc, id) {
  return xmldoc.evaluate("//*[@id=\"" + id + "\"]", xmldoc, null,
                         XPathResult.FIRST_ORDERED_NODE_TYPE,
                         null).singleNodeValue;
}

function SporeFile() {
}

SporeFile.prototype = {
  load: function spore_load (src) {
    var xhr = new XMLHttpRequest();
    var self = this;
    xhr.onreadystatechange = function () {
      // Status of 0 handles files coming off the local disk
      if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
        runSoon(function () {
                  var xml = xhr.responseXML;
                  xml.getElementById = function(id) {
                    return xpathGetElementById(xml, id);
                  };
                  self.parse(xml);

                  if (self._loadHandler) {
                    runSoon(function () { self._loadHandler.apply(window); });
                  }
                });
      }
    };

    xhr.open("GET", src, true);
    xhr.overrideMimeType("text/xml");
    xhr.setRequestHeader("Content-Type", "text/xml");
    xhr.send(null);
  },

  parse: function spore_parse (xml) {
    function nsResolver(prefix) {
      var ns = {
        'c' : 'http://www.collada.org/2005/11/COLLADASchema'
      };
      return ns[prefix] || null;
    }

    function getNode(xpathexpr, ctxNode) {
      if (ctxNode == null)
        ctxNode = xml;
      //console.log("xpath: " + xpathexpr);
      return xml.evaluate(xpathexpr, ctxNode, nsResolver, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;
    }

    // pull out the Z-up flag
    var isYUp = true;
    var upAxisNode = getNode('//c:asset/c:up_axis');
    if (upAxisNode) {
      var val = nodeText(upAxisNode);
      if (val.indexOf("Z_UP") != 1)
        isYUp = false;
    }

    // pull out the mesh geometry
    var meshNode = getNode('//c:library_geometries/c:geometry[@id="mesh"]/c:mesh');
    if (!meshNode) {
      console.log("Couldn't find mesh node");
      return false;
    }

    var baseMesh = { };

    this.xns = nsResolver;
    this.gn = getNode;
    this.meshNode = meshNode;

    // then we have position, normal, texcoord, tangent, and bitangent float_arrays to pull out
    var farrays = ['position', 'normal', 'texcoord', 'tangent', 'bitangent'];
    for (var i = 0; i < farrays.length; ++i) {
      var fname = farrays[i];

      var fnode = getNode('//c:source[@id="' + fname + '"]/c:float_array', meshNode);
      if (!fnode) {
        console.log("Missing " + fname + " float_array in mesh node");
        return false;
      }

      var count = parseInt(fnode.getAttribute('count'));
      var data = parseFloatListString(nodeText(fnode));

      if (data.length < count) {
        console.log("Attribute " + fname + " expected " + count + " elements but parse only gave " + data.length);
        return false;
      }

      baseMesh[fname] = data;
    }

    // then grab the indices
    var inode = getNode('//c:triangles/c:p', meshNode);
    if (!inode) {
      console.log("Missing triangles/p in mesh node");
      return false;
    }

    var indices = parseIntListString(nodeText(inode));

    var baseLen = Math.floor(indices.length/3);

    var mesh = {
      position: Array(baseLen*3),
      normal: Array(baseLen*3),
      texcoord: Array(baseLen*2),
      textangent: Array(baseLen*3),
      texbinormal: Array(baseLen*3)
    };

    // figure out the bounding box
    var minx = Infinity, miny = Infinity, minz = Infinity;
    var maxx = -Infinity, maxy = -Infinity, maxz = -Infinity;
    var npoints = Math.floor(baseMesh.position.length / 3);
    for (var i = 0; i < npoints; ++i) {
      var x = baseMesh.position[i*3  ];
      var y = baseMesh.position[i*3+1];
      var z = baseMesh.position[i*3+2];

      minx = Math.min(minx, x);
      miny = Math.min(miny, y);
      minz = Math.min(minz, z);

      maxx = Math.max(maxx, x);
      maxy = Math.max(maxy, y);
      maxz = Math.max(maxz, z);
    }

    mesh.bbox = {
      min: { x: minx, y: miny, z: minz },
      max: { x: maxx, y: maxy, z: maxz }
    };

    // XXX the vertex index and the normal index tend to be identical;
    // however, the texture info is not.  I don't really know how to unroll
    // that for efficient rendering with GL (where I only get one index array
    // to play with), so we expand everything out here.

    for (var i = 0; i < baseLen; ++i) {
      var vindex = indices[i*3];
      var nindex = indices[i*3+1];
      var tindex = indices[i*3+2];

      mesh.position[i*3  ] = baseMesh.position[vindex*3  ];
      mesh.position[i*3+1] = baseMesh.position[vindex*3+1];
      mesh.position[i*3+2] = baseMesh.position[vindex*3+2];

      mesh.normal[i*3  ] = baseMesh.normal[nindex*3  ];
      mesh.normal[i*3+1] = baseMesh.normal[nindex*3+1];
      mesh.normal[i*3+2] = baseMesh.normal[nindex*3+2];

      mesh.texcoord[i*2  ] = baseMesh.texcoord[tindex*2  ];
      mesh.texcoord[i*2+1] = baseMesh.texcoord[tindex*2+1];
    }

    // now load the textures and kick off the loads
    var textures = { };
    var inames = ["diffuse", "normal", "specular"];
    for (var i = 0; i < inames.length; ++i) {
      var node = getNode('//c:library_images/c:image[@name="' + inames[i] + '"]/c:init_from');
      if (!node)
        continue;

      var name = nodeText(node);

      // convert tga's to png's
      if (name.substr(-4).toLowerCase() == ".tga")
        name = name.substr(0, name.length-3) + "png";

      var uri = xml.baseURI.toString();
      uri = uri.substr(0, uri.lastIndexOf("/")) + "/" + name;

      var img = new Image();
      img.src = uri;

      textures[inames[i]] = img;
    }

    this.mesh = mesh;
    this.textures = textures;

    return true;
  }
};
