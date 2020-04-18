// A list of all SVG tags plus optional properties:
//   * needParent       (string)        - parent element required for a valid context
//   * needChild        (string list)   - child element(s) required for a valid context
//   * needAttr         (string map)    - attribute(s) required for a valid context
//   * noRenderer       (bool)          - true if the element doesn't have an associated renderer

var SvgTags = {
    a:                    { },
//  audio:                { },
    animate:              { noRenderer: true },
    animateMotion:        { noRenderer: true },
    animateTransform:     { noRenderer: true },
//  canvas:               { },
    circle:               { },
    clipPath:             { },
    cursor:               { noRenderer: true },
    defs:                 { },
    desc:                 { noRenderer: true },
    discard:              { noRenderer: true },
    ellipse:              { },
    feBlend:              { needParent: 'filter' },
    feColorMatrix:        { needParent: 'filter' },
    feComponentTransfer:  { needParent: 'filter' },
    feComposite:          { needParent: 'filter' },
    feConvolveMatrix:     { needParent: 'filter', needAttr: { kernelMatrix: '0 0 0 0 0 0 0 0 0' } },
    feDiffuseLighting:    { needParent: 'filter', needChild: [ 'fePointLight' ] },
    feDisplacementMap:    { needParent: 'filter' },
    feDistantLight:       { needParent: 'feSpecularLighting' },
    feDropShadow:         { needParent: 'filter' },
    feFlood:              { needParent: 'filter' },
    feFuncA:              { needParent: 'feComponentTransfer' },
    feFuncB:              { needParent: 'feComponentTransfer' },
    feFuncG:              { needParent: 'feComponentTransfer' },
    feFuncR:              { needParent: 'feComponentTransfer' },
    feGaussianBlur:       { needParent: 'filter' },
    feImage:              { needParent: 'filter' },
    feMerge:              { needParent: 'filter', needChild: [ 'feMergeNode' ] },
    feMergeNode:          { needParent: 'feMerge' },
    feMorphology:         { needParent: 'filter' },
    feOffset:             { needParent: 'filter' },
    fePointLight:         { needParent: 'feSpecularLighting' },
    feSpecularLighting:   { needParent: 'filter', needChild: [ 'fePointLight' ] },
    feSpotLight:          { needParent: 'feSpecularLighting' },
    feTile:               { needParent: 'filter' },
    feTurbulence:         { needParent: 'filter' },
    filter:               { },
    foreignObject:        { },
    g:                    { },
//  hatch:                { },
//  hatchPath:            { },
//  iframe:               { },
    image:                { },
    line:                 { },
    linearGradient:       { },
    marker:               { },
    mask:                 { },
//  meshGradient:         { },
//  meshPatch:            { },
//  meshRow:              { },
    metadata:             { noRenderer: true },
    mpath:                { },
    path:                 { },
    pattern:              { },
    polygon:              { },
    polyline:             { },
    radialGradient:       { },
    rect:                 { },
    script:               { noRenderer: true },
    set:                  { noRenderer: true },
//  solidColor:           { },
//  source:               { },
    stop:                 { },
    style:                { noRenderer: true },
    svg:                  { },
    switch:               { },
    symbol:               { },
    text:                 { },
    textPath:             { },
//  track:                { },
    title:                { noRenderer: true },
    tspan:                { },
    use:                  { },
//  video:                { },
    view:                 { noRenderer: true },
}

// SVG element class shorthands as defined by the spec.
var SvgTagClasses = {
    CLASS_ANIMATION: [
        // https://svgwg.org/svg2-draft/animate.html#TermAnimationElement
        'animate', 'animateMotion', 'animateTransform', 'discard', 'set'
    ],

    CLASS_CONTAINER: [
        // https://svgwg.org/svg2-draft/struct.html#TermContainerElement
        'a', 'defs', 'g', 'marker', 'mask', 'pattern', 'svg', 'switch', 'symbol'
    ],

    CLASS_DESCRIPTIVE: [
        // https://svgwg.org/svg2-draft/struct.html#TermDescriptiveElement
        'desc', 'metadata', 'title'
    ],

    CLASS_FILTER_PRIMITIVE: [
         // http://dev.w3.org/fxtf/filters/#elementdef-filter-primitive
        'feBlend', 'feColorMatrix', 'feComponentTransfer', 'feComposite', 'feConvolveMatrix',
        'feDiffuseLighting', 'feDisplacementMap', 'feDropShadow', 'feFlood', 'feGaussianBlur',
        'feImage', 'feMerge', 'feMorphology', 'feOffset', 'feSpecularLighting', 'feTile',
        'feTurbulence'
    ],

    CLASS_GRAPHICS: [
        // https://svgwg.org/svg2-draft/struct.html#TermGraphicsElement
        'audio', 'canvas', 'circle', 'ellipse', 'foreignObject', 'iframe', 'image', 'line', 'path',
        'polygon', 'polyline', 'rect', 'text', 'use', 'video'
    ],

    CLASS_GRAPHICS_REFERENCING_ELEMENT: [
        // https://svgwg.org/svg2-draft/struct.html#TermGraphicsReferencingElement
        'audio', 'iframe', 'image', 'use', 'video'
    ],

    CLASS_LIGHT_SOURCE: [
        // http://dev.w3.org/fxtf/filters/#light-source
        'feDistantLight', 'fePointLight', 'feSpotLight'
    ],

    CLASS_PAINT_SERVER: [
        // https://svgwg.org/svg2-draft/intro.html#TermPaintServerElement
        'solidColor', 'linearGradient', 'radialGradient', 'meshGradient', 'pattern', 'hatch'
    ],

    CLASS_SHAPE: [
        // https://svgwg.org/svg2-draft/shapes.html#TermShapeElement
        'circle', 'ellipse', 'line', 'path', 'polygon', 'polyline', 'rect'
    ],

    CLASS_STRUCTURAL: [
        // https://svgwg.org/svg2-draft/intro.html#TermStructuralElement
        'defs', 'g', 'svg', 'symbol', 'use'
    ],

    CLASS_STRUCTURALLY_EXTERNAL_ELEMENTS: [
        // https://svgwg.org/svg2-draft/intro.html#TermStructurallyExternalElement
        'audio', 'foreignObject', 'iframe', 'image', 'script', 'use', 'video'
    ],

    CLASS_TEXT_CONTENT_ELEMENTS: [
        // https://svgwg.org/svg2-draft/text.html#TermTextContentElement
        'text', 'textPath', 'tspan'
    ],

    CLASS_TEXT_CONTENT_BLOCK_ELEMENTS: [
        // https://svgwg.org/svg2-draft/text.html#TermTextContentBlockElement
        'text'
    ],

    CLASS_TEXT_CONTENT_CHILD_ELEMENTS: [
        // https://svgwg.org/svg2-draft/text.html#TermTextContentChildElement
        'textPath', 'tspan'
    ],
};
