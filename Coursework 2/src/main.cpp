#include "GLShader.h"
//#include <GL\GLU.h>
#include <GLFW\glfw3.h>
#include <chrono>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\common.hpp>
#include <glm\gtx\norm.hpp>
#include "Camera.h"
#include <fstream>
#include<random>
#include<cmath>
#include<chrono>
#include <iostream>
#include <ctime>
//#include <math.h>
#include <algorithm>
#include "Texture.h"
#include <glm\gtx\string_cast.hpp>
#include <omp.h>
#include <thread>

//The maximum particles to be simulated
#define MAX_PARTICLES 100

//Newtons gravitational constant, probably wont use this because of how weak gravity is 
#define G 6.673e-11

//The texture for each particle
Texture tex;
//Basic target camera 
Camera cam;

//Particle shader
GLShader shader;
GLFWwindow* window;

// Uniform locations for shader.
GLuint CameraRight_worldspace_ID;
GLuint CameraUp_worldspace_ID;
GLuint ViewProjMatrixID;

// Get the number of threads this hardware can support.
int numThreads = std::thread::hardware_concurrency();

// This class represents the particle.
struct Particle
{
	//Position of the particle.
	glm::vec3 pos;
	//Current Force acting on the particle
	glm::vec3 force;
	//Colour of the particle.
	unsigned char r, g, b, a;
	//Radius of the particle, not represented visually
	float size;
	//Velocity of the particle
	glm::vec3 velocity;
	//Particles mass
	float mass;

	//Caclulate the force that another particle is having on this particle.
	void AddForce(Particle& b)
	{

	}

	//Update the particle
	void Update(float deltaTime)
	{
		velocity += (force / mass);
		pos += deltaTime * velocity;
	}

	void ResetForce()
	{
		force = glm::dvec3(0);
	}

	bool operator<(const Particle& that) const
	{
		// Sort in reverse order : far particles drawn first.
		return this->pos.z > that.pos.z;
	}

	void Print()
	{
		std::cout << glm::to_string(pos) << std::endl;
	}
};

Particle Particles[MAX_PARTICLES];
static GLfloat* gl_pos_data = new GLfloat[MAX_PARTICLES * 4];
static GLubyte* gl_colour_data = new GLubyte[MAX_PARTICLES * 4];
GLuint pos_buffer;
GLuint colour_buffer;
GLuint vertex_buffer;

void Update(double deltaTime)
{
	cam.Update(deltaTime);
}


void Render()
{
	// Update the OpenGL buffers with updated particle positions.
	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, MAX_PARTICLES * sizeof(GLfloat) * 4, gl_pos_data);

	glBindBuffer(GL_ARRAY_BUFFER, colour_buffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, MAX_PARTICLES * sizeof(GLubyte) * 4, gl_colour_data);

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//	glClearColor(1, 1, 1, 1);
	glm::mat4 ProjectionMatrix = cam.GetProjection();
	glm::mat4 ViewMatrix = cam.GetView();
	glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	// Use our shader
	shader.Use();
	glUniform3f(CameraRight_worldspace_ID, ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
	glUniform3f(CameraUp_worldspace_ID, ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
	glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex.id);
	glUniform1i(glGetUniformLocation(shader.GetId(), "tex"), 1);


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : positions of particles' centers
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 3rd attribute buffer : particles' colors
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, colour_buffer);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);


	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, MAX_PARTICLES);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}


int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1920, 1080, "N-Body Simulation", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);


	// Create and compile our GLSL program from the shaders
	shader.SetProgram();
	shader.AddShaderFromFile("Quad.vert", GLShader::VERTEX);
	shader.AddShaderFromFile("Quad.frag", GLShader::FRAGMENT);
	shader.Link();


	cam.SetProjection(glm::quarter_pi<float>(), 1920 / 1080, 2.414f, 100000);
	cam.SetWindow(window);
	cam.SetPosition(glm::vec3(0, 0, 20));

	// Vertex shader
	CameraRight_worldspace_ID = glGetUniformLocation(shader.GetId(), "CameraRight_worldspace");
	CameraUp_worldspace_ID = glGetUniformLocation(shader.GetId(), "CameraUp_worldspace");
	ViewProjMatrixID = glGetUniformLocation(shader.GetId(), "VP");

	double lastTime = glfwGetTime();

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 generator(seed);
	std::uniform_real_distribution<double> uniform01(0.0, 1.0);
	
	for (int i = 1; i < MAX_PARTICLES; i++)
	{
		double x = rand() % 100;
		double y = rand() % 100;
		double z = rand() % 100;

		Particles[i].pos = glm::dvec3(x, y, z);
		Particles[i].velocity = glm::dvec3(0);
		Particles[i].r = rand() % 256;
		Particles[i].g = rand() % 256;
		Particles[i].b = rand() % 256;
		Particles[i].a = 255;
		Particles[i].mass = 1;
		Particles[i].size = 1;

		gl_colour_data[4 * i + 0] = Particles[i].r;
		gl_colour_data[4 * i + 1] = Particles[i].g;
		gl_colour_data[4 * i + 2] = Particles[i].b;
		gl_colour_data[4 * i + 3] = Particles[i].a;

	}

	tex = Texture("debug.png");

	static const GLfloat g_vertex_buffer_data[] =
	{
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
	};
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	//The VBO containing the positions and sizes of the particles
	glGenBuffers(1, &pos_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer);
	//Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	//The VBO containing the colors of the particles
	glGenBuffers(1, &colour_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, colour_buffer);
	//Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

	do
	{

		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		Update(delta);
		Render();
		lastTime = currentTime;

	}// Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);


	delete[] gl_pos_data;

	// Cleanup VBO and shader
	glDeleteBuffers(1, &colour_buffer);
	glDeleteBuffers(1, &pos_buffer);
	glDeleteBuffers(1, &vertex_buffer);
	glDeleteProgram(shader.GetId());
	glDeleteVertexArrays(1, &VertexArrayID);


	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}