// volumerenderer.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "volumerenderer.h"

SDL_Window* window_;

void SetupWindow()
{
	// Setup SDL
	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	window_ = SDL_CreateWindow("Volume texture visualizer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1280, 720,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	assert(window_);

	auto context = SDL_GL_CreateContext(window_);
}

std::string vertexShaderStr =
"#version 330 core\n"
"layout(location = 0) in vec3 position;\n"
"uniform mat4 mvp;\n"
"void main () { \n"
"	gl_position = mvp * vec4(position, 1);\n"
"}"
;

std::string fragmentShaderStr =
"#version 330 core\n"
"out vec3 color; \n"
"void main() { \n"
"	color = vec3(1, 0, 0);\n"
"}"
;

int main(int argc, char** argv)
{
	SetupWindow();

	auto projection = glm::perspective(90.0f, 1280.0f / 720, 1.0f, 100.0f);

	auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f), glm::vec3(.0f, 1.0f, 0.0f));

	glm::vec3 cubeVerts[] = {
		{ -1.0f, -1.0f, -1.0f }, // Left Bottom Front
		{ -1.0f, -1.0f, 1.0f }, // Left bottom back
		{ -1.0f, 1.0f, -1.0f }, // left top front
		{ -1.0f, 1.0f, 1.0f }, // left top back
		{ 1.0f, -1.0f, -1.0f }, // right bottom front
		{ 1.0f, -1.0f, 1.0f }, // right bottom back
		{ 1.0f, 1.0f, -1.0f }, // right top front
		{ 1.0f, 1.0f, 1.0f } // right top back
	};

	int indices[] = {
		0, 6, 2, 0, 4, 6, // Front face
		4, 7, 6, 4, 5, 7, // right face
		5, 3, 7, 5, 1, 3, // back face,
		1, 2, 3, 1, 0, 2, // left face
		2, 7, 3, 2, 6, 7, // top face
		1, 4, 0, 1, 5, 4 // bottom face
	};

	while (true) {
		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(window_);
	}

	return 0;
}
