#version 330 core

uniform mat4 Transform;
layout(location=0) in vec2 Vertex;
invariant out vec4 gl_Position;

void main() {
    gl_Position = Transform*vec4(Vertex,1,1);
}
