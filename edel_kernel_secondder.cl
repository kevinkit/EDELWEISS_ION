__kernel void second_filter(__global int* input, global int* output, global int* filtersettings)
{

	int id = get_global_id(0);
	int old;	
	int i = 0;
	int filter_length = filtersettings[0];
	int y = 0;
	int x = 0;
	int z = 0;
	int secondder_a;
	int secondder_b;
	int e = -1212;


	int id_filter_length = id + filter_length;
	int id_2_filter_length = id + (filter_length << 1);
	
	for(i = 0; i < filter_length; i ++)
	{
		x += input[id + i];
		y += input[id_filter_length + i];
		z += input[id_2_filter_length + i];
	}

	secondder_a  = x - (y << 1) + z;
	secondder_b = secondder_a - input[id] + 3*(input[id_filter_length + 1] - input[id_2_filter_length +1]) + input[id_2_filter_length + filter_length + 1];

	x = 0;
	y = 0;



		if(secondder_a >= 0 && secondder_b < 0)
		{
			for(i = 0; i < filter_length; i++)
			{
				x += input[id + i + (filter_length >> 1)];
				y += + input[id + i + filter_length + (filter_length >> 1)]; 
			}
			
			e = (y-x)/(filter_length);
			if(e >= filtersettings[1])
			{
				old = atomic_inc(&filtersettings[3]);
				output[(old << 1)] = e;
				output[(old << 1) +1 ] = id;
			}

		} 	


}	


