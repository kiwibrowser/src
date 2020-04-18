// Transformation matrices
uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;

// Vertex attributes
attribute vec3 position;
attribute vec3 normal;
attribute vec3 tangent;
attribute vec2 uv;

// Uniforms
uniform sampler2D tNormal;
uniform sampler2D tDisplacement;
uniform float uDisplacementScale;
uniform float uDisplacementBias;

// Varyings
varying vec3 vTangent;
varying vec3 vBinormal;
varying vec3 vNormal;
varying vec2 vUv;
varying vec3 vViewPosition;

void main()
{
    vec4 mvPosition = modelViewMatrix * vec4( position, 1.0 );
    vViewPosition = -mvPosition.xyz;
    vNormal = normalize(vec3(normalMatrix * vec4(normal, 1.0)));

    // tangent and binormal vectors
    vTangent = normalize(vec3(normalMatrix * vec4(tangent, 1.0)));
    vBinormal = cross( vNormal, vTangent );
    vBinormal = normalize( vBinormal );

    // texture coordinates
    vUv = uv;

    // displacement mapping
    vec3 dv = texture2D( tDisplacement, uv ).xyz;
    float df = uDisplacementScale * dv.x + uDisplacementBias;
    vec4 displacedPosition = vec4(vNormal.xyz * df, 0.0 ) + mvPosition;
    gl_Position = projectionMatrix * displacedPosition;

    vec3 normalTex = texture2D(tNormal, uv).xyz * 2.0 - 1.0;
    vNormal = vec3(normalMatrix * vec4(normalTex, 1.0));
}
