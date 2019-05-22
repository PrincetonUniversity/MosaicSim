/*
 * nvdla.h
 *
 *  Created on: May 17, 2019
 *      Author: geichler
 */

#ifndef NVDLA_H_
#define NVDLA_H_

#include <math.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include "../accelerators.hpp"
using namespace std;

#define INPUT_DATA_TYPE 1
#define WEIGHT_DATA_TYPE 1
#define	COMPRESION_RATE	0
#define CONVOLUTION_BUFFER 128;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ROUND(number,mul) (((number%mul)!=0)?((number + mul)-(number%mul)):(number))

#define NVDLA_AREA 1000
#define	NVDLA_POWER 1000

enum layer_type{fc, conv};

class nvdla_layer
{
	public:
		nvdla_layer();
		~nvdla_layer();
		nvdla_layer(const nvdla_layer &n);

		//Methods for calculating the inner parameters of NN layer
		void 	calc_height_after_conv();
		void 	calc_width_after_conv();
		void	calc_height_after_pool();
		void	calc_width_after_pool();
		void	calc_num_of_calculations();
		void	calc_input_data_size();
		void	calc_min_input_data_size();
		void	calc_output_data_size();
		void	calc_weight_data_size();
		void	calc_min_weight_data_size();
		void	calc_dram_traffic();
		void	calc_dram_cycles();
		void	calc_mac_cycles();
		void	calc_max_cycle();

		//Calculates all - must be executed before returning information on the layer
		void	commit();

		//get methods - return interesting parameters on the layer
		double	get_max_cycle(){return mMAX_cycle;};
		double	get_mac_cycles(){return mMAC_cycles;};
		double	get_frequency(){return mFrequency;};
		double	get_calculations(){return mCalculations;};
		double	get_num_of_mul(){return mNumber_of_mul;};
		layer_type get_type(){return mLayer_type;}
		double	get_dram_traffic(){return mDRAM_traffic;};

		//set methods
		void	set_num_of_inputs(int num_of_inputs){mNum_of_input_channels_C = num_of_inputs;};
		void	set_input_height(int input_height){mInput_height_H = input_height;};
		void	set_input_width(int input_width){mInput_width_W = input_width;};
		void	set_num_of_outputs(int num_of_outputs){mNum_of_output_K = num_of_outputs;};
		void	set_filter_height(int filter_height){mFilter_height_R = filter_height;};
		void	set_filter_width(int filter_width){mFilter_width_S = filter_width;};
		void	set_zero_padding(bool zero_pad){mEnable_zero_pad_Z = zero_pad;};
		void	set_vertical_conv_dim(int vertical_conv_dim){mVertical_conv_H = vertical_conv_dim;};
		void	set_horizontal_conv_dim(int horizintal_conv_dim){mHorizonal_conv_V = horizintal_conv_dim;};
		void	set_pooling_after_conv(bool pooling){mEnable_pool_after_conv = pooling;};
		void	set_pooling_height(int height){mPooling_height_D = height;};
		void	set_pooling_width(int width){mPooling_width_E = width;};
		void	set_vertical_pooling_dim(int vertical_pool_dim){mVertical_pooling_F = vertical_pool_dim;};
		void	set_horizontal_pooling_dim(int horizontal_pool_dim){mHorizontal_pooling_G = horizontal_pool_dim;};
		void	set_winograd(int winograd){mWinograd = winograd;};
		void	set_layer_type(layer_type type){mLayer_type = type;}; // Only conv or fc
		void	set_fc_batch_size(int batch_size){mFc_batch_size = batch_size;};
		void	set_dram_bw_limit(int dram_bw_limit){mDRAM_bw_limit = dram_bw_limit;};
		void	set_frequency(int frequency){mFrequency = frequency;};
		void	set_num_of_mul(int num_of_mul){mNumber_of_mul = num_of_mul;};

	private:
		//Pre-defined member variables
		double 			mNum_of_input_channels_C = 0;
		double 			mInput_height_H = 0;
		double 			mInput_width_W = 0;
		double 			mNum_of_output_K = 0;
		double 			mFilter_height_R = 0;
		double 			mFilter_width_S = 0;
		bool 			mEnable_zero_pad_Z = 0;
		double 			mVertical_conv_H = 0;
		double 			mHorizonal_conv_V = 0;
		bool			mEnable_pool_after_conv = 0;
		double			mPooling_height_D = 0;
		double			mPooling_width_E = 0;
		double 			mVertical_pooling_F = 0;
		double 			mHorizontal_pooling_G = 0;
		int				mWinograd = 0;
		layer_type 		mLayer_type = conv;
		double			mFc_batch_size = 0;
		double 			mDRAM_bw_limit = 0;
		double			mFrequency = 0;
		double			mNumber_of_mul = 0;

		//Calculated by methods
		double	mHeight_after_conv = 0;
		double	mHeight_after_pool = 0;
		double	mWidth_after_conv = 0;
		double	mWidth_after_pool = 0;
		double	mCalculations = 0;
		double 	mInput_data_size = 0;
		double 	mMin_input_data_size = 0;
		double	mOutput_data_size = 0;
		double	mWeight_data_size = 0;
		double 	mMin_weight_data_size = 0;
		double	mDRAM_traffic = 0;
		double	mDRAM_cycles = 0;
		double	mMAC_cycles = 0;
		double	mMAX_cycle = 0;

};

class nvdla_acc
{
	public:

	nvdla_acc(int num_of_layers);
	~nvdla_acc();

	//Adds a new layer the network - as the last layer
	void	add_layer(nvdla_layer* new_layer);

	//Accelerator setup
	void clear(){network.clear();}; // Removes all layers
	void remove_layer(int i){network.erase(network.begin()+i-1);}; // removes layer number i
	void set_num_of_layers(int num){mNum_of_layers = num;};

	//Returns information on the full accelerator's performance - executed only after setting all the layers
	double	get_total_max_cycles();
	double	get_total_mac_cycles();
	double	get_total_time(); //ms
	double	get_fps();
	double	get_hw_mac_efficiency();
	double	get_network_mac_efficiency();
	double	get_total_calculations();
	int		get_num_of_layers(){return mNum_of_layers;};
	double 	get_total_dram_traffic();

	//Calculates all layers parameters - must be executed before returning information
	void	commit();

	private:

	int mNum_of_layers;
	std::vector<nvdla_layer*> network;

};

typedef struct config_nvdla
{
	int num_of_inputs;
	int input_height;
	int input_width;
	int num_of_outputs;
	int filter_height;
	int filter_width;
	bool zero_pad;
	int vertical_conv_dim;
	int horizintal_conv_dim;
	bool pooling;
	int pool_height;
	int pool_width;
	int vertical_pool_dim;
	int horizontal_pool_dim;
	int winograd;
	layer_type type;
	int batch_size;
	int dram_bw_limit; //GB/s
	int frequency; //MHz
	int num_of_mul;
} config_nvdla_t;

acc_perf_t sim_nvdla(config_nvdla_t config);

#endif /* NVDLA_H_ */
