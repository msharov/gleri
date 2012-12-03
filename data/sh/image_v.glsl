#version 330 core

uniform mat4 Transform;
layout(location=0) in vec2 Vertex;
layout(location=1) in vec2 TexCoord;
invariant out vec2 f_tex;
invariant out vec4 gl_Position;

void main() {
    gl_Position = Transform*vec4(Vertex,1,1);
    f_tex = TexCoord/(1<<14);
}
