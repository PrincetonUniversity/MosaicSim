/*
 * nvdla.cpp
 *
 *  Created on: May 17, 2019
 *      Author: geichler
 */

#include "nvdla.h"


//Methods for nvdla_acc

nvdla_acc::nvdla_acc(int num_of_layers)
{
	mNum_of_layers = num_of_layers;
};

nvdla_acc::~nvdla_acc()
{

}

void	nvdla_acc::commit()
{
	std::vector<nvdla_layer*>::iterator iter;
	for (iter = network.begin(); iter < network.end(); iter++)
		{
			(*iter)->commit();
		}
};

void	nvdla_acc::add_layer(nvdla_layer* new_layer)
{
	if(network.size() < mNum_of_layers)
	{
		network.push_back(new_layer);
	}
};

double	nvdla_acc::get_total_max_cycles()
{
	double total_cycles = 0;

	std::vector<nvdla_layer*>::iterator iter;
	for (iter = network.begin(); iter < network.end(); iter++)
	{
		total_cycles += (*iter)->get_max_cycle();
	}

	return total_cycles;
};

double	nvdla_acc::get_total_mac_cycles()
{
	double total_cycles = 0;

	std::vector<nvdla_layer*>::iterator iter;
	for (iter = network.begin(); iter < network.end(); iter++)
	{
		total_cycles += (*iter)->get_mac_cycles();
	}

	return total_cycles;
};

double	nvdla_acc::get_total_time()
{
	double total_time = (this)->get_total_max_cycles();

	total_time /= 1000 / network[0]->get_frequency();

	return total_time;
};

double	nvdla_acc::get_fps()
{
	double total_fps = (this)->get_total_time();

	total_fps = 1000 / total_fps;

	return total_fps;
};

double	nvdla_acc::get_hw_mac_efficiency()
{
	double	max_cycles;
	double	mac_cycles;

	return mac_cycles/max_cycles;
};

double	nvdla_acc::get_total_calculations()
{
	double total_calculations = 0;

		std::vector<nvdla_layer*>::iterator iter;
		for (iter = network.begin(); iter < network.end(); iter++)
		{
			total_calculations += (*iter)->get_calculations();
		}

		return total_calculations;
}

double	nvdla_acc::get_network_mac_efficiency()
{
	double calculations = (this)->get_total_calculations();
	double total_cycles = (this)->get_total_max_cycles();

	return calculations / (network[0]->get_num_of_mul()) / total_cycles;
};




//Methods for nvdla_layer

nvdla_layer::nvdla_layer()
{

}

nvdla_layer::~nvdla_layer()
{

}

void	nvdla_layer::commit()
{
	calc_height_after_conv();
	calc_width_after_conv();
	calc_height_after_pool();
	calc_width_after_pool();
	calc_num_of_calculations();
	calc_input_data_size();
	calc_min_input_data_size();
	calc_output_data_size();
	calc_weight_data_size();
	calc_min_weight_data_size();
	calc_dram_traffic();
	calc_dram_cycles();
	calc_mac_cycles();
	calc_max_cycle();
};

void nvdla_layer::calc_height_after_conv()
{
	mHeight_after_conv = mEnable_zero_pad_Z ?
			ceil( mInput_height_H/mVertical_conv_H) : (ceil((mInput_height_H-mFilter_height_R)/mVertical_conv_H)) + 1;
};

void nvdla_layer::calc_width_after_conv()
{
	mWidth_after_conv = mEnable_zero_pad_Z ?
			ceil((mInput_width_W/mHorizonal_conv_V)): (ceil(mInput_width_W-mFilter_width_S)/mHorizonal_conv_V) + 1;
};

void	nvdla_layer::calc_height_after_pool()
{
	mHeight_after_pool = mEnable_pool_after_conv ?
			ceil((mHeight_after_conv-mPooling_height_D)/mVertical_pooling_F) : mHeight_after_pool;
};

void	nvdla_layer::calc_width_after_pool()
{
	mWidth_after_pool = mEnable_pool_after_conv ?
			ceil((mWidth_after_conv-mPooling_width_E)/mHorizontal_pooling_G) : mWidth_after_conv;
};

void	nvdla_layer::calc_num_of_calculations()
{
	mCalculations = mNum_of_input_channels_C * mHeight_after_conv * mWidth_after_conv * mNum_of_output_K * mFilter_height_R * mFilter_width_S;
};

void	nvdla_layer::calc_input_data_size()
{
	mInput_data_size = 0;

	int convolution_size = ROUND((mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int input_size = ceil((mInput_height_H / mVertical_conv_H)) * ceil(mInput_width_W/mHorizonal_conv_V);
	int type = INPUT_DATA_TYPE/1024; //KB

	mInput_data_size = convolution_size * input_size * type;
};

void	nvdla_layer::calc_min_input_data_size()
{
	mMin_input_data_size = 0;

	int convolution_size = ROUND((mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int input_size = MIN(mFilter_height_R + 1, ceil((mInput_height_H / mVertical_conv_H))) * ceil(mInput_width_W/mHorizonal_conv_V);
	int type = INPUT_DATA_TYPE/1024; //KB

	mInput_data_size = convolution_size * input_size * type;
};

void	nvdla_layer::calc_output_data_size()
{
	mOutput_data_size = 0;
	double height = mHeight_after_conv * mHeight_after_pool;
	double output = ROUND(mNum_of_output_K, 16);
	int type = INPUT_DATA_TYPE/1024; //KB

	mOutput_data_size = height * output * type;
};

void	nvdla_layer::calc_weight_data_size()
{
	mWeight_data_size = 0;

	int convolution_size = ROUND((mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int filter = ceil((mFilter_height_R / mVertical_conv_H)) * ceil((mFilter_width_S/mHorizonal_conv_V));
	double output = mNum_of_output_K * WEIGHT_DATA_TYPE;
	double type = (1-COMPRESION_RATE) / 1024;//KB

	mWeight_data_size = convolution_size * filter * output * type;
}

void	nvdla_layer::calc_min_weight_data_size()
{
	mMin_weight_data_size = 0;

	int convolution_size = ROUND((mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int filter = ceil((double) (mFilter_height_R / mVertical_conv_H)) * ceil((mFilter_width_S/mHorizonal_conv_V));
	double output = 32 * WEIGHT_DATA_TYPE;
	double type = (1-COMPRESION_RATE) / 1024;//KB

	double result = convolution_size * filter * output * type;
	double buffer_size = CONVOLUTION_BUFFER;

	if (result >= buffer_size)
	{

		mMin_weight_data_size = buffer_size/2;
	}
	else
	{
		mMin_weight_data_size = result;
	}
};

void	nvdla_layer::calc_dram_traffic()
{
	mDRAM_traffic = 0;
	int batch_size;

	if(mLayer_type == fc)
	{
		batch_size = mFc_batch_size;
	}
	else
	{
		batch_size = 1;
	}

	mDRAM_traffic = mWeight_data_size/batch_size + mOutput_data_size + mInput_data_size;

};

void	nvdla_layer::calc_dram_cycles()
{
	mDRAM_cycles = ceil(mDRAM_traffic/mDRAM_bw_limit * mFrequency);
};

void	nvdla_layer::calc_mac_cycles()
{
	mMAC_cycles = 0;

	int input_size = ROUND(mNum_of_input_channels_C*mVertical_conv_H*mHorizonal_conv_V, 16);
	int output_size = 0;

	if(mEnable_zero_pad_Z)
	{
		input_size = input_size * ceil(mInput_height_H/mVertical_conv_H) * ceil(mInput_width_W/mHorizonal_conv_V);
	}
	else
	{
		input_size = input_size * ceil((mInput_height_H - mFilter_height_R + 1)/mVertical_conv_H) * ceil((mInput_width_W - mFilter_width_S + 1)/mHorizonal_conv_V);
	}

	if(mLayer_type == fc)
	{
		output_size = ROUND(mNum_of_output_K * mFc_batch_size, 16) * mFilter_height_R * mFilter_width_S / mVertical_conv_H / mHorizonal_conv_V / mNumber_of_mul / mFc_batch_size;
	}
	else
	{
		output_size = ROUND(mNum_of_output_K, 16) * mFilter_height_R * mFilter_width_S / mVertical_conv_H / mHorizonal_conv_V / mNumber_of_mul;
	}

	mMAC_cycles = input_size * output_size;

	if(mFilter_height_R == 3)
	{
		if(mFilter_width_S == 3)
		{
			mMAC_cycles /= 2.25;
		}
	}
};

void	nvdla_layer::calc_max_cycle()
{
	mMAX_cycle = MAX(mDRAM_cycles, mMAC_cycles);
};
