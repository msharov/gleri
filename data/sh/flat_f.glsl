#version 330 core

uniform vec4 Color;
invariant out vec4 FragColor;

void main() {
    FragColor = Color;
}
