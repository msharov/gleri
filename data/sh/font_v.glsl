#version 330 core

uniform mat4 Transform;
uniform vec4 FontSize;
layout(location=2) in vec4 Vertex;
invariant out Sg { vec4 pos; vec4 tex; } g;

void main() {
    vec4 glyphtl = Transform*vec4(Vertex.xy,1,1);
    vec4 glyphbr = Transform*vec4(Vertex.xy+FontSize.xy,1,1);
    g.pos = vec4(glyphtl.xy,glyphbr.xy);

    vec2 textl = Vertex.zw+vec2(.5,.5);
    vec2 texbr = textl+FontSize.xy;
    g.tex = vec4(textl,texbr)/FontSize.zwzw;
}
