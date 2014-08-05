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

/*


int ocl_get_local_size(size_t global, size_t* results)
{

	size_t i = 1;

	size_t zw;
	int j = 0;
	for(; i < global; i++)
	{

		if(global%i == 0)
		{


			if(i != global && i != 1)
			{
				zw = i;
				results[j] = zw;
				j++;
				printf("%d \n", zw);
			}

			if(i == global)
			{

				results = NULL;
			}

		}

	}
	printf("results[%i] = %d", j-1, results[j-1]);
	return j;


}

*/

#endif

