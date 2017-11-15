#version 330 core

uniform sampler2D Texture;
in vec2 f_tex;
out vec4 FragColor;

void main() {
    FragColor = texture2D(Texture,f_tex);
}
