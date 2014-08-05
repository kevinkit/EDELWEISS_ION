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







	cl_event event;
	cl_ulong queued, submit,start, end;
	double total_time;
	int run = 0;
	char timefile[1024] = "time_fil2.txt";
	FILE *file;

	//Filterlength 1024 does not need anybody...
	for(; run <= 8; run++)
	{
		switch(run)
		{
			case 0: filter_length = 2; break;
			case 1: filter_length = 4; break;
			case 2: filter_length = 8; break;
			case 3: filter_length = 16; break;
			case 4: filter_length = 32; break;
			case 5: filter_length = 64; break;
			case 6: filter_length = 128; break;
			case 7: filter_length = 256; break;
			case 8: filter_length = 512; break;
		}

		sprintf(timefile,"time_fil%d_optimized_new_filter.txt", filter_length);


		size_t* localsizes;

		localsizes = malloc(input_length*sizeof(size_t));


		int amount;
		int g;


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
		clFinish(queue);

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
			if(harddebug != 0)
			{
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
			}
			output_bit_on_counts = sizeof(gint32) * safetyfactor * 2*((counter + 2));

			printf("\n counts = %d\n", counter);
			printf("%lu Bits needed for Output-Vector \n", output_bit_on_counts);

		}

		output_bit_on_counts = sizeof(gint32) * safetyfactor *input_length* 2*((counter + 2));

		clEnqueueWriteBuffer(queue, input, CL_TRUE, 0, SIZE_input_bit, input_vector, 0, NULL, NULL);

		gint32* energy_time = (gint32*)malloc(output_bit_on_counts);


		for(i = 0; i < safetyfactor * (2*counter+2); i++)
		{
			energy_time[i] = -9999;
		}



		output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_bit_on_counts, NULL , &errcode);
		OCL_CHECK_ERROR(errcode);


		cl_mem* arg_buffer[3];
		arg_buffer[0] = input;
		arg_buffer[1] = output;
		arg_buffer[2] = settings;


		ocl_set_kernel_arg(filter1, arg_buffer, 2);



		size_t global_item_size = (input_length - 3*filter_length +1);

		amount = ocl_get_local_size(ocl,context ,queue, arg_buffer, 2,"edel_kernel_secondder.cl", "second_filter", filtersettings, filter_length, global_item_size, localsizes);
		printf("amount = %d", amount);

		if(debugmode != 0)
		{
			if(amount > 0)
			{
				FILE *good;
				good = fopen("filterlengthgood.txt", "a+");
				if(good == NULL)
				{
					printf("Error in opening debug file \n");
					exit(1);
				}
				fprintf(good, "The filterlength %d is good for this filter.\n", filter_length);
				fclose(good);
			}
			else
			{
				FILE* attention;
				attention = fopen("filterlengthbad.txt", "a+");
				if(attention == NULL)
				{
					printf("error in opening debug file \n");
					exit(1);
				}
				fprintf(attention, "The filterlength %d is not good for this filter, choose another filterlength ! \n", filter_length);
				fclose(attention);
				printf("There is no way to fit it evenly divided to workgroups, just let OpenCL do it \n");
			}
			if(harddebug != 0)
			{
				getchar();
			}

		}




		if(amount != -10)
		{
			OCL_CHECK_ERROR(clEnqueueNDRangeKernel(queue, filter1, 1, NULL, &global_item_size, &localsizes[amount], 0, NULL, &event));	
		}
		else
		{
			OCL_CHECK_ERROR(clEnqueueNDRangeKernel(queue, filter1, 1, NULL, &global_item_size, NULL,0,NULL,&event));
		}



		//local_item_size = NULL;	
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
			//		safetyfactor = safetyfactor + 5*(filtersettings[3] - counter);
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
				free(localsizes);
			}
			else
			{
				printf("The Filter found to many peaks it. It expected %d. It found %d times more than expected \n", expected, notexpect);
			}
		}



	              file = fopen(timefile, "a+");
                if(file == NULL)
                {
                        printf("could not open file \n");
                        exit(1);
                }

                clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(queued), &queued, NULL);
                clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT,sizeof(submit), &submit, NULL);
                clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,sizeof(start), &start, NULL);
                clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,sizeof(end), &end, NULL);

                total_time = end - start;
                printf("total time = %f\n",total_time);
                printf("Queued %llu\t Submit: %llu\t Start: %llu\t End: %llu \t \n",queued,submit,start,end);



                printf("Runtime %0.3f\n",(total_time/1000000.0));
                fprintf(file, "%0.3f\t", (total_time/1000000.0));

















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
		free(localsizes);
	}


}



