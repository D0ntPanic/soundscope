void main(void)
{
	gl_Position = vec4((gl_Vertex.x - 384.0) / 384.0, (384.0 - gl_Vertex.y) / 384.0, 1, 1);
	gl_TexCoord[0] = gl_MultiTexCoord0;
}

