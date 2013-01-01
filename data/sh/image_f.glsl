#version 330 core

uniform sampler2D Texture;

invariant in vec2 f_tex;
invariant out vec4 FragColor;

void main() {
    FragColor = texture2D(Texture,f_tex);
}
