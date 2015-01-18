#define GL_GLEXT_PROTOTYPES
#include <stdio.h>
#include <math.h>
#include <SDL.h>
#include <GL/gl.h>

#define SCOPE_SIZE 768
#define LINE_SEGMENTS 24


struct Vertex
{
	float startX, startY, endX, endY;
	float u, v;
};

Vertex verts[4096 * 4];
GLuint vertbuf;

SDL_Surface* g_screen;
short* g_audioData;
long g_audioPos, g_audioSamples;
long g_renderPos;

GLuint scopeProg, scopeFrac, scopeTexture, scopeFrameBuffer;
GLuint downsampleXTexture, downsampleXFrameBuffer, downsampleXProg, downsampleXImage;
GLuint downsampleYTexture, downsampleYFrameBuffer, downsampleYProg, downsampleYImage;
GLuint bloomXTexture, bloomXFrameBuffer, bloomXProg, bloomXImage;
GLuint bloomYTexture, bloomYFrameBuffer, bloomYProg, bloomYImage;
GLuint finalProg, finalScope, finalBloom;


GLuint LoadShader(const char* psFile, const char* vsFile)
{
	FILE* fp = fopen(psFile, "r");
	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* psSource = (char*)malloc(len + 1);
	fread(psSource, 1, len, fp);
	psSource[len] = 0;
	fclose(fp);

	fp = fopen(vsFile, "r");
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* vsSource = (char*)malloc(len + 1);
	fread(vsSource, 1, len, fp);
	vsSource[len] = 0;
	fclose(fp);

	GLuint pid = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(pid, 1, (const GLchar**)&psSource, NULL);
	glCompileShader(pid);

	int status;
	glGetShaderiv(pid, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		char msg[4096];
		printf("Pixel shader (%s) compile failed\n", psFile);
		glGetShaderInfoLog(pid, 4095, NULL, msg);
		msg[4095] = 0;
		printf("%s\n", msg);
		return 0;
	}

	GLuint vid = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vid, 1, (const GLchar**)&vsSource, NULL);
	glCompileShader(vid);

	glGetShaderiv(vid, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		char msg[4096];
		printf("Vertex shader (%s) compile failed\n", vsFile);
		glGetShaderInfoLog(vid, 4095, NULL, msg);
		msg[4095] = 0;
		printf("%s\n", msg);
		return 0;
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, pid);
	glAttachShader(prog, vid);
	glLinkProgram(prog);

	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		char msg[4096];
		printf("Shader link failed (%s, %s)\n", psFile, vsFile);
		glGetProgramInfoLog(prog, 4095, NULL, msg);
		msg[4095] = 0;
		printf("%s\n", msg);
		return 0;
	}

	return prog;
}


bool CreateRenderTarget(unsigned int w, unsigned int h, GLuint* texture, GLuint* target)
{
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCOPE_SIZE, SCOPE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenFramebuffersEXT(1, target);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, *target);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, *texture, 0);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		printf("Invalid texture render target\n");
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	return true;
}


void AudioCallback(void* params, unsigned char* stream, int len)
{
	short* dest = (short*)stream;

	for (int i = 0; i < (len / 2); i++)
	{
		if (g_audioPos >= g_audioSamples)
			dest[i] = 0;
		else
			dest[i] = g_audioData[g_audioPos++];
	}
}


void Render()
{
	long audioPos = g_audioPos;
	bool updated = false;
	while ((g_renderPos + 2048) <= audioPos)
	{
		int prevX = g_audioData[g_renderPos];
		int prevY = g_audioData[g_renderPos + 1];

		int n = 0;
		for (int i = 0; i < 4096; i += 2)
		{
			int curX = g_audioData[g_renderPos + i];
			int curY = g_audioData[g_renderPos + i + 1];

			float startX = prevX;
			float startY = prevY;
			float endX = curX;
			float endY = curY;
			startX = (SCOPE_SIZE - 1) - (((startX * SCOPE_SIZE) / 0xffff) +
				(SCOPE_SIZE / 2));
			startY = ((startY * SCOPE_SIZE) / 0xffff) +
				(SCOPE_SIZE / 2);
			endX = (SCOPE_SIZE - 1) - (((endX * SCOPE_SIZE) / 0xffff) +
				(SCOPE_SIZE / 2));
			endY = ((endY * SCOPE_SIZE) / 0xffff) +
				(SCOPE_SIZE / 2);

			verts[n].startX = startX - 2;  verts[n].startY = startY - 2;
			verts[n].endX = endX - 2;  verts[n].endY = endY - 2;
			verts[n].u = -2;  verts[n].v = -2;

			verts[n + 1].startX = startX + 2;  verts[n + 1].startY = startY - 2;
			verts[n + 1].endX = endX + 2;  verts[n + 1].endY = endY - 2;
			verts[n + 1].u = 2;  verts[n + 1].v = -2;

			verts[n + 2].startX = startX + 2;  verts[n + 2].startY = startY + 2;
			verts[n + 2].endX = endX + 2;  verts[n + 2].endY = endY + 2;
			verts[n + 2].u = 2;  verts[n + 2].v = 2;

			verts[n + 3].startX = startX - 2;  verts[n + 3].startY = startY + 2;
			verts[n + 3].endX = endX - 2;  verts[n + 3].endY = endY + 2;
			verts[n + 3].u = -2;  verts[n + 3].v = 2;

			n += 4;
			prevX = curX;
			prevY = curY;
		}

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, scopeFrameBuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_ONE, GL_ONE);
		glUseProgram(scopeProg);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(4, GL_FLOAT, sizeof(Vertex), (void*)(size_t)0);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(float) * 4));

		for (int i = 0; i <= LINE_SEGMENTS; i++)
		{
			glUniform1f(scopeFrac, (float)i / (float)LINE_SEGMENTS);
			glDrawArrays(GL_QUADS, 0, 4096 * 4);
		}

		g_renderPos += 2048;
		updated = true;
	}

	if (!updated)
	{
		// No updates, don't need to re-render
		SDL_Delay(10);
		return;
	}

	// Downsample in the X direction
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, downsampleXFrameBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(downsampleXProg);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, scopeTexture);
	glUniform1i(downsampleXImage, 0);
	glBegin(GL_QUADS);
		glVertex2f(0, 0); glTexCoord2f(1, 1);
		glVertex2f(SCOPE_SIZE, 0); glTexCoord2f(1, 0);
		glVertex2f(SCOPE_SIZE, SCOPE_SIZE); glTexCoord2f(0, 0);
		glVertex2f(0, SCOPE_SIZE); glTexCoord2f(0, 1);
	glEnd();

	// Downsample in the Y direction
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, downsampleYFrameBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(downsampleYProg);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, downsampleXTexture);
	glUniform1i(downsampleYImage, 0);
	glBegin(GL_QUADS);
		glVertex2f(0, 0); glTexCoord2f(1, 1);
		glVertex2f(SCOPE_SIZE, 0); glTexCoord2f(1, 0);
		glVertex2f(SCOPE_SIZE, SCOPE_SIZE); glTexCoord2f(0, 0);
		glVertex2f(0, SCOPE_SIZE); glTexCoord2f(0, 1);
	glEnd();

	// Blur in the X direction
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, bloomXFrameBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(bloomXProg);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, downsampleYTexture);
	glUniform1i(bloomXImage, 0);
	glBegin(GL_QUADS);
		glVertex2f(0, 0); glTexCoord2f(1, 1);
		glVertex2f(SCOPE_SIZE, 0); glTexCoord2f(1, 0);
		glVertex2f(SCOPE_SIZE, SCOPE_SIZE); glTexCoord2f(0, 0);
		glVertex2f(0, SCOPE_SIZE); glTexCoord2f(0, 1);
	glEnd();

	// Blur in the Y direction
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, bloomYFrameBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(bloomYProg);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bloomXTexture);
	glUniform1i(bloomYImage, 0);
	glBegin(GL_QUADS);
		glVertex2f(0, 0); glTexCoord2f(1, 1);
		glVertex2f(SCOPE_SIZE, 0); glTexCoord2f(1, 0);
		glVertex2f(SCOPE_SIZE, SCOPE_SIZE); glTexCoord2f(0, 0);
		glVertex2f(0, SCOPE_SIZE); glTexCoord2f(0, 1);
	glEnd();

	// Draw final output to screen
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(finalProg);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, scopeTexture);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bloomYTexture);
	glUniform1i(finalScope, 0);
	glUniform1i(finalBloom, 1);
	glBegin(GL_QUADS);
		glVertex2f(0, 0); glTexCoord2f(1, 1);
		glVertex2f(SCOPE_SIZE, 0); glTexCoord2f(1, 0);
		glVertex2f(SCOPE_SIZE, SCOPE_SIZE); glTexCoord2f(0, 0);
		glVertex2f(0, SCOPE_SIZE); glTexCoord2f(0, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	SDL_GL_SwapBuffers();
}


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: soundscope <input>\n");
		return 1;
	}

	FILE* fp = fopen(argv[1], "rb");
	if (!fp)
	{
		printf("Unable to open input file\n");
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	g_audioData = (short*)malloc(len);
	g_audioPos = 0;
	g_audioSamples = len / 2;
	g_renderPos = 0;
	fread(g_audioData, 1, len, fp);
	fclose(fp);

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("SDL init failed\n");
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
	g_screen = SDL_SetVideoMode(SCOPE_SIZE, SCOPE_SIZE, 24, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL);
	if (!g_screen)
	{
		printf("Set video mode failed\n");
		return 1;
	}

	glClearColor(0, 0, 0, 0);
	glViewport(0, 0, SCOPE_SIZE, SCOPE_SIZE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCOPE_SIZE, SCOPE_SIZE, 0, 1, -1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glGenBuffers(1, &vertbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vertbuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), NULL, GL_STREAM_DRAW);

	CreateRenderTarget(SCOPE_SIZE, SCOPE_SIZE, &scopeTexture, &scopeFrameBuffer);
	CreateRenderTarget(SCOPE_SIZE / 8, SCOPE_SIZE, &downsampleXTexture, &downsampleXFrameBuffer);
	CreateRenderTarget(SCOPE_SIZE / 8, SCOPE_SIZE / 8, &downsampleYTexture, &downsampleYFrameBuffer);
	CreateRenderTarget(SCOPE_SIZE / 8, SCOPE_SIZE / 8, &bloomXTexture, &bloomXFrameBuffer);
	CreateRenderTarget(SCOPE_SIZE / 8, SCOPE_SIZE / 8, &bloomYTexture, &bloomYFrameBuffer);

	scopeProg = LoadShader("scope.ps", "scope.vs");
	scopeFrac = glGetUniformLocation(scopeProg, "frac");

	downsampleXProg = LoadShader("downsample_x.ps", "downsample_x.vs");
	downsampleXImage = glGetUniformLocation(downsampleXProg, "image");
	downsampleYProg = LoadShader("downsample_y.ps", "downsample_y.vs");
	downsampleYImage = glGetUniformLocation(downsampleYProg, "image");
	bloomXProg = LoadShader("bloom_x.ps", "bloom_x.vs");
	bloomXImage = glGetUniformLocation(bloomXProg, "image");
	bloomYProg = LoadShader("bloom_y.ps", "bloom_y.vs");
	bloomYImage = glGetUniformLocation(bloomYProg, "image");
	finalProg = LoadShader("final.ps", "final.vs");
	finalScope = glGetUniformLocation(finalProg, "scope");
	finalBloom = glGetUniformLocation(finalProg, "bloom");

	SDL_AudioSpec desired, actual;
	desired.freq = 44100;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 1470;
	desired.callback = AudioCallback;
	desired.userdata = NULL;
	if (SDL_OpenAudio(&desired, &actual) != 0)
	{
		printf("Open audio failed\n");
		return 1;
	}

	SDL_PauseAudio(0);

	bool quit = false;
	while (!quit)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			default:
				break;
			}
		}

		Render();
	}

	return 0;
}

