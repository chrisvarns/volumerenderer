// volumerenderer.cpp : Defines the entry point for the console application.
//
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

SDL_Window* window_;
GLuint program_;
GLint mvpLoc_;
GLint positionLoc_;
GLuint cubeVertexBuffer_;
GLuint cubeIndexBuffer_;
GLuint edgesVertexBuffer_;
GLuint intersectionPointBuffer_;
GLuint intersectionTriangleBuffer_;
int numIntersectionPoints_;
int numIntersectionTriangles_;
glm::mat4 projection_;
glm::mat4 model_;

float viewAngleV_ = 0.0f;
float viewAngleH_ = 0.0f;

struct {
	bool showAppAbout = false;
	bool drawCube = true;
	bool drawEdges = false;
	bool drawIntersectionPoints = false;
	bool drawIntersectionGeometry = false;
	bool updateIntersections = true;
	bool fullscreen = false;
	uint32_t cubeNumSlices = 10;
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

const glm::vec3 cubePos_LBF = { -1.0f, -1.0f, -1.0f };
const glm::vec3 cubePos_LBB = { -1.0f, -1.0f, 1.0f };
const glm::vec3 cubePos_LTF = { -1.0f, 1.0f, -1.0f };
const glm::vec3 cubePos_LTB = { -1.0f, 1.0f, 1.0f };
const glm::vec3 cubePos_RBF = { 1.0f, -1.0f, -1.0f };
const glm::vec3 cubePos_RBB = { 1.0f, -1.0f, 1.0f };
const glm::vec3 cubePos_RTF = { 1.0f, 1.0f, -1.0f };
const glm::vec3 cubePos_RTB = { 1.0f, 1.0f, 1.0f };

std::vector<glm::vec3> cubeVerts_ = {
	cubePos_LBF,
	cubePos_LBB,
	cubePos_LTF,
	cubePos_LTB,
	cubePos_RBF,
	cubePos_RBB,
	cubePos_RTF,
	cubePos_RTB
};

std::vector<uint16_t> cubeIndices_ = {
	0, 6, 2, 0, 4, 6, // Front face
	4, 7, 6, 4, 5, 7, // right face
	5, 3, 7, 5, 1, 3, // back face,
	1, 2, 3, 1, 0, 2, // left face
	2, 7, 3, 2, 6, 7, // top face
	1, 4, 0, 1, 5, 4 // bottom face
};

const glm::vec3 rayDir_Back = { 0.0f, 0.0f, 2.0f };
const glm::vec3 rayDir_Up = { 0.0f, 2.0f, 0.0f };
const glm::vec3 rayDir_Right = { 2.0f, 0.0f, 0.0f };

std::vector<glm::vec3> edgeVerts_ = {
	{ cubePos_LBF }, { cubePos_LBF + rayDir_Back },
	{ cubePos_LBF }, { cubePos_LBF + rayDir_Up },
	{ cubePos_LBF }, { cubePos_LBF + rayDir_Right },
	{ cubePos_LTF }, { cubePos_LTF + rayDir_Back },
	{ cubePos_LTF }, { cubePos_LTF + rayDir_Right },
	{ cubePos_RBF }, { cubePos_RBF + rayDir_Back },
	{ cubePos_RBF }, { cubePos_RBF + rayDir_Up },
	{ cubePos_RTF }, { cubePos_RTF + rayDir_Back },
	{ cubePos_LBB }, { cubePos_LBB + rayDir_Up },
	{ cubePos_LBB }, { cubePos_LBB + rayDir_Right },
	{ cubePos_LTB }, { cubePos_LTB + rayDir_Right },
	{ cubePos_RBB }, { cubePos_RBB + rayDir_Up }
};

void CreateVertexBuffers() {

	glGenBuffers(1, &cubeVertexBuffer_);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBuffer_);
	glBufferData(GL_ARRAY_BUFFER, cubeVerts_.size() * sizeof(*cubeVerts_.data()), cubeVerts_.data(), GL_DYNAMIC_DRAW);

	glGenBuffers(1, &cubeIndexBuffer_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIndexBuffer_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices_.size() * sizeof(*cubeIndices_.data()), cubeIndices_.data(), GL_DYNAMIC_DRAW);

	glGenBuffers(1, &edgesVertexBuffer_);
	glBindBuffer(GL_ARRAY_BUFFER, edgesVertexBuffer_);
	glBufferData(GL_ARRAY_BUFFER, edgeVerts_.size() * sizeof(*edgeVerts_.data()), edgeVerts_.data(), GL_DYNAMIC_DRAW);

	glGenBuffers(1, &intersectionPointBuffer_);
	glGenBuffers(1, &intersectionTriangleBuffer_);

	positionLoc_ = glGetAttribLocation(program_, "position");
	glEnableVertexAttribArray(positionLoc_);
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

		if (ImGui::CollapsingHeader("Draw"))
		{
			ImGui::Checkbox("Cube", &imguiSettings_.drawCube);
			ImGui::Checkbox("Edges", &imguiSettings_.drawEdges);
			ImGui::Checkbox("Intersection points", &imguiSettings_.drawIntersectionPoints);
			ImGui::Checkbox("Intersection geometry", &imguiSettings_.drawIntersectionGeometry);
		}

		if (ImGui::CollapsingHeader("Update"))
		{
			ImGui::Checkbox("Intersections", &imguiSettings_.updateIntersections);
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

void UpdateIntersections(glm::mat4 modelView) {
	struct EdgeRay {
		glm::vec3 origin;
		glm::vec3 direction;
	};
	std::vector<EdgeRay> edgeRays;
	// x == nearPlane, y == farPlane
	glm::vec2 depthRange = glm::vec2(-100.f, +100.f);

	auto edgeVertItr = edgeVerts_.cbegin();
	while (edgeVertItr != edgeVerts_.cend()) {

		auto startPoint = modelView * glm::vec4(*edgeVertItr++, 1.0f);
		auto endPoint = modelView * glm::vec4(*edgeVertItr++, 1.0f);
		depthRange.x = glm::max(depthRange.x, startPoint.z);
		depthRange.y = glm::min(depthRange.y, startPoint.z);
		depthRange.x = glm::max(depthRange.x, endPoint.z);
		depthRange.y = glm::min(depthRange.y, endPoint.z);

		EdgeRay edgeRay;
		edgeRay.origin = startPoint;
		edgeRay.direction = endPoint - startPoint;
		edgeRays.push_back(edgeRay);
	}

	float depthDiff = depthRange.x - depthRange.y;
	float sliceDiff = depthDiff / imguiSettings_.cubeNumSlices;
	float offsetToFirstPlane = sliceDiff / 2;
	auto viewToWorld = glm::inverse(modelView);

	std::vector<glm::vec3> intersectionPoints;
	std::vector<glm::vec3> intersectionTriangles;
	glm::vec3 n = glm::vec3(0.0f, 0.0f, 1.0f);
	float d = depthRange.y + offsetToFirstPlane;

	for (int planeIdx = 0; planeIdx < imguiSettings_.cubeNumSlices; planeIdx++) {
		std::vector<glm::vec3> intersectionPointsThisPlane;

		// Perform the intersection tests
		for (auto& ray : edgeRays) {
			float t = (d - glm::dot(ray.origin, n))
				/ glm::dot(ray.direction, n);
			if (t > 0.0f && t < 1.0f) {
				intersectionPointsThisPlane.push_back(ray.origin + ray.direction * t);
			}
		}

		// Find center point
		glm::vec2 centerPoint = glm::vec2(0.0f);
		for (auto& point : intersectionPointsThisPlane)
		{
			centerPoint += glm::vec2(point);
		}
		centerPoint /= intersectionPointsThisPlane.size();

		// Sort them in anti-clockwise order
		std::sort(intersectionPointsThisPlane.begin(), intersectionPointsThisPlane.end(),
			[centerPoint](const glm::vec3& a, const glm::vec3& b) -> bool
		{
			auto posX = glm::vec2(1.0f, 0.0f);
			auto angleA = glm::orientedAngle(glm::normalize(glm::vec2(a) - centerPoint), posX);
			auto angleB = glm::orientedAngle(glm::normalize(glm::vec2(b) - centerPoint), posX);
			return angleA > angleB;
		});

		// Convert everything to world space
		for (auto& point : intersectionPointsThisPlane) {
			point = viewToWorld * glm::vec4(point, 1.0f);
		}
		glm::vec3 worldSpaceCenterPoint = viewToWorld * glm::vec4(centerPoint, d, 1.0f);

		// Generate the geometry
		for (int i = 0; i < intersectionPointsThisPlane.size(); ++i)
		{
			auto& current = intersectionPointsThisPlane[i];
			auto& next = intersectionPointsThisPlane[(i + 1) % intersectionPointsThisPlane.size()];
			intersectionTriangles.push_back(current);
			intersectionTriangles.push_back(next);
			intersectionTriangles.push_back(worldSpaceCenterPoint);
		}

		intersectionPoints.insert(intersectionPoints.end(), intersectionPointsThisPlane.begin(), intersectionPointsThisPlane.end());
		d += sliceDiff;
	}

	// Update the points buffer
	glBindBuffer(GL_ARRAY_BUFFER, intersectionPointBuffer_);
	glBufferData(GL_ARRAY_BUFFER, intersectionPoints.size() * sizeof(*intersectionPoints.data()), intersectionPoints.data(), GL_DYNAMIC_DRAW);
	numIntersectionPoints_ = intersectionPoints.size();

	// Update the triangles buffer
	glBindBuffer(GL_ARRAY_BUFFER, intersectionTriangleBuffer_);
	glBufferData(GL_ARRAY_BUFFER, intersectionTriangles.size() * sizeof(*intersectionTriangles.data()), intersectionTriangles.data(), GL_DYNAMIC_DRAW);
	numIntersectionTriangles_ = intersectionTriangles.size();
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
	auto modelView = view * translate * scale;

	if(imguiSettings_.updateIntersections)
		UpdateIntersections(modelView);

	auto mvp = projection_ * modelView;

	if (imguiSettings_.drawCube) {
		glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBuffer_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIndexBuffer_);
		glVertexAttribPointer(positionLoc_, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);
		glUniformMatrix4fv(mvpLoc_, 1, false, (GLfloat*)&mvp);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	}
	if (imguiSettings_.drawEdges) {
		glBindBuffer(GL_ARRAY_BUFFER, edgesVertexBuffer_);
		glVertexAttribPointer(positionLoc_, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);
		glUniformMatrix4fv(mvpLoc_, 1, false, (GLfloat*)&mvp);
		glDrawArrays(GL_LINES, 0, 24);
	}
	if (imguiSettings_.drawIntersectionGeometry) {
		glBindBuffer(GL_ARRAY_BUFFER, intersectionTriangleBuffer_);
		glVertexAttribPointer(positionLoc_, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);
		glUniformMatrix4fv(mvpLoc_, 1, false, (GLfloat*)&mvp);
		glDrawArrays(GL_TRIANGLES, 0, numIntersectionTriangles_);
	}
	if (imguiSettings_.drawIntersectionPoints) {
		glBindBuffer(GL_ARRAY_BUFFER, intersectionPointBuffer_);
		glVertexAttribPointer(positionLoc_, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);
		glUniformMatrix4fv(mvpLoc_, 1, false, (GLfloat*)&mvp);
		glPointSize(10.0f);
		glDrawArrays(GL_POINTS, 0, numIntersectionPoints_);
	}

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
		case SDL_KEYDOWN:
			if (!io.WantCaptureMouse)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_RETURN:
					if (SDL_GetModState() && KMOD_ALT) {
						imguiSettings_.fullscreen = !imguiSettings_.fullscreen;
						SDL_SetWindowFullscreen(window_, imguiSettings_.fullscreen);
						PostResizeGlSetup();
					}
					break;
				}
			}
			break;
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
