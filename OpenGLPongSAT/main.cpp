
#include <iostream>
#include <vector>

// We are using the glew32s.lib
// Thus we have a define statement
// If we do not want to use the static library, we can include glew32.lib in the project properties
// If we do use the non static glew32.lib, we need to include the glew32.dll in the solution folder
// The glew32.dll can be found here $(SolutionDir)\..\External Libraries\GLEW\bin\Release\Win32
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Transform.h"
#include "ShaderProgram.h"



// Variables for the Height and width of the window
const GLint WIDTH = 800, HEIGHT = 800;
const GLfloat ballVelocity = 1.0f;
const GLfloat playerVelocity = 1.5f;
glm::vec3 ballDirection = glm::normalize(glm::vec3(1, -1, 0));


GLuint VAO;
GLuint VBO;

//Move a paddle based on input
int MovePlayer(GLFWwindow* window, int pNum)
{
	if (pNum == 1)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			return 1;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			return -1;
		}

		return 0;

	}

	if (pNum == 2)
	{
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			return 1;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			return -1;
		}

		return 0;
	}

}

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;

	AABB()
	{
		min = glm::vec3();
		max = glm::vec3();
	}

};

//Calculate min and max of a set of points
void CalcAABB(GLfloat vertices[], glm::mat4 worldMatrix, AABB* AABB)
{
	glm::vec4 c_min = worldMatrix * glm::vec4(vertices[0], vertices[1], vertices[2], 1.0);
	glm::vec4 c_max = c_min;
	glm::vec4 vertPos = glm::vec4();

	for (int i = 0; i < 18; i += 3)
	{
		vertPos = worldMatrix * glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0);

		if (vertPos.x > c_max.x)
			c_max.x = vertPos.x;
		if (vertPos.y > c_max.y)
			c_max.y = vertPos.y;

		if (vertPos.x < c_min.x)
			c_min.x = vertPos.x;
		if (vertPos.y < c_min.y)
			c_min.y = vertPos.y;

	}
	AABB->min= c_min;
	AABB->max = c_max;
}



// Returns the 8 points for a given AABB. Largely just a helper function for this specific implementation.
std::vector<glm::vec3> GetPoints(AABB obj)
{
	std::vector<glm::vec3> points;

	// Left face
	// minx, miny, minz
	// minx, maxy, minz
	// minx, maxy, maxz
	// minx, miny, maxz
	points.push_back(glm::vec3(obj.min.x, obj.min.y, obj.min.z));
	points.push_back(glm::vec3(obj.min.x, obj.max.y, obj.min.z));
	points.push_back(glm::vec3(obj.min.x, obj.max.y, obj.max.z));
	points.push_back(glm::vec3(obj.min.x, obj.min.y, obj.max.z));

	// Right face
	// maxx, maxy, maxz
	// maxx, maxy, minz
	// maxx, miny, minz
	// maxx, miny, maxz
	points.push_back(glm::vec3(obj.max.x, obj.max.y, obj.max.z));
	points.push_back(glm::vec3(obj.max.x, obj.max.y, obj.min.z));
	points.push_back(glm::vec3(obj.max.x, obj.min.y, obj.min.z));
	points.push_back(glm::vec3(obj.max.x, obj.min.y, obj.max.z));

	return points;
}

// Returns the normals for an AABB. Also contains commented implementation for returning the normals of an arbitrary set of points.
std::vector<glm::vec3> GetNormals(AABB obj)
{
	// Since AABB is a cube, there should be 6 normals
	// But it's aligned on the axes, so there are only 3 we need to worry about, X, Y, and Z.
	std::vector<glm::vec3> normals;

	normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));

	return normals;
}


// Gets the minimum and maximum projections for a set of points given an axis to project them onto.
// Will output the values into the min and max variables passed into the function.
void GetMinMax(std::vector<glm::vec3> points, glm::vec3 axis, float& min, float&max)
{
	// To get a projection along a vector, you take the dot product of the vertex (in vector form) with the axis vector.
	// Since we're looking for both a minimum and a maximum, we start with the first point, and go from there.
	min = glm::dot(points[0], axis);
	max = min;

	// Looping through the rest of the points.
	for (int i = 1; i < points.size(); i++)
	{
		// Find the projection for the current point.
		float currProj = glm::dot(points[i], axis);

		// If this projection is smaller than our minimum projection, the minimum becomes this.
		if (min > currProj)
		{
			min = currProj;
		}

		// If this projection is larger than our maximum projection, the maximum becomes this.
		if (currProj > max)
		{
			max = currProj;
		}
	}
}

//SAT Code taken from https://github.com/IGME-RIT/physics-SAT2DAABB-VisualStudio
bool SATCollision(AABB box1, AABB box2)
{
	std::vector<glm::vec3> normals1 = GetNormals(box1);
	std::vector<glm::vec3> normals2 = GetNormals(box2);

	std::vector<glm::vec3> points1 = GetPoints(box1);
	std::vector<glm::vec3> points2 = GetPoints(box2);

	bool isSeparated = false;

	// For each normal
	for (int i = 0; i < normals1.size(); i++)
	{
		// Get the Min and Max projections for each object along the normal.
		float aMin, aMax;
		GetMinMax(points1, normals1[i], aMin, aMax);

		float bMin, bMax;
		GetMinMax(points2, normals1[i], bMin, bMax);

		// If the maximum projection of one of the objects is less than the minimum projection of the other object, then we can determine that there is a separation 
		// along this axis. Thus, we set isSeparated to true and break out of the for loop.
		isSeparated = aMax < bMin || bMax < aMin;
		if (isSeparated) break;
	}

	// This only runs if we still haven't proven that there is a separation between the two objects.
	// SAT is an optimistic algorithm in that it will stop the moment it determines there isn't a collision, and as such the less collisions there are the faster it will run.
	if (!isSeparated)
	{
		// Loop through the normals for the second object.
		// Note that since this is an AABB, the normals will be the same as the previous object. Again, this is left for future implementation and understanding.
		// The process below is exactly the same as above, only with object b's normals instead of object a.
		for (int i = 0; i < normals2.size(); i++)
		{
			float aMin, aMax;
			GetMinMax(points1, normals2[i], aMin, aMax);

			float bMin, bMax;
			GetMinMax(points2, normals2[i], bMin, bMax);

			isSeparated = aMax < bMin || bMax < aMin;
			if (isSeparated) break;
		}
	}

	// At this point, isSeparated has been tested against each normal. If it has been set to true, then there is a separation. If it is false, that means none of the axes 
	// were separated, and there is a collision.
	return isSeparated;
}

void ResolveCollision(Transform* player, Transform* ball)
{
	//Collision with player 1
	if (ball->position.x < 0)
	{
		//Check if the ball is behind or in front of the paddle
		if(ball->position.x > player->position.x)
			ballDirection = glm::normalize(glm::reflect((player->position - ball->position), glm::vec3(1, 0, 0)));

		return;
	}

	//Collision with player 2

	//Check if the ball is behind or in front of the paddle
	if (ball->position.x < player->position.x)
		ballDirection = glm::normalize(glm::reflect((player->position - ball->position), glm::vec3(-1, 0, 0)));
}


int main()
{
#pragma region GL setup
	//Initializes the glfw
	glfwInit();

	// Setting the required options for GLFW

	// Setting the OpenGL version, in this case 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 99);
	// Setting the Profile for the OpenGL.

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// Setting the forward compatibility of the application to true
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// We don't want the window to resize as of now.
	// Therefore we will set the resizeable window hint to false.
	// To make is resizeable change the value to GL_TRUE.
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create the window object
	// The first and second parameters passed in are WIDTH and HEIGHT of the window we want to create
	// The third parameter is the title of the window we want to create
	// NOTE: Fourth paramter is called monitor of type GLFWmonitor, used for the fullscreen mode.
	//		 Fifth paramter is called share of type GLFWwindow, here we can use the context of another window to create this window
	// Since we won't be using any of these two features for the current tutorial we will pass nullptr in those fields
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Shaders Tutorial", nullptr, nullptr);

	// We call the function glfwGetFramebufferSize to query the actual size of the window and store it in the variables.
	// This is useful for the high density screens and getting the window size when the window has resized.
	// Therefore we will be using these variables when creating the viewport for the window
	int screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

	// Check if the window creation was successful by checking if the window object is a null pointer or not
	if (window == nullptr)
	{
		// If the window returns a null pointer, meaning the window creation was not successful
		// we print out the messsage and terminate the glfw using glfwTerminate()
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();

		// Since the application was not able to create a window we exit the program returning EXIT_FAILURE
		return EXIT_FAILURE;
	}

	// Creating a window does not make it the current context in the windows.
	// As a results if the window is not made the current context we wouldn't be able the perform the operations we want on it
	// So we make the created window to current context using the function glfwMakeContextCurrent() and passing in the Window object
	glfwMakeContextCurrent(window);

	// Enable GLEW, setting glewExperimental to true.
	// This allows GLEW take the modern approach to retrive function pointers and extensions
	glewExperimental = GL_TRUE;

	// Initialize GLEW to setup OpenGL function pointers
	if (GLEW_OK != glewInit())
	{
		// If the initalization is not successful, print out the message and exit the program with return value EXIT_FAILURE
		std::cout << "Failed to initialize GLEW" << std::endl;

		return EXIT_FAILURE;
	}

	// Setting up the viewport
	// First the parameters are used to set the top left coordinates
	// The next two parameters specify the height and the width of the viewport.
	// We use the variables screenWidth and screenHeight, in which we stored the value of width and height of the window,
	glViewport(0, 0, screenWidth, screenHeight);
#pragma endregion

#pragma region Game_Setup
	float ballWidth = 0.02f;
	float ballHeight = 0.02f;
	Transform* player1 = new Transform();
	Transform* player2 = new Transform();

	player1->scale = player2->scale = glm::vec3(ballWidth, ballHeight*4, 1.0);
	player1->position = glm::vec3(-0.8f, 0.0, 0.0);
	player2->position = glm::vec3(0.8f, 0.0, 0.0);
	player1->Update();
	player2->Update();

	Transform* ball = new Transform();
	ball->position = glm::vec3(0.0, 0.8, 0.0);
	ball->scale = glm::vec3(ballWidth, ballHeight, 1.0);
	ball->Update();

	glm::vec2 score = glm::vec2(0, 0);


	AABB p1Box = AABB();
	AABB p2Box = AABB();
	AABB ballBox = AABB();
#pragma endregion
	GLfloat vertices[] = {
		// Triangle 1		
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		//Triangle 2
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f
	};



	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3* sizeof(GLfloat), (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	Shader*	vShader = new Shader((char*)"shaders/vShader.glsl", GL_VERTEX_SHADER);
	Shader*	fShader = new Shader((char*)"shaders/fShader.glsl", GL_FRAGMENT_SHADER);
	ShaderProgram* shader = new ShaderProgram();
	shader->AttachShader(vShader);
	shader->AttachShader(fShader);
	shader->Bind();

	GLint uniform = glGetUniformLocation(shader->GetGLShaderProgram(), (char*)"worldMatrix");
	// This is the game loop, the game logic and render part goes in here.
	// It checks if the created window is still open, and keeps performing the specified operations until the window is closed
	while (!glfwWindowShouldClose(window))
	{

		// Calculate delta time.
		float dt = glfwGetTime();
		// Reset the timer.
		glfwSetTime(0);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, screenWidth, screenHeight);

		
#pragma region Player_Updates
		player1->position.y += MovePlayer(window, 1) * playerVelocity * dt;
		player1->position.y = glm::clamp(player1->position.y, -0.9f, 0.9f);
		player1->Update();
		

		player2->position.y += MovePlayer(window, 2) * playerVelocity * dt;
		player2->position.y = glm::clamp(player2->position.y, -0.9f, 0.9f);
		player2->Update();
		
#pragma endregion

#pragma region Ball_Updates

		ball->position += ballDirection * ballVelocity * dt;
		if (ball->position.y >= 1 || ball->position.y <= -1)
			ballDirection.y *= -1;

		ball->Update();

		CalcAABB(vertices, player1->GetWorldMatrix(),&p1Box);
		CalcAABB(vertices, player2->GetWorldMatrix(), &p2Box);
		CalcAABB(vertices, ball->GetWorldMatrix(), &ballBox);

		//Paddle-ball collision
		if (!SATCollision(p1Box, ballBox))
		{
			ResolveCollision(player1, ball);
			
		}
		else if (!SATCollision(p2Box, ballBox))
		{
			ResolveCollision(player2, ball);
		}

		//Respawn ball
		if (ball->position.x >= 1)
		{
			score.x++;
			ball->position = glm::vec3(0.0, -0.8, 0.0);
			ballDirection = glm::normalize(glm::vec3(-1, 1, 0));

			printf("Player 1 Scored! Current score is %u to %u\n", (int)score.x, (int)score.y );
		}
		else if (ball->position.x <= -1)
		{
			score.y++;
			ball->position = glm::vec3(0.0, 0.8, 0.0);
			ballDirection = glm::normalize(glm::vec3(1, -1, 0));
			printf("Player 2 Scored! Current score is %u to %u\n", (int)score.x, (int)score.y);
		}


		ball->Update();

	//Check win condition
		if (score.x == 5)
		{
			printf("Player 1 Wins! Score is reset.\n");
			score.x = score.y = 0;
		}
		if (score.y == 5)
		{
			printf("Player 2 Wins! Score is reset.\n");
			score.x = score.y = 0;
		}
#pragma endregion

		glBindVertexArray(VAO);	

		glUniformMatrix4fv(uniform, 1, GL_FALSE, &ball->GetWorldMatrix()[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUniformMatrix4fv(uniform, 1, GL_FALSE, &player1->GetWorldMatrix()[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUniformMatrix4fv(uniform, 1, GL_FALSE, &player2->GetWorldMatrix()[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);


		// Swaps the front and back buffers of the specified window
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete player1, player2, ball, shader;
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	// Terminate all the stuff related to GLFW and exit the program using the return value EXIT_SUCCESS
	glfwTerminate();

	return EXIT_SUCCESS;
}


