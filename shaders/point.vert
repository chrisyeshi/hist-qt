#version 330

uniform mat4 matVP;
uniform mat4 matModel;
uniform vec3 campos;
uniform float ptSize;

in vec4 posAttr;
in float valAttr;

out vec4 fVertex;

void main() {

    int mode = 1;
    if(mode == 1) { //particle draw mode
        float dist = distance((matModel * vec4(posAttr.xyz, 1.0)).xyz, campos);
        gl_PointSize = max(ptSize / dist, 1.0);
//        gl_PointSize = 5.0;
    }

    gl_Position = matVP * matModel * vec4(posAttr.xyz,1.0);

    fVertex = vec4(posAttr.xyz, valAttr);

}
