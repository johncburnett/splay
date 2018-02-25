#version 330

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform vec4 globalColor;

uniform sampler2D tex0; // position
uniform sampler2D tex1; // position
uniform sampler2D tex2; // normals
uniform sampler2D tex3; // normals

uniform float interp;
uniform float scale;
uniform float displace;
uniform vec4 pos0;
uniform vec4 pos1;
uniform vec4 cam;
uniform float bell;

uniform float scanY;
uniform float scanHeight;
uniform float scanX;
uniform float scanWidth;
uniform int runScan;
uniform int wrapScan;

in vec2 texcoord;

float gauss(float u){
    return exp(-pow(u,2)/(2.0*pow(bell,2)));
}

void main() {
    //texCoord = vec2(gl_MultiTexCoord0);
    //texCoord = gl_TexCoord[0];
    //texCoord = gl_TexCoord[0];
    vec2 texCoordVarying = texcoord;

    vec4 c0 = vec4(texture(tex0, texCoordVarying.xy).xyz * scale, 1.0) + pos0;
    vec4 c1 = vec4(texture(tex1, texCoordVarying.xy).xyz * scale, 1.0) + pos1;
    
    vec4 n0 = normalize(texture(tex2, texCoordVarying.xy)) * scale;
    vec4 n1 = normalize(texture(tex3, texCoordVarying.xy)) * scale;
    
    vec4 position = mix(c0, c1, interp);
    //vec4 distort = mix(n0, n1, interp) * displace;
    
    if (runScan > 0) {
        float lower = scanY - scanHeight;
        float upper = scanY;
        if (position.y > lower && position.y < upper) position += distort;
        else if (position.y > lower + scale && wrapScan > 0) position += distort;
        
//        float left  = scanX - scanWidth;
//        float right = scanX;
//        if (position.x > left && position.x < right) position += distort;
//        else if (position.x > left + scale && wrapScan > 0) position += distort;
    }
    //else position += distort;
    
    float dof_dist = abs(dot(cam.xyz, position.xyz) + cam.w)/(length(cam.xyz)+0.00001);
    dof_dist = 1.0-gauss(dof_dist);
    position += dof_dist;

    gl_Position = modelViewProjectionMatrix * position;
}
