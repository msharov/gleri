#version 330 core

uniform mat4 Transform;
uniform vec4 ImageRect;
uniform vec4 SpriteRect;

out GeomVertex { vec4 pos; vec4 tex; } g;

void main() {
    vec4 imgtl = Transform*vec4(ImageRect.xy,1,1);
    vec4 imgbr = Transform*vec4(ImageRect.xy+SpriteRect.zw,1,1);
    g.pos = vec4(imgtl.xy,imgbr.xy);
    g.tex = (vec4(SpriteRect.xy,SpriteRect.xy+SpriteRect.zw)
		*vec4(1,-1,1,-1)+vec4(.5,.5,.5,.5))
		    /ImageRect.zwzw+vec4(0,1,0,1);
}
