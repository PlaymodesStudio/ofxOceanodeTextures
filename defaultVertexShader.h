R"(
#version 410

uniform mat4 modelViewProjectionMatrix;
layout(location = 0) in vec4  position;

void main(){
    gl_Position = modelViewProjectionMatrix * position;
}
)"
