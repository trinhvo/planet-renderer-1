#version 130

attribute vec3 aVertex;

// Used in the vertex transformation
uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;
uniform mat4 uNMatrix;

// Used in the clipmap
uniform vec2 uScale;
uniform vec2 uOffset;
uniform vec4 uFineBlockOrig;
uniform sampler2D uHeightmap;

uniform float uHeightScale;

// Used in the fragment shader
varying vec2 vDecalTexCoord;
varying vec4 vColor;

const float PI = 3.1415926;

// Transform a xz-plane to sphere
vec3 sphere(float radius, vec2 xz, float y) {
    vec2 uv = xz * 2 * PI;
    float theta = uv.x;
    float phi = uv.y;

    float rho = radius + y;
    return vec3(
            rho * cos(theta) * cos(phi),
            rho * sin(phi),
            rho * sin(theta) * cos(phi)
    );
}

void main()
{
    vec2 worldPos = aVertex.xy * uScale + uOffset;
    vec2 uv = (aVertex.xy) * uFineBlockOrig.xy + uFineBlockOrig.zw;
    float height = texture2D(uHeightmap, uv).x * uHeightScale;

    vec4 pos = uPMatrix * uMVMatrix * vec4(worldPos.x, height, worldPos.y, 1.0);
    // pos.z -= 5.0;

    gl_Position = pos;

    vDecalTexCoord = uv;
    vColor = vec4(height / uHeightScale, 1.0 - height / uHeightScale, 0.0, 1.0);
}
