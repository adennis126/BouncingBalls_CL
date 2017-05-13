/*
IMPORTANT: Rewrite all the ball collision implementation
introduce the mass element in ball collision, and this may make easier for collision since calculation is based on mass as well
*/
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <windows.h>
#include "Dependencies\glew\glew.h"
#include "Dependencies\freeglut\freeglut.h"
#include <Math.h>

using namespace std;
const float PI = 3.1415926;
const int BALL_COUNT = 10;
const float windowWidth = 640;
const float windowHeight = 480;
const float windowPosX = 50;
const float windowPosY = 50;
const float d = 1;
const float ballRadius = 15;


float ballX[BALL_COUNT];
float ballY[BALL_COUNT];
float angle[BALL_COUNT];
float ballC[BALL_COUNT];


cl_uint numPlatforms = 0;           //the NO. of platforms
cl_platform_id platform = nullptr;  //the chosen platform
cl_context context = nullptr;       // OpenCL context
cl_command_queue commandQueue = nullptr;
cl_program program = nullptr;       // OpenCL kernel program object that'll be running on the compute device
cl_mem d_ballx = nullptr;      // input1 memory object for input argument 1
cl_mem d_bally = nullptr;      // input2 memory object for input argument 2
cl_mem d_ball_angle = nullptr;      // output memory object for output
cl_kernel kernel = nullptr;         // kernel object


cl_int    status;

float convertX(float x){
	return (x - (windowWidth / 2)) / (windowWidth / 2);
}

float convertY(float y){
	return (y - (windowHeight / 2)) / (windowHeight / 2);
}

void edge_update()
{
	for (int i = 0; i < BALL_COUNT; ++i)
	{
		if (ballX[i] - ballRadius < 0)
		{
			angle[i] = -angle[i];
		    ballX[i] = ballRadius;
		}
		if (ballX[i] + ballRadius > windowWidth)
		{
			angle[i] = -angle[i];
			ballX[i] = windowWidth - ballRadius;
		}
		if (ballY[i] - ballRadius < 0)
		{
			angle[i] = PI - angle[i];
			ballY[i] = ballRadius;
		}
		if (ballY[i] + ballRadius > windowHeight)
		{
			angle[i] = PI - angle[i];
			ballY[i] = windowHeight - ballRadius;
		}
	}
}

void bounce_update()
{
	bool flag[BALL_COUNT];
	for (int i = 0; i < BALL_COUNT; ++i)  flag[i] = false;
	for (int i = 0; i < BALL_COUNT; ++i)
	{
		if (flag[i]) continue;
		for (int j = i + 1 ;  j < BALL_COUNT ; ++j)
		if (!flag[i] &&                      
			(ballX[i] -ballX[j])*(ballX[i] - ballX[j]) + (ballY[i] - ballY[j])*(ballY[i] - ballY[j])
		< (ballRadius+ballRadius) * (ballRadius  +ballRadius))
			{
				angle[i] +=  (rand()%45 / 180 *PI+0.8*PI);
				angle[j] += (rand()  % 45 / 180 * PI+0.8*PI);
				flag[i] = true;
				flag[j] = true;
			}
	}
}

void init_ball_statue()
{
	
	for (int i = 0; i < BALL_COUNT; ++i)
	{
		ballX[i] = (i+1)*windowWidth / BALL_COUNT - 30;
		ballY[i] = rand() % BALL_COUNT *((windowHeight -ballRadius )/ BALL_COUNT) +2*ballRadius;
		angle[i] = static_cast<float>(rand())/PI;
		ballC[i] = rand() % 3 + 1;
	}
}

//After computation in the GPU, the info has to be passed to CPU so that CPU can do the render on the screen
void display(){
	glClear(GL_COLOR_BUFFER_BIT);
	for (int i = 0; i < BALL_COUNT; ++i){
		if (ballC[i] == 1){
			glColor3f(1.0f, 0.0f, 0.0f);
		}
		else if (ballC[i] == 2){
			glColor3f(0.0f, 1.0f, 0.0f);
		}
		else if (ballC[i] == 3){
			glColor3f(0.0f, 0.0f, 1.0f);
		}

		glBegin(GL_TRIANGLE_FAN);
		glVertex3f(convertX(ballX[i]), convertY(ballY[i]), 0.0f); 

		//*******************Draw ball***********************
		for (int j = 0; j <= 100; j++){
			float angle = (j * 2.0f * PI) / 100;
			float x = sin(angle) * ballRadius + ballX[i];
			float y = cos(angle) * ballRadius + ballY[i];
			glVertex3f(convertX(x), convertY(y), 0.0f);
		}
		glEnd();
	}
	glutSwapBuffers();
}

void initGL(){
	init_ball_statue();	
}
void  openCl()
{
	d_ballx = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, BALL_COUNT * sizeof(float), ballX, nullptr);
	d_bally = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,BALL_COUNT * sizeof(float), ballY, nullptr);
	d_ball_angle = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, BALL_COUNT * sizeof(float), angle, nullptr);

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&d_ballx);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&d_bally);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&d_ball_angle);


	size_t global_work_size[1] = { BALL_COUNT};
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);
	clFinish(commandQueue);     // Force wait until the OpenCL kernel is completed
	status = clEnqueueReadBuffer(commandQueue, d_ballx, CL_TRUE, 0, global_work_size[0] * sizeof(float), ballX, 0, NULL, NULL);
	status = clEnqueueReadBuffer(commandQueue, d_bally, CL_TRUE, 0, global_work_size[0] * sizeof(float), ballY, 0, NULL, NULL);

}
void  timerProc(int  id)
{
	// execute opencl 
	openCl();
	edge_update();
	bounce_update();
	display();
	glutTimerFunc(10, timerProc, 1);
}

int main(int argc, char **argv){


	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		cout << "Error: Getting platforms!" << endl;
		return 0;
	}

	/*For clarity, choose the first available platform. */
	if (numPlatforms > 0)
	{
		cl_platform_id* platforms = (cl_platform_id*)malloc(numPlatforms* sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}
	else
	{
		puts("Your system does not have any OpenCL platform!");
		return 0;
	}

	/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint                numDevices = 0;
	cl_device_id        *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
	if (numDevices == 0) //no GPU available.
	{
		cout << "No GPU device available." << endl;
		cout << "Choose CPU as default device." << endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));

		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
		cout << "The number of devices: " << numDevices << endl;
	}

	/*Step 3: Create context.*/
	context = clCreateContext(NULL, 1, devices, NULL, NULL, NULL);

	/*Step 4: Creating command queue associate with the context.*/
	commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

	/*Step 5: Create program object */
	// Read the kernel code to the buffer
	FILE *fp = fopen("kernel.cl", "rb");
	if (fp == nullptr)
	{
		puts("The kernel file not found!");
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	size_t kernelLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *kernelCodeBuffer = (char*)malloc(kernelLength + 1);
	fread(kernelCodeBuffer, 1, kernelLength, fp);
	kernelCodeBuffer[kernelLength] = '\0';
	fclose(fp);

	const char *aSource = kernelCodeBuffer;
	program = clCreateProgramWithSource(context, 1, &aSource, &kernelLength, NULL);

	/*Step 6: Build program. */
	status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);

	kernel = clCreateKernel(program, "MyCLAdd", NULL);


	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(windowPosX, windowPosY);
	glutCreateWindow("Bouncing Ball");
	initGL();
	glutDisplayFunc(display);
	glutTimerFunc(1000, timerProc, 1);
	glutMainLoop();

	status = clReleaseKernel(kernel);//*Release kernel.
	status = clReleaseProgram(program);    //Release the program object.
	status = clReleaseMemObject(d_ballx);//Release mem object.
	status = clReleaseMemObject(d_bally);
	status = clReleaseMemObject(d_ball_angle);
	status = clReleaseCommandQueue(commandQueue);//Release  Command queue.
	status = clReleaseContext(context);//Release context.

	free(devices);
	return 0;
}