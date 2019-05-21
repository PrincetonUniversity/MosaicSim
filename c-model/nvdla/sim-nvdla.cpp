/*
 * sim-nvdla.cpp
 *
 *  Created on: May 21, 2019
 *      Author: geichler
 */

#include "nvdla.h"
#include <iostream>
using namespace std;

int main(void)
{
	//Create an instance of the accelerator
	//You need to set the number of layers you want (in the example it is 1)
	nvdla_acc accelerator(1);

	//Create a neural network layer
	nvdla_layer* first_layer = new nvdla_layer();

	//Set all the parameters of a layer
	int num_of_inputs = 3;
	first_layer->set_num_of_inputs(num_of_inputs);

	int input_height = 224;
	first_layer->set_input_height(input_height);

	int input_width = 224;
	first_layer->set_input_width(input_width);

	int num_of_outputs = 96;
	first_layer->set_num_of_outputs(num_of_outputs);

	int filter_height = 11;
	first_layer->set_filter_height(filter_height);

	int filter_width = 11;
	first_layer->set_filter_width(filter_width);

	bool zero_pad = 0;
	first_layer->set_zero_padding(zero_pad);

	int vertical_conv_dim = 4;
	first_layer->set_vertical_conv_dim(vertical_conv_dim);

	int horizintal_conv_dim = 4;
	first_layer->set_horizontal_conv_dim(horizintal_conv_dim);

	bool pooling = true;
	first_layer->set_pooling_after_conv(pooling);

	int pool_height = 3;
	first_layer->set_pooling_height(pool_height);

	int pool_width = 3;
	first_layer->set_pooling_width(pool_width);

	int vertical_pool_dim = 2;
	first_layer->set_vertical_pooling_dim(vertical_pool_dim);

	int horizontal_pool_dim = 2;
	first_layer->set_horizontal_pooling_dim(horizontal_pool_dim);

	int winograd = 0;
	first_layer->set_winograd(winograd);

	layer_type type = conv;
	first_layer->set_layer_type(type); // Only conv or fc

	int batch_size = 16;
	first_layer->set_fc_batch_size(batch_size);

	int dram_bw_limit = 10; //GB/s
	first_layer->set_dram_bw_limit(dram_bw_limit);

	int frequency = 1000; //MHz
	first_layer->set_frequency(frequency);

	int num_of_mul = 2048;
	first_layer->set_num_of_mul(num_of_mul);


	//Insert the layer into the
	accelerator.add_layer(first_layer);

	//Calculate all performance attributes of the current accelerator
	accelerator.commit();

	//Print results
	cout << "Accelerator Data:" << endl;
	cout << "Number of layers: " << accelerator.get_num_of_layers() << endl;
	cout << "Max cycles: " << accelerator.get_total_max_cycles() << endl;
	cout << "MAC cycles: " << accelerator.get_total_mac_cycles() << endl;
	cout << "Total time: " << accelerator.get_total_time() <<"[ms]"<< endl;
	cout << "FPS: " << accelerator.get_fps() << endl;
	cout << "HW MAC efficiency: " << accelerator.get_hw_mac_efficiency() << endl;
	cout << "Network MAC efficiency: " << accelerator.get_network_mac_efficiency() << endl;
	cout << "Total calculations: " << accelerator.get_total_calculations() << endl;
	cout << endl;


	//Now let's create the whole AlexNet
	accelerator.clear();
	accelerator.set_num_of_layers(11);
	accelerator.add_layer(first_layer);
	nvdla_layer* layer_array[10];

	nvdla_layer* second_layer = new nvdla_layer(*first_layer);
	second_layer->set_num_of_inputs(48);
	second_layer->set_input_height(27);
	second_layer->set_input_width(27);
	second_layer->set_num_of_outputs(128);
	second_layer->set_filter_height(5);
	second_layer->set_filter_width(5);
	second_layer->set_zero_padding(true);
	second_layer->set_vertical_conv_dim(1);
	second_layer->set_horizontal_conv_dim(1);
	second_layer->set_pooling_after_conv(true);
	accelerator.add_layer(second_layer);
	layer_array[0] = second_layer;

	nvdla_layer* third_layer = new nvdla_layer(*second_layer);
	accelerator.add_layer(third_layer);
	layer_array[1] = third_layer;

	nvdla_layer* fourth_layer = new nvdla_layer(*third_layer);
	fourth_layer->set_num_of_inputs(256);
	fourth_layer->set_input_height(13);
	fourth_layer->set_input_width(13);
	fourth_layer->set_num_of_outputs(384);
	fourth_layer->set_filter_height(3);
	fourth_layer->set_filter_width(3);
	fourth_layer->set_zero_padding(true);
	fourth_layer->set_vertical_conv_dim(1);
	fourth_layer->set_horizontal_conv_dim(1);
	fourth_layer->set_pooling_after_conv(false);
	fourth_layer->set_winograd(1);
	accelerator.add_layer(fourth_layer);
	layer_array[2] = fourth_layer;

	nvdla_layer* fifth_layer = new nvdla_layer(*fourth_layer);
	fifth_layer->set_num_of_inputs(192);
	fifth_layer->set_num_of_outputs(192);
	accelerator.add_layer(fifth_layer);
	layer_array[3] = fifth_layer;

	nvdla_layer* sixth_layer = new nvdla_layer(*fifth_layer);
	accelerator.add_layer(sixth_layer);
	layer_array[4] = sixth_layer;

	nvdla_layer* seventh_layer = new nvdla_layer(*sixth_layer);
	seventh_layer->set_num_of_outputs(128);
	seventh_layer->set_pooling_after_conv(true);
	accelerator.add_layer(seventh_layer);
	layer_array[5] = seventh_layer;

	nvdla_layer* eighth_layer = new nvdla_layer(*seventh_layer);
	accelerator.add_layer(eighth_layer);
	layer_array[6] = eighth_layer;

	nvdla_layer* nineth_layer = new nvdla_layer(*eighth_layer);
	nineth_layer->set_num_of_inputs(256);
	nineth_layer->set_input_height(6);
	nineth_layer->set_input_width(6);
	nineth_layer->set_num_of_outputs(4096);
	nineth_layer->set_filter_height(6);
	nineth_layer->set_filter_width(6);
	nineth_layer->set_zero_padding(false);
	nineth_layer->set_pooling_after_conv(false);
	nineth_layer->set_layer_type(fc);
	nineth_layer->set_winograd(0);
	accelerator.add_layer(nineth_layer);
	layer_array[7] = nineth_layer;

	nvdla_layer* tenth_layer = new nvdla_layer(*nineth_layer);
	tenth_layer->set_num_of_inputs(4096);
	tenth_layer->set_input_height(1);
	tenth_layer->set_input_width(1);
	tenth_layer->set_filter_height(1);
	tenth_layer->set_filter_width(1);
	accelerator.add_layer(tenth_layer);
	layer_array[8] = tenth_layer;

	nvdla_layer* eleventh_layer = new nvdla_layer(*tenth_layer);
	eleventh_layer->set_num_of_outputs(1000);
	accelerator.add_layer(eleventh_layer);
	layer_array[9] = eleventh_layer;

	accelerator.commit();

	for(int i=0; i < 10; i++)
	{
		cout << "layer " << i+2 << " max cycles: " <<layer_array[i]->get_max_cycle()<< endl;
	}

	//Print results
	cout << endl;
	cout << "Accelerator Data:" << endl;
	cout << "Number of layers: " << accelerator.get_num_of_layers() << endl;
	cout << "Max cycles: " << accelerator.get_total_max_cycles() << endl;
	cout << "MAC cycles: " << accelerator.get_total_mac_cycles() << endl;
	cout << "Total time: " << accelerator.get_total_time() <<"[ms]"<< endl;
	cout << "FPS: " << accelerator.get_fps() << endl;
	cout << "HW MAC efficiency: " << accelerator.get_hw_mac_efficiency() << endl;
	cout << "Network MAC efficiency: " << accelerator.get_network_mac_efficiency() << endl;
	cout << "Total calculations: " << accelerator.get_total_calculations() << endl;

}


