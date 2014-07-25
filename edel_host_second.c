#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "ocl.h"
#include "OCL_gpu.h"
#include "settings.h"
#include "writeback.h"
#include "input.h"

int main(int argc, char* argv[])
{
	
	//will be put in the settings.h
	const size_t SIZE_execution_bit = (input_length - 3*filter_length +1);
	const size_t SIZE_input_bit = sizeof(gint32)*(input_length+1);
	const size_t SIZE_settings_bit = sizeof(gint32)*4;

	size_t output_bit_on_counts;
	size_t* SIZE_execution_pointer = &SIZE_execution_bit;

	gint32* filtersettings = (gint32*) malloc(SIZE_settings_bit);
	gint32* input_vector = (gint32*) malloc(SIZE_input_bit);
	gint32* positions = (gint32*) malloc(SIZE_input_bit);

	filtersettings[0] = filter_length;
	filtersettings[1] = threshhold;
	filtersettings[2] = input_length;
	filtersettings[3] = 0;
	
	
	//GPU-Init
	ocl = ocl_new(CL_DEVICE_TYPE_GPU,1);
	context = ocl_get_context(ocl);
	queue = ocl_get_cmd_queues (ocl)[0];
	
	program = ocl_create_program_from_file(ocl, "edel_kernel_secondder.cl", NULL, &errcode);
	OCL_CHECK_ERROR(errcode);
	
	filter1 = clCreateKernel(program, "second_filter", &errcode);
	OCL_CHECK_ERROR(errcode);
	
	//GPU-Buffer which can be done before the Computation
	settings = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, SIZE_settings_bit, filtersettings, &errcode);
	OCL_CHECK_ERROR(errcode);

	input = clCreateBuffer(context, CL_MEM_READ_ONLY, SIZE_input_bit, NULL, &errcode);
	OCL_CHECK_ERROR(errcode);
	
	
	if(debugmode != 0)
	{
		srand((unsigned) time( NULL ));
		counter = rand_rects(expected,1,input_length,3*filter_length,3*filter_length,3*filter_length,peak_length,base+peak, input_vector, noise, base, 0,positions);
		for(i = 0; i < input_length;i++)
		{
			if(input_length < 10000)
			{
				printf("input_vector[%i] = %d\n",i,input_vector[i]);
			}
			else
			{
				printf("input_vector[%i] = %d\t",i,input_vector[i]);
			}
		}
	
	
		printf("\n counts = %d\n", counter);
                printf("%lu Bits needed for Output-Vector \n", output_bit_on_counts);

	}
	
	output_bit_on_counts = sizeof(gint32) * safetyfactor * 2*((counter + 2));

	clEnqueueWriteBuffer(queue, input, CL_TRUE, 0, SIZE_input_bit, input_vector, 0, NULL, NULL);

	gint32* energy_time = (gint32*)malloc(output_bit_on_counts);
		
	
	for(i = 0; i < safetyfactor * (2*counter+2); i++)
	{
		energy_time[i] = -9999;
	}


	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_bit_on_counts, NULL , &errcode);
	OCL_CHECK_ERROR(errcode);

	
	OCL_CHECK_ERROR(clSetKernelArg(filter1, 0, sizeof(cl_mem), &input));
	OCL_CHECK_ERROR(clSetKernelArg(filter1, 1, sizeof(cl_mem), &output));
	OCL_CHECK_ERROR(clSetKernelArg(filter1, 2, sizeof(cl_mem), &settings));
	
	

	OCL_CHECK_ERROR(clEnqueueNDRangeKernel(queue, filter1, 1, NULL, SIZE_execution_pointer, NULL, 0, NULL, NULL));

	clEnqueueReadBuffer(queue, output, CL_TRUE, 0, output_bit_on_counts, energy_time, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, settings, CL_TRUE, 0, SIZE_settings_bit, filtersettings, 0, NULL, NULL);

	
	//Writing back the data
	for(i = 0; i < filtersettings[3]; i++)
	{
			writing_back(filemode, filename, filename_e,filename_t, energy_time,i);
	}

	if(debugmode != 0)
	{
		printf("The Positions are:\n");
		for(i=0; i < counter; i++)
		{
			printf("%d\t", positions[i]);
			printf("note that this postion is the middle of the rect \n");
		}
	}
	//Safetychanges
	if(filtersettings[3] > counter)
	{
		safetyfactor = safetyfactor + 5*(filtersettings[3] - counter);
		if(safetyfactor <= 0)
		{
			safetyfactor = 10;
		}
		
		notexpect = filtersettings[3] - expected;
		
		if(safemode != 0 && notexpect >= notexpect_max)
		{
			printf("The Filter found to many peaks it. It expected %d. It found %d times more than expected.\n", expected, notexpect);
			printf("Safemode is on. Exit program \n");
			OCL_CHECK_ERROR(clReleaseMemObject(input));
			OCL_CHECK_ERROR(clReleaseMemObject(output));
			OCL_CHECK_ERROR(clReleaseMemObject(settings));
			OCL_CHECK_ERROR(clReleaseKernel(filter1));
			OCL_CHECK_ERROR(clReleaseProgram(program));
	
			ocl_free(ocl);
	
			free(input_vector);
			free(energy_time);
			free(positions);
			free(filtersettings);
			
		}
		else
		{
			printf("The Filter found to many peaks it. It expected %d. It found %d times more than expected \n", expected, notexpect);
		}
	}
	


	OCL_CHECK_ERROR(clReleaseMemObject(input));
	OCL_CHECK_ERROR(clReleaseMemObject(output));
	OCL_CHECK_ERROR(clReleaseMemObject(settings));
	OCL_CHECK_ERROR(clReleaseKernel(filter1));
	OCL_CHECK_ERROR(clReleaseProgram(program));
	
	ocl_free(ocl);
	free(input_vector);
	free(energy_time);
    	free(positions);
    	free(filtersettings);





}



