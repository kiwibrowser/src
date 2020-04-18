uniform mat4 projection;
uniform mat4 modelview;
uniform vec3 lightPosition;
uniform vec3 logoDirection;
uniform float currentTime;
attribute vec3 vertex;
varying vec4 color;

void main()
{
    vec4 curVertex = vec4(vertex.x, vertex.y, vertex.z, 1.0);
    gl_Position = projection * modelview * curVertex;
    float referenceTime = 15.0;
    vec3 lightDirection = normalize(lightPosition - vertex);    
    float c = max(dot(lightDirection, logoDirection), 0.0);
    c = c * c * c * lightDirection.y;
    if ((currentTime > referenceTime - 5.0) && (currentTime < referenceTime - 3.0))
    {
        c *= 1.0 - (currentTime - (referenceTime - 5.0)) * 0.5;
    }
    color = vec4(c, c, (c * 0.78125), 1.0);
}
