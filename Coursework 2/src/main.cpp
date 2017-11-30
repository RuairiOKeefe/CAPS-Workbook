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
#define MAX_PARTICLES 1000
//Small value to prevent /0
#define SOFTENING 1e-9f
//Newtons gravitational constant, probably wont use this because of how weak gravity is 
#define G 6.673e-11f

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
	//Radius of the particle
	float radius;
	//Velocity of the particle
	glm::vec3 velocity;
	//Particles mass
	float mass;


	//Update the particle
	void Update(float deltaTime)
	{
		velocity += (force / mass);
		pos += deltaTime * velocity;
	}

};

GLuint VertexArrayID;
static GLfloat* gl_pos_data = new GLfloat[MAX_PARTICLES * 4];
static GLubyte* gl_colour_data = new GLubyte[MAX_PARTICLES * 4];
GLuint pos_buffer;
GLuint colour_buffer;
GLuint vertex_buffer;

Particle particles[MAX_PARTICLES];
double lastTime;

int Initialise()
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

	//Backgroud colour
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);


	// Create and compile our GLSL program from the shaders
	shader.SetProgram();
	shader.AddShaderFromFile("../res/shaders/Quad.vert", GLShader::VERTEX);
	shader.AddShaderFromFile("../res/shaders/Quad.frag", GLShader::FRAGMENT);
	shader.Link();


	cam.SetProjection(glm::quarter_pi<float>(), 1920 / 1080, 2.414f, 100000);
	cam.SetWindow(window);
	cam.SetPosition(glm::vec3(0, 0, 800));

	// Vertex shader
	CameraRight_worldspace_ID = glGetUniformLocation(shader.GetId(), "CameraRight_worldspace");
	CameraUp_worldspace_ID = glGetUniformLocation(shader.GetId(), "CameraUp_worldspace");
	ViewProjMatrixID = glGetUniformLocation(shader.GetId(), "VP");

	lastTime = glfwGetTime();

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 generator(seed);
	std::uniform_real_distribution<double> randomPos(0.0, 1.0);
	//need to use
	for (int i = 1; i < MAX_PARTICLES; i++)
	{
		double x = (rand() % 100) - 50;
		double y = (rand() % 100) - 50;
		double z = (rand() % 100) - 50;

		particles[i].pos = glm::dvec3(x, y, z);
		particles[i].velocity = glm::dvec3(0);
		particles[i].r = 0;
		particles[i].g = 100;
		particles[i].b = 255;
		particles[i].a = 255;
		particles[i].mass = 1;
		particles[i].radius = particles[i].mass;

		gl_colour_data[4 * i + 0] = particles[i].r;
		gl_colour_data[4 * i + 1] = particles[i].g;
		gl_colour_data[4 * i + 2] = particles[i].b;
		gl_colour_data[4 * i + 3] = particles[i].a;

	}

	tex = Texture("../res/textures/Particle.png");

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return 0;
}

void UpdateParticles(double deltaTime)
{
	//set somewhere more logical
	float dt = 0.01;
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		float fX = 0.0f; float fY = 0.0f; float fZ = 0.0f;

		for (int j = 0; j < MAX_PARTICLES; j++)
		{
			float dx = particles[j].pos.x - particles[i].pos.x;
			float dy = particles[j].pos.y - particles[i].pos.y;
			float dz = particles[j].pos.z - particles[i].pos.z;
			float distSqr = dx*dx + dy*dy + dz*dz + SOFTENING;
			float invDist = 1.0f / sqrtf(distSqr);
			float invDist3 = invDist * invDist * invDist;

			fX += (particles[i].mass * particles[j].mass) * dx * invDist3;
			fY += (particles[i].mass * particles[j].mass) * dy * invDist3;
			fZ += (particles[i].mass * particles[j].mass) * dz * invDist3;
		}

		particles[i].velocity.x += fX;
		particles[i].velocity.y += fY;
		particles[i].velocity.z += fZ;
	}

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		
		Particle& p = particles[i];
		p.pos += dt*p.velocity;

		float elasticVel = 0.1;
		if (p.pos.x < -100)
			p.velocity.x += elasticVel;
		if (p.pos.x > 100)
			p.velocity.x -= elasticVel;
		if (p.pos.y < -100)
			p.velocity.y += elasticVel;
		if (p.pos.y > 100)
			p.velocity.y -= elasticVel;
		if (p.pos.z < -100)
			p.velocity.z += elasticVel;
		if (p.pos.z > 100)
			p.velocity.z -= elasticVel;

		// Update GPU buffer with new positions.
		gl_pos_data[4 * i + 0] = p.pos.x;
		gl_pos_data[4 * i + 1] = p.pos.y;
		gl_pos_data[4 * i + 2] = p.pos.z;
		gl_pos_data[4 * i + 3] = p.radius;

	}
}

void Update(double deltaTime)
{
	//make targe once I set bounds
	float ratio_width = glm::quarter_pi<float>() / static_cast<float>(1920);
	float ratio_height = glm::quarter_pi<float>() / static_cast<float>(1080);

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	glfwSetCursorPos(window, 1920.0 / 2, 1080.0 / 2);
	// Calculate delta of cursor positions from last frame
	double delta_x = xpos - 1920.0 / 2;
	double delta_y = ypos - 1080.0 / 2;
	// Multiply deltas by ratios - gets actual change in orientation
	delta_x *= ratio_width;
	delta_y *= ratio_height;
	cam.Rotate(static_cast<float>(delta_x), static_cast<float>(-delta_y)); // flipped y to revert the invert.
	cam.Update(deltaTime);


	// Handle N-Body simulation segment.
	//SimulateParticles();
	UpdateParticles(deltaTime);
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
	if (Initialise() == -1)
		return -1;

	//While still running and esc hasnt been pressed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0)
	{

		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		Update(delta);//need to store postions in collection and render later
		Render();
		lastTime = currentTime;

	}

	delete[] gl_pos_data;

	//Cleanup VBO and shader
	glDeleteBuffers(1, &colour_buffer);
	glDeleteBuffers(1, &pos_buffer);
	glDeleteBuffers(1, &vertex_buffer);
	glDeleteProgram(shader.GetId());
	glDeleteVertexArrays(1, &VertexArrayID);


	//Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}