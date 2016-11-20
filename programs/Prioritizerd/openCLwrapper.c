/*!
 @file openCLwrapper.c
 @date 26 Feb 2014
 @author Ben Strutt

 @brief
 Set of wrapper functions which (hopefully) simplify the unwieldy openCL functions and do error checking in a systematic way.

 @detailed
 The openCL functions are somewhat cumbersome so this is a set of wrapper functions, which makes the functional code shorter and gives human readable errors.
 I have adopted the convention of dropping the cl from the opencl function name when I am  wrapping it.
 */

#include "openCLwrapper.h"
#include <signal.h>
#include <unistd.h>

void getPlatformAndDeviceInfo(cl_platform_id* platformIds, cl_uint maxPlatforms, cl_uint myPlatform, cl_device_type devType){

  /* See how many platforms are available.*/
  cl_uint numPlatforms;
  cl_int status = clGetPlatformIDs(maxPlatforms, platformIds, &numPlatforms);
  statusCheck(status, "clGetPlatformIds");
  printf("%d platform(s) detected.\n", numPlatforms);


  //overwrite the $DISPLAY because sometimes it seems messed up
  setenv("DISPLAY",":0",1); 
  setenv("COMPUTE",":0",1); 

  /* Get the names of the platforms and display to user. */
  char platNames[maxPlatforms][100];
  uint plat = 0;
  for(plat = 0; plat < numPlatforms; plat++){
    size_t nameBufferLength;
    status = clGetPlatformInfo(platformIds[plat], CL_PLATFORM_NAME, sizeof(platNames[plat]), platNames[plat], &nameBufferLength);
    statusCheck(status, "clGetPlatformInfo");
    printf("\tPlatform %d: %s\n", plat, platNames[plat]);
  }
  
  /* see how many devices our chosen platform platform ...*/
  const cl_uint maxDevices = 4;
  cl_device_id deviceIds[maxDevices];
  cl_uint numDevices;
  clGetDeviceIDs(platformIds[myPlatform], devType, maxDevices, deviceIds, &numDevices);
  statusCheck(status, "clGetDeviceIDs");

  /* Give up if we can't find any devices on chosen platform */
  if (numDevices==0) {
    syslog(LOG_ERR, "Error! 0 devices found on platform %s!\n", platNames[myPlatform]);
    syslog(LOG_ERR, "This normally means I can't talk to the X server for some reason.\n");
    syslog(LOG_ERR, "Exiting program.\n");
    fprintf(stderr, "Error! 0 devices found on platform %s!\n", platNames[myPlatform]);
    fprintf(stderr, "This normally means I can't talk to the X server for some reason.\n");
    fprintf(stderr, "Exiting program.\n");
    sleep(10); 
    raise(SIGTERM); 
  }

  /* Prints useful information about the GPU architecture to the screen.*/
  size_t maxWorkGroupSize;
  char devNames[maxDevices][100];
  uint dev=0;
  for(dev = 0; dev < numDevices; dev++){
    size_t length;
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_NAME, sizeof(devNames[dev]), devNames, &length);
    statusCheck(status, "clGetDeviceInfo");
     
    cl_bool available;
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_AVAILABLE, sizeof(available), &available, &length);
    statusCheck(status, "clGetDeviceInfo");
     
    cl_uint maxComputeUnits;
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(maxComputeUnits), &maxComputeUnits, &length);
    statusCheck(status, "clGetDeviceInfo");
     
    cl_uint maxWorkItemDimension;
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(maxWorkItemDimension), &maxWorkItemDimension, &length);
    statusCheck(status, "clGetDeviceInfo");

    size_t maxWorkItemSizes[maxWorkItemDimension];
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(maxWorkItemSizes), &maxWorkItemSizes, &length);
    statusCheck(status, "clGetDeviceInfo");

    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkGroupSize), &maxWorkGroupSize, &length);
    statusCheck(status, "clGetDeviceInfo");

    cl_ulong maxMemAllocSize = 0;
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxMemAllocSize), &maxMemAllocSize, &length);
    statusCheck(status, "clGetDeviceInfo");
      
    cl_ulong deviceLocalMemSize = 0;
    status = clGetDeviceInfo(deviceIds[dev], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(deviceLocalMemSize), &deviceLocalMemSize, &length);
    statusCheck(status, "clGetDeviceInfo");

    printf("\t\tDevice: %d\n", dev);
    printf("\t\tName: %s\n", devNames[dev]);
    printf("\t\tAvailable: ");
    if(available){printf("Yes");}
    else{printf("No");}
    printf("\n\t\tNumber of Compute Units: %d\n", maxComputeUnits);
    printf("\t\tNumber of Work Item dimensions: %d\n", maxWorkItemDimension);
    printf("\t\tMax Work Items for each dimension: ");
    uint dim=0; for(dim = 0; dim < maxWorkItemDimension; dim++){ printf("%ld ", maxWorkItemSizes[dim]);}
    printf("\n\t\tMax Work Group size: %ld\n", maxWorkGroupSize);
    printf("\t\tGlobal memory size (bytes): %ld\n", maxMemAllocSize);
    printf("\t\tLocal memory size (bytes): %ld\n", deviceLocalMemSize);
  }
}



/*!
  @brief
  Reads in the kernel source code and compiles it

  @detailed
  Reads in the kernel source code and compiles it.
  The output from the compiler can be printed to the terminal with the showCompileLog variable.
  Returns a cl_program which is used to enqueue all the opencl kernels.

 */
cl_program compileKernelsFromSource(const char* fileName, const char* opt, cl_context context, cl_device_id* deviceList, cl_uint numDevicesToUse, uint showCompileLog){
  
  cl_int status;            /*For error checking*/
  FILE *fp;                 /*File containing kernel source code*/
  /* size_t source_size;       /\*Size*\/ */
  char *source_str;         /*c-style string containing source code*/
  cl_program prog;          /*The compiled kernel*/
  char buildOptions[200];   /*Contains options to OpenCL kernel compiler*/

  /* Only required if showCompileLog flag is > 0 */
  char *buffer;             /* Pointer to c-style string containing compiler output - for debugging */
  size_t len;               /* Size of c-style string */

#define MAX_SOURCE_SIZE (0x100000) 

  /* Load kernel source code */
  fp = fopen(fileName, "rb");
  if (!fp) {
    syslog(LOG_ERR, "There was an error opening %s, program will exit", fileName);
    fprintf(stderr, "There was an error opening %s, program will exit", fileName);
    raise(SIGTERM); 
  }
  source_str = (char *)malloc(MAX_SOURCE_SIZE);
  /* source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp); */
  fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  prog = clCreateProgramWithSource(context, 1, (const char**)&source_str, NULL, &status);
  statusCheck(status, "clCreateProgramWithSource");
  free(source_str);

  /* ... with build options to include files in the same directory */
  /*  sprintf(buildOptions,"-I../include -cl-denorms-are-zero -cl-finite-math-only"); */
  sprintf(buildOptions,"-I../include %s", opt);

  status = clBuildProgram(prog, numDevicesToUse, deviceList, buildOptions, NULL, NULL);

  /* Vital for tracking down errors in the kernel source code.*/
  if(showCompileLog > 0){
    clGetProgramBuildInfo(prog, deviceList[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    buffer = (char*)malloc(len);
    clGetProgramBuildInfo(prog, deviceList[0], CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    if( status != CL_SUCCESS){
      syslog(LOG_ERR, "%s\n", buffer);
      fprintf(stderr, "%s\n", buffer);
    }
    else{
      syslog(LOG_INFO, "%s\n", buffer);
      printf("%s\n", buffer);
    }
    free(buffer);
  }
  statusCheck(status, "clBuildProgram");

  return prog;
}



/*! 
  @brief
  Creates a cl_kernel and checks it for errors.

  @detailed
  Utilizes the optional error checking functionality provided by opencl, wrapped up nicely.
*/
cl_kernel createKernel(cl_program prog, const char* kernelName){

  cl_int status; // for error checking

  // Call the openCL function to create a kernel
  cl_kernel newKernel = clCreateKernel(prog, kernelName, &status);

  // description of function and check error
  char errorDescription[1024];
  sprintf(errorDescription, "createKernel %s", kernelName);
  statusCheck(status, errorDescription);
  return newKernel;
}




/*! 
  @brief
  Essentially a holder for a NULL pointer plus a size.
*/
buffer* createLocalBuffer(size_t size, const char* bufferName){
  buffer* theBuffer = malloc(sizeof(buffer));
  theBuffer->mem = NULL;
  theBuffer->size = size;
  theBuffer->name = bufferName;
  theBuffer->typeChar = "f";
  return theBuffer;
}

/*! 
  @brief
  Creates a cl_mem buffer, checks it for errors, wraps it up and returns it.

  @detailed
  Utilizes the optional error checking functionality provided by opencl, wrapped up nicely.
*/

buffer* createBuffer(cl_context context, cl_mem_flags memFlags, size_t size, const char* typeChar, const char* bufferName){
  cl_int status;
  /* 
     I am not using alloc host pointer method, since I prefer to copy data explicitly in code.
     (I think it makes it more obvious what is going on.)
  */
  cl_mem newBuffer = clCreateBuffer(context, memFlags, size, NULL, &status);
  char errorDescription[1024];
  sprintf(errorDescription, "createBuffer %s", bufferName);
  statusCheck(status, errorDescription);

  /* Put all this stuff into my wrapped buffer*/
  buffer* wrappedBuffer = malloc(sizeof(buffer));

  wrappedBuffer->mem = newBuffer;
  wrappedBuffer->size = size;
  wrappedBuffer->name = bufferName;
  wrappedBuffer->typeChar = typeChar;
  
  return wrappedBuffer;
}



/*!
  @Brief
  Release the cl_mem object and free the memory taken by the wrapper
 */
void destroyBuffer(buffer* theBuffer){

  clReleaseMemObject(theBuffer->mem);
  free(theBuffer);

}



/*! 
  @brief
  Sets all arguments for a kernel in a single function.

  @detailed
  The wrapped function requires pointers to the buffers and sizes.
*/
void setKernelArgs(cl_kernel kernel, int numArgs, buffer** buffers, const char* kernelName){
  uint arg=0;

  for(arg=0; arg<numArgs; arg++){
    setKernelArg(kernel, arg, buffers[arg], kernelName);
  /*   if(buffers[arg]->mem != NULL){ /\*  For global memory *\/ */
  /*     /\* Wants the size of the cl_mem object, i.e. the size on the CPU, not the GPU  *\/ */
  /*     /\* I guess the cl_mem object contains that information somewhere *\/ */
  /*     status = clSetKernelArg(kernel, arg, sizeof(buffers[arg]->mem), &buffers[arg]->mem); */
  /*   } */
  /*   else{ /\* For local memory *\/ */
  /*     /\* In the case of making a local buffer (I have set cl_mem to NULL),  *\/ */
  /*     /\* we need the size of the buffer we want to create on the GPU in bytes. *\/ */
  /*     status = clSetKernelArg(kernel, arg, buffers[arg]->size, NULL); */
  /*   } */
  /*   /\* Opencl error checking *\/ */
  /*   sprintf(errorDescription, "setKernelArg %d %s", arg, kernelName); */
  /*   statusCheck(status, errorDescription); */
  } 
}


void setKernelArg(cl_kernel kernel, uint arg, buffer* theBuffer, const char* kernelName){
  
  cl_int status;
  char errorDescription[1024];
  if(theBuffer->mem != NULL){ /*  For global memory */
    /* Wants the size of the cl_mem object, i.e. the size on the CPU, not the GPU  */
    /* I guess the cl_mem object contains that information somewhere */
    status = clSetKernelArg(kernel, arg, sizeof(theBuffer->mem), &theBuffer->mem);
  }
  else{ /* For local memory */
    /* In the case of making a local buffer (I have set cl_mem to NULL),  */
    /* we need the size of the buffer we want to create on the GPU in bytes. */
    status = clSetKernelArg(kernel, arg, theBuffer->size, NULL);
  }
  /* Opencl error checking */
  sprintf(errorDescription, "setKernelArg %d %s", arg, kernelName);
  statusCheck(status, errorDescription);

}



/*!
  @brief
  Non-blocking function to copy data to GPU.

  @Detailed
  Has to be non-blocking in order to allow asynchronous memory transfers

 */
cl_event writeBuffer(cl_command_queue cq, buffer* theBuffer, void* array, cl_uint wlSize, const cl_event* wl){
  //  cl_event* copyEvent = malloc(sizeof(cl_event));
  cl_event copyEvent; 
  cl_int status = clEnqueueWriteBuffer(cq, theBuffer->mem, CL_FALSE,
				       0, theBuffer->size, array,
				       //				       wlSize, wl, copyEvent);
				       wlSize, wl, &copyEvent);
  char errorDescription[1024];
  sprintf(errorDescription, "clEnqueueWriteBuffer (in writeBuffer) %s", theBuffer->name);
  statusCheck(status, errorDescription);
  //  return (*copyEvent);
  return copyEvent;
}



/*!
  @brief
  Non-blocking function to copy data from GPU.

  @Detailed
  Has to be non-blocking in order to allow asynchronous memory transfers

 */
cl_event readBuffer(cl_command_queue cq, buffer* theBuffer, void* array, cl_uint wlSize, const cl_event* wl){
  //  cl_event* copyEvent = malloc(sizeof(cl_event));
  cl_event copyEvent; 
  cl_int status = clEnqueueReadBuffer(cq, theBuffer->mem, CL_FALSE,
				      0, theBuffer->size, array,
				      //				      wlSize, wl, copyEvent);
				      wlSize, wl, &copyEvent);
  char errorDescription[1024];
  sprintf(errorDescription, "clEnqueueReadBuffer (in readBuffer) %s", theBuffer->name);
  statusCheck(status, errorDescription);
  //  return (*copyEvent);
  return copyEvent;
}



/*!
  @brief
  Copies the number of bytes given by theBuffer.size from array to the cl_mem object theBuffer wraps on the GPU.

  @Detailed
  This assumes that's the amount of data you want to copy of course...

 */
void copyArrayToGPU(cl_command_queue cq, buffer* theBuffer, void* array){
  moveDataBetweenCPUandGPU(cq, theBuffer, array, 0);
}

void copyArrayFromGPU(cl_command_queue cq, buffer* theBuffer, void* array){
  moveDataBetweenCPUandGPU(cq, theBuffer, array, 1);
}


/*!
  @brief
  General bi-directional copying data function.

  @detailed
  Functions with nicer names call this general function with the appropriate direction flag.
 */

void moveDataBetweenCPUandGPU(cl_command_queue cq, buffer* theBuffer, void* array, uint directionFlag){

  cl_int status;
  void* mappedArray = clEnqueueMapBuffer(cq, theBuffer->mem, CL_TRUE, CL_MAP_WRITE,
					 0, theBuffer->size, 0, NULL, NULL, &status);
  char errorDescription[1024];
  sprintf(errorDescription, "clEnqueueMapBuffer (in moveDataBetweenCPUandGPU) %s", theBuffer->name);
  statusCheck(status, errorDescription);
  
  if(directionFlag == 0){
    memcpy(mappedArray, array, theBuffer->size);
  }
  else{
    memcpy(array, mappedArray, theBuffer->size);
  }
  
  status = clEnqueueUnmapMemObject(cq, theBuffer->mem, mappedArray, 0, NULL, NULL);
  sprintf(errorDescription, "clEnqueueUnmapMemObject (in moveDataBetweenCPUandGPU) %s", theBuffer->name);
  statusCheck(status, errorDescription);

  clFinish(cq);
}



void printBufferToTextFile(cl_command_queue cq, const char* fileName, int polInd, buffer* theBuffer, const int numEvents){

  /* If numEventsToPrint not specified, then print them all... */
  printBufferToTextFile2(cq, fileName, polInd, theBuffer, numEvents, numEvents);

}

void printBufferToTextFile2(cl_command_queue cq, const char* fileName, int polInd, buffer* theBuffer, const int numEvents, const int numEventsToPrint){

  size_t dataSize = 0;
  int typeInd = -1;
  char fileNameWithFolder[1024];

  if(strcmp(theBuffer->typeChar, "f") == 0){
    dataSize = sizeof(float);
    typeInd = 0;
  }
  else if(strcmp(theBuffer->typeChar, "i") == 0){
    dataSize = sizeof(int);
    typeInd = 1;
  }
  else if(strcmp(theBuffer->typeChar, "s") == 0){
    dataSize = sizeof(short);
    typeInd = 2;
  }
  else{
    printf("Could not interpret buffer data type.\n");
    printf("Exiting program.\n");
    raise(SIGTERM); 
  }

  /* sprintf(fileNameWithFolder, "../debugOutput/%s_%d", fileName, polInd); */
  sprintf(fileNameWithFolder, "/tmp/%s_%d", fileName, polInd);

  size_t numBytes = theBuffer->size; 
  void* array = malloc(numBytes);
  copyArrayFromGPU(cq, theBuffer, array);

  FILE* outputFile = NULL;
  outputFile = fopen(fileNameWithFolder, "w");
  if(outputFile == NULL){
    printf("Failed to open output file in printBufferToTextFile.\n");
    printf("Exiting Program.\n");
    raise(SIGTERM); 
  }


  unsigned int element = 0;
  uint event = 0;
  /* uint nb = 0; */
  uint numElements = theBuffer->size/dataSize;
  uint elementsPerLine = numElements/numEvents;
  
  /* 
     This pointer typecasting arithmetic is everything wrong with c.
     It shouldn't be allowed, but it is...
  */
  for(element=0; element<numElements; element++){
    if(element > 0 && element%elementsPerLine == 0){
      fprintf(outputFile, "\n");
      event++;
    }

    /* Stop when we've done the desired number... */
    if(event >= numEventsToPrint ){
      break;
    }
    
    if(typeInd == 0){
      fprintf(outputFile, "%f ", *(((float*)array)+element));
    }
    else if(typeInd == 1){
      fprintf(outputFile, "%i ", *(((int*)array)+element));
    }
    else if(typeInd == 2){
      fprintf(outputFile, "%hi ", *(((short*)array)+element));
    }
  }
  fprintf(outputFile, "\n");
  fclose(outputFile);
  free(array);
}

/*! 
  @brief
  Reads cl_int status variable to see if a function returned successfully.

  @detailed
  Reads the cl_int variable returned by many opencl functions and exits with an error message if something is amiss.
*/
void statusCheck(cl_int status, const char* description){
  if (status != CL_SUCCESS){
    const char* errorName =  translateReturnValue(status);
    syslog(LOG_ERR, "%s returned status %s. Exiting host program.\n", description, errorName);
    fprintf(stderr, "%s returned status %s. Exiting host program.\n", description, errorName);
    raise(SIGTERM); 
  }
}






/*! 
  @brief Translates the horrible opencl error list into something human readable.

  @detailed
  A switch case look up table converting opencl status returns into something more human friendly.
  Also works better with the openCL documentation.
*/
const char * translateReturnValue(cl_int ret){
  switch(ret){
  case 0: return "CL_SUCCESS";
  case -1: return "CL_DEVICE_NOT_FOUND";
  case -2: return "CL_DEVICE_NOT_AVAILABLE";
  case -3: return "CL_COMPILER_NOT_AVAILABLE";
  case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
  case -5: return "CL_OUT_OF_RESOURCES";
  case -6: return "CL_OUT_OF_HOST_MEMORY";
  case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
  case -8: return "CL_MEM_COPY_OVERLAP";
  case -9: return "CL_IMAGE_FORMAT_MISMATCH";
  case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
  case -11: return "CL_BUILD_PROGRAM_FAILURE";
  case -12: return "CL_MAP_FAILURE";

  case -30: return "CL_INVALID_VALUE";
  case -31: return "CL_INVALID_DEVICE_TYPE";
  case -32: return "CL_INVALID_PLATFORM";
  case -33: return "CL_INVALID_DEVICE";
  case -34: return "CL_INVALID_CONTEXT";
  case -35: return "CL_INVALID_QUEUE_PROPERTIES";
  case -36: return "CL_INVALID_COMMAND_QUEUE";
  case -37: return "CL_INVALID_HOST_PTR";
  case -38: return "CL_INVALID_MEM_OBJECT";
  case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
  case -40: return "CL_INVALID_IMAGE_SIZE";
  case -41: return "CL_INVALID_SAMPLER";
  case -42: return "CL_INVALID_BINARY";
  case -43: return "CL_INVALID_BUILD_OPTIONS";
  case -44: return "CL_INVALID_PROGRAM";
  case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
  case -46: return "CL_INVALID_KERNEL_NAME";
  case -47: return "CL_INVALID_KERNEL_DEFINITION";
  case -48: return "CL_INVALID_KERNEL";
  case -49: return "CL_INVALID_ARG_INDEX";
  case -50: return "CL_INVALID_ARG_VALUE";
  case -51: return "CL_INVALID_ARG_SIZE";
  case -52: return "CL_INVALID_KERNEL_ARGS";
  case -53: return "CL_INVALID_WORK_DIMENSION";
  case -54: return "CL_INVALID_WORK_GROUP_SIZE";
  case -55: return "CL_INVALID_WORK_ITEM_SIZE";
  case -56: return "CL_INVALID_GLOBAL_OFFSET";
  case -57: return "CL_INVALID_EVENT_WAIT_LIST";
  case -58: return "CL_INVALID_EVENT";
  case -59: return "CL_INVALID_OPERATION";
  case -60: return "CL_INVALID_GL_OBJECT";
  case -61: return "CL_INVALID_BUFFER_SIZE";
  case -62: return "CL_INVALID_MIP_LEVEL";
  case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
  default: return "Unknown OpenCL error";
  }
}
