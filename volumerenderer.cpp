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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	window_ = SDL_CreateWindow("Volume texture visualizer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1280, 720,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	assert(window_);

	auto context = SDL_GL_CreateContext(window_);
	glewInit();
}

std::string GetCompileLog(GLuint shader) {
	GLint shaderLogSize;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shaderLogSize);
	std::string log = std::string(shaderLogSize, 'x');
	glGetShaderInfoLog(shader, log.length(), &shaderLogSize, (GLchar*)log.data());
	return log;
}

GLuint program_;
void CompileAndLinkShaders()
{
	std::string vertexShaderStr =
		"#version 330 core\n"
		"layout(location = 0) in vec3 position;\n"
		"uniform mat4 mvp;\n"
		"void main () { \n"
		"	gl_Position = mvp * vec4(position, 1);\n"
		"}"
		;

	std::string fragmentShaderStr =
		"#version 330 core\n"
		"out vec3 color; \n"
		"void main() { \n"
		"	color = vec3(1, 0, 0);\n"
		"}"
		;

	auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
	{
		auto vertexShaderSrcPtr = vertexShaderStr.data();
		GLint length = vertexShaderStr.length();
		glShaderSource(vertexShader, 1, &vertexShaderSrcPtr, &length);
		glCompileShader(vertexShader);
		GLint compileStatus;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileStatus);
		if (compileStatus != GL_TRUE) {
			auto log = GetCompileLog(vertexShader);
			assert(false);
		}
	}
	auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	{
		auto fragmentShaderSrcPtr = fragmentShaderStr.data();
		GLint length = fragmentShaderStr.length();
		glShaderSource(fragmentShader, 1, &fragmentShaderSrcPtr, &length);
		glCompileShader(fragmentShader);
		GLint compileStatus;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileStatus);
		if (compileStatus != GL_TRUE) {
			auto log = GetCompileLog(fragmentShader);
			assert(false);
		}
	}

	program_ = glCreateProgram();
	glAttachShader(program_, vertexShader);
	glAttachShader(program_, fragmentShader);
	glLinkProgram(program_);
	GLint linkStatus;
	glGetProgramiv(program_, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE) {
		assert(false);
	}
	glUseProgram(program_);
}

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

uint16_t cubeIndices[] = {
	0, 6, 2, 0, 4, 6, // Front face
	4, 7, 6, 4, 5, 7, // right face
	5, 3, 7, 5, 1, 3, // back face,
	1, 2, 3, 1, 0, 2, // left face
	2, 7, 3, 2, 6, 7, // top face
	1, 4, 0, 1, 5, 4 // bottom face
};

GLuint vertexBuffer_;
GLuint indexBuffer_;
void CreateVertexBuffers() {

	glGenBuffers(1, &vertexBuffer_);
	glGenBuffers(1, &indexBuffer_);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
}

int main(int argc, char** argv)
{
	SetupWindow();

	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	CompileAndLinkShaders();
	CreateVertexBuffers();

	auto projection = glm::perspective(90.0f, 1280.0f / 720, 1.0f, 100.0f);
	auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	auto translate = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -1.0f));
	auto mvp = projection * view * translate;

	auto mvpLoc = glGetUniformLocation(program_, "mvp");
	glUniformMatrix4fv(mvpLoc, 1, false, (GLfloat*)&mvp);

	auto positionLoc = glGetAttribLocation(program_, "position");
	glEnableVertexAttribArray(positionLoc);
	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);

	glClearColor(0.15f, 0.15f, 0.20f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

	SDL_GL_SwapWindow(window_);
	while (true) {
	}

	return 0;
}
