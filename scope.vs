uniform float frac;

void main(void)
{
	vec2 pos = (vec2(gl_Vertex.x, gl_Vertex.y) * frac) + (vec2(gl_Vertex.z, gl_Vertex.w) * (1.0 - frac));
	gl_Position = vec4((pos.x - 384.0) / 384.0, (384.0 - pos.y) / 384.0, 1, 1);
	gl_TexCoord[0] = gl_MultiTexCoord0;
}

