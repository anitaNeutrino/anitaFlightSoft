/*
*************************************************************************************************************
* Author: Ben Strutt
* Email: b.strutt.12@ucl.ac.uk
*************************************************************************************************************
*/

#ifndef OPENCLWRAPPER_H
#define OPENCLWRAPPER_H

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#else
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

#include <stdio.h>
#include <string.h>

typedef struct
{
  cl_mem mem;         /* Actual opencl object - pretty small so is probably more like some kind of pointer*/
  size_t size;        /* Opencl functions require the size and the object so keep them together */
  const char* name;   /* Used for debugging - should be the same as the variable name */
  const char* typeChar;
} buffer;


void getPlatformAndDeviceInfo(cl_platform_id* platformIds, cl_uint maxPlatforms, cl_uint myPlatform, cl_device_type devType);

cl_program compileKernelsFromSource(const char* fileName, cl_context context, cl_device_id* deviceList, 
				    cl_uint numDevicesToUse, uint showCompileLog);

cl_kernel createKernel(cl_program prog, const char* kernelName);

buffer* createLocalBuffer(size_t size, const char* bufferName);

buffer* createBuffer(cl_context context, cl_mem_flags memFlags, size_t size, const char* typeChar, const char* bufferName);

void destroyBuffer(buffer* theBuffer);


cl_event writeBuffer(cl_command_queue cq, buffer* theBuffer, void* array, cl_uint wlSize, const cl_event* wl);

void copyArrayToGPU(cl_command_queue cq, buffer* theBuffer, void* array);

void copyArrayFromGPU(cl_command_queue cq, buffer* theBuffer, void* array);

void moveDataBetweenCPUandGPU(cl_command_queue cq, buffer* theBuffer, void* array, uint directionFlag);

void printBufferToTextFile(cl_command_queue cq, const char* fileName, int polInd, buffer* theBuffer, const int numEvents);

void setKernelArgs(cl_kernel kernel, int numArgs, buffer** buffers, const char* kernelName);
void setKernelArg(cl_kernel kernel, uint arg, buffer* theBuffer, const char* kernelName);

const char * translateReturnValue(cl_int err);

void statusCheck(cl_int ret, const char* functionName);

#endif
