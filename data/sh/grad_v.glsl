#version 330 core

uniform mat4 Transform;
layout(location=0) in vec2 Vertex;
layout(location=1) in vec4 Color;
out vec4 f_color;

void main() {
    gl_Position = Transform*vec4(Vertex,1,1);
    f_color = Color/255;
}
