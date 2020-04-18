#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import sys

SVG_NODE_HEIGHT = 17
FONT_SIZE = 12


def hash_to_float(string):
    return hash(string) / float(sys.maxsize)


def getLegacyColor(method):
    r = 175 + int(50 * hash_to_float(reversed(method)))
    g = 60 + int(180 * hash_to_float(method))
    b = 60 + int(55 * hash_to_float(reversed(method)))
    return (r, g, b)


def getDSOColor(method):
    r = 170 + int(80 * hash_to_float(reversed(method)))
    g = 180 + int(70 * hash_to_float((method)))
    b = 170 + int(80 * hash_to_float(reversed(method)))
    return (r, g, b)


def getHeatColor(callsite, total_weight):
    r = 245 + 10 * (1 - callsite.weight() / total_weight)
    g = 110 + 105 * (1 - callsite.weight() / total_weight)
    b = 100
    return (r, g, b)


def createSVGNode(callsite, depth, f, total_weight, height, color_scheme, nav):
    x = float(callsite.offset) / total_weight * 100
    y = height - (depth + 1) * SVG_NODE_HEIGHT
    width = callsite.weight() / total_weight * 100

    method = callsite.method.replace(">", "&gt;").replace("<", "&lt;")
    if width <= 0:
        return

    if color_scheme == "dso":
        r, g, b = getDSOColor(callsite.dso)
    elif color_scheme == "legacy":
        r, g, b = getLegacyColor(method)
    else:
        r, g, b = getHeatColor(callsite, total_weight)

    r_border, g_border, b_border = [max(0, color - 50) for color in [r, g, b]]

    f.write(
        """<g id=%d class="n" onclick="zoom(this);" onmouseenter="select(this);" nav="%s">
        <title>%s | %s (%.0f events: %3.2f%%)</title>
        <rect x="%f%%" y="%f" ox="%f" oy="%f" width="%f%%" owidth="%f" height="15.0"
        ofill="rgb(%d,%d,%d)" fill="rgb(%d,%d,%d)" style="stroke:rgb(%d,%d,%d)"/>
        <text x="%f%%" y="%f" font-size="%d" font-family="Monospace"></text>
        </g>""" %
        (callsite.id,
         ','.join(str(x) for x in nav),
         method,
         callsite.dso,
         callsite.weight(),
         callsite.weight() / total_weight * 100,
         x,
         y,
         x,
         y,
         width,
         width,
         r,
         g,
         b,
         r,
         g,
         b,
         r_border,
         g_border,
         b_border,
         x,
         y + 12,
         FONT_SIZE))


def renderSVGNodes(flamegraph, depth, f, total_weight, height, color_scheme):
    for i, child in enumerate(flamegraph.children):
        # Prebuild navigation target for wasd

        if i == 0:
            left_index = 0
        else:
            left_index = flamegraph.children[i - 1].id

        if i == len(flamegraph.children) - 1:
            right_index = 0
        else:
            right_index = flamegraph.children[i + 1].id

        up_index = max(child.children, key=lambda x: x.weight()).id if child.children else 0

        # up, left, down, right
        nav = [up_index, left_index, flamegraph.id, right_index]

        createSVGNode(child, depth, f, total_weight, height, color_scheme, nav)
        # Recurse down
        renderSVGNodes(child, depth + 1, f, total_weight, height, color_scheme)


def renderSearchNode(f):
    f.write(
        """<rect id="search_rect"  style="stroke:rgb(0,0,0);" onclick="search(this);" class="t"
        rx="10" ry="10" x="1029" y="10" width="80" height="30" fill="rgb(255,255,255)""/>
        <text id="search_text"  class="t" x="1044" y="30"    onclick="search(this);">Search</text>
        """)


def renderUnzoomNode(f):
    f.write(
        """<rect id="zoom_rect" style="display:none;stroke:rgb(0,0,0);" class="t"
        onclick="unzoom(this);" rx="10" ry="10" x="10" y="10" width="80" height="30"
        fill="rgb(255,255,255)"/>
         <text id="zoom_text" style="display:none;" class="t" x="19" y="30"
         onclick="unzoom(this);">Zoom out</text>""")


def renderInfoNode(f):
    f.write(
        """<clipPath id="info_clip_path"> <rect id="info_rect" style="stroke:rgb(0,0,0);"
        rx="10" ry="10" x="120" y="10" width="789" height="30" fill="rgb(255,255,255)"/>
        </clipPath>
        <rect id="info_rect" style="stroke:rgb(0,0,0);"
        rx="10" ry="10" x="120" y="10" width="799" height="30" fill="rgb(255,255,255)"/>
         <text clip-path="url(#info_clip_path)" id="info_text" x="128" y="30"></text>""")


def renderPercentNode(f):
    f.write(
        """<rect id="percent_rect" style="stroke:rgb(0,0,0);"
        rx="10" ry="10" x="934" y="10" width="82" height="30" fill="rgb(255,255,255)"/>
         <text  id="percent_text" text-anchor="end" x="999" y="30">100.00%%</text>""")


def renderSVG(flamegraph, f, color_scheme):
    height = (flamegraph.get_max_depth() + 2) * SVG_NODE_HEIGHT
    f.write("""<div class="flamegraph_block" style="width:100%%; height:%dpx;">
            """ % height)
    f.write("""<svg xmlns="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1"
    width="100%%" height="100%%" style="border: 1px solid black;"
    rootid="%d">
    """ % (flamegraph.children[0].id))
    f.write("""<defs > <linearGradient id="background_gradiant" y1="0" y2="1" x1="0" x2="0" >
    <stop stop-color="#eeeeee" offset="5%" /> <stop stop-color="#efefb1" offset="90%" />
    </linearGradient> </defs>""")
    f.write("""<rect x="0.0" y="0" width="100%" height="100%" fill="url(#background_gradiant)" />
            """)
    renderSVGNodes(flamegraph, 0, f, flamegraph.weight(), height, color_scheme)
    renderSearchNode(f)
    renderUnzoomNode(f)
    renderInfoNode(f)
    renderPercentNode(f)
    f.write("</svg></div><br/>\n\n")
