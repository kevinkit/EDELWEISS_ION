#ifndef OCL_GPU_H
#define OCL_GPU_H

#include "ocl.h"



OclPlatform *ocl;
cl_context context;
cl_int errcode;
cl_mem input;
cl_mem output;
cl_mem settings;
cl_program program;
cl_kernel filter1;
cl_command_queue queue;






#endif

