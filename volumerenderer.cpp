// volumerenderer.cpp : Defines the entry point for the console application.
//
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

SDL_Window* window_;
GLuint program_;
GLint mvpLoc_;
GLuint vertexBuffer_;
GLuint indexBuffer_;
glm::mat4 projection_;
glm::mat4 model_;

float viewAngleV_ = 0.0f;
float viewAngleH_ = 0.0f;

struct {
	bool showAppAbout = false;
	float cameraSensitivity = 0.1f;
	float cameraFov = 60.0f;
	glm::vec4 backgroundColor = glm::vec4(0.15f, 0.15f, 0.20f, 1.0f);
} imguiSettings_;

glm::vec2 windowSize_ = glm::vec2(1280, 720);

void SetupWindow()
{
	// Setup SDL
	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	window_ = SDL_CreateWindow("Volume texture visualizer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowSize_.x, windowSize_.y,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	assert(window_);

	auto context = SDL_GL_CreateContext(window_);
	glewInit();

	auto imguiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplSdlGL3_Init(window_);
	ImGui::StyleColorsDark();
}

std::string GetCompileLog(GLuint shader) {
	GLint shaderLogSize;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shaderLogSize);
	std::string log = std::string(shaderLogSize, 'x');
	glGetShaderInfoLog(shader, log.length(), &shaderLogSize, (GLchar*)log.data());
	return log;
}

void CompileAndLinkShaders()
{
	std::string vertexShaderStr =
		"#version 330 core\n"
		"layout(location = 0) in vec3 position;\n"
		"out vec3 vertColor;\n"
		"uniform mat4 mvp;\n"
		"void main () { \n"
		"	gl_Position = mvp * vec4(position, 1);\n"
		"	vertColor = (position + 1)/2;\n"
		"}"
		;

	std::string fragmentShaderStr =
		"#version 330 core\n"
		"in vec3 vertColor;\n"
		"out vec3 color; \n"
		"void main() { \n"
		"	color = vertColor;\n"
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

	mvpLoc_ = glGetUniformLocation(program_, "mvp");
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

void CreateVertexBuffers() {

	glGenBuffers(1, &vertexBuffer_);
	glGenBuffers(1, &indexBuffer_);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

	auto positionLoc = glGetAttribLocation(program_, "position");
	glEnableVertexAttribArray(positionLoc);
	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);
}

void PostResizeGlSetup() {
	auto aspect = windowSize_.x / windowSize_.y;
	projection_ = glm::perspective(glm::radians(imguiSettings_.cameraFov), aspect, 1.0f, 10.0f);

	glViewport(0, 0, windowSize_.x, windowSize_.y);
}

void HandleWindowResize(int width, int height) {
	windowSize_.x = width;
	windowSize_.y = height;

	PostResizeGlSetup();
}

void SetupGLState()
{
	CompileAndLinkShaders();
	CreateVertexBuffers();

	PostResizeGlSetup();
}

void RenderMenus()
{
	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always, ImVec2(0, 0));
	if (ImGui::Begin("Window", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_AlwaysAutoResize)
	) {
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				ImGui::MenuItem("About", NULL, &imguiSettings_.showAppAbout);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (ImGui::CollapsingHeader("Camera Controls"))
		{
			ImGui::SliderFloat("Mouse sensitivity", &imguiSettings_.cameraSensitivity, 0.01f, 0.5f);
			if (ImGui::SliderFloat("FOV", &imguiSettings_.cameraFov, 10.0f, 150.0f))
			{
				PostResizeGlSetup();
			}
		}

		if (ImGui::CollapsingHeader("Pretty"))
		{
			ImGui::ColorPicker3("Background Color", (float*)&imguiSettings_.backgroundColor);
		}

		if (imguiSettings_.showAppAbout)
		{
			ImGui::Begin("About", &imguiSettings_.showAppAbout, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Text("Volume Texture Visualizer", ImGui::GetVersion());
			ImGui::Separator();
			ImGui::Text("By Chris Varnsverry.");
			ImGui::End();
		}

		ImGui::End();
	}
}

void Render() {
	ImGui_ImplSdlGL3_NewFrame(window_);

	glClearColor(
		imguiSettings_.backgroundColor.r,
		imguiSettings_.backgroundColor.g,
		imguiSettings_.backgroundColor.b,
		imguiSettings_.backgroundColor.a
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);


	auto viewPos = glm::vec3(0.0f, 0.0f, 2.0f);
	viewPos = glm::rotate(viewPos, glm::radians(viewAngleV_), glm::vec3(1.0f, 0.0f, 0.0f));
	viewPos = glm::rotate(viewPos, glm::radians(viewAngleH_), glm::vec3(0.0f, 1.0f, 0.0f));
	
	auto view = glm::lookAt(viewPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	auto scale = glm::scale(glm::mat4(), glm::vec3(0.5f));
	auto translate = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	auto mvp = projection_ * view * translate * scale;
	glUniformMatrix4fv(mvpLoc_, 1, false, (GLfloat*)&mvp);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

	ImGui::ShowDemoWindow(nullptr);
	RenderMenus();
	ImGui::Render();
	ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(window_);
}

void HandleWindowEvent(const SDL_Event& event) {
	switch (event.window.event)
	{
	case SDL_WINDOWEVENT_RESIZED:
		HandleWindowResize(event.window.data1, event.window.data2);
		break;
	default:
		break;
	}
}

void MessagePump() {
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSdlGL3_ProcessEvent(&event);
		auto& io = ImGui::GetIO();
		switch (event.type)
		{
		case SDL_QUIT:
			exit(0);
			break;

		case SDL_WINDOWEVENT:
			HandleWindowEvent(event);
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (!io.WantCaptureMouse && event.button.button == SDL_BUTTON_LEFT) SDL_SetRelativeMouseMode(SDL_TRUE);
			break;
		case SDL_MOUSEBUTTONUP:
			if (!io.WantCaptureMouse && event.button.button == SDL_BUTTON_LEFT) SDL_SetRelativeMouseMode(SDL_FALSE);
			break;
		case SDL_MOUSEMOTION:
			if (!io.WantCaptureMouse && SDL_GetRelativeMouseMode())
			{
				viewAngleH_ += imguiSettings_.cameraSensitivity * event.motion.xrel;
				viewAngleV_ += imguiSettings_.cameraSensitivity * event.motion.yrel;
				viewAngleV_ = glm::clamp(viewAngleV_, -85.f, 85.f);
			}
			break;
		/*case SDL_KEYDOWN:
			if (!io.WantCaptureMouse)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_r:
					++m_RenderMode;
					break;
				}
			}
			break;*/
		default:
			break;
		}
	}
}

int main(int argc, char** argv)
{
	SetupWindow();

	SetupGLState();

	while (true) {
		MessagePump();
		Render();
	}

	return 0;
}
