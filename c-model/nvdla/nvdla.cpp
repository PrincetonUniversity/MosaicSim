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

double	nvdla_acc::get_total_dram_traffic()
{
	double total_traffic = 0;

	std::vector<nvdla_layer*>::iterator iter;
	for (iter = network.begin(); iter < network.end(); iter++)
	{
		total_traffic += (*iter)->get_dram_traffic();
	}

	return total_traffic;
};

double	nvdla_acc::get_total_time()
{
	double total_time = (this)->get_total_max_cycles();
	//cout << "total_time = " << total_time << endl;
	total_time = total_time / 1000 / network[0]->get_frequency();
	//cout << "total_time = " << total_time << endl;

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
	double	max_cycles = (this)->get_total_max_cycles();
	double	mac_cycles = (this)->get_total_mac_cycles();

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
	//cout << "calculations = " << calculations << endl;
	//cout << "total_cycles = " << total_cycles << endl;
	//cout << "network[0]->get_num_of_mul() = " << network[0]->get_num_of_mul() << endl;

	return calculations / (network[0]->get_num_of_mul()) / total_cycles;
};




//Methods for nvdla_layer

nvdla_layer::nvdla_layer()
{

}

nvdla_layer::nvdla_layer(const nvdla_layer &n)
{
	mNum_of_input_channels_C = n.mNum_of_input_channels_C;
	mInput_height_H = n.mInput_height_H;
	mInput_width_W = n.mInput_width_W;
	mNum_of_output_K = n.mNum_of_output_K;
	mFilter_height_R = n.mFilter_height_R;
	mFilter_width_S = n.mFilter_width_S;
	mEnable_zero_pad_Z = n.mEnable_zero_pad_Z;
	mVertical_conv_H = n.mVertical_conv_H;
	mHorizonal_conv_V = n.mHorizonal_conv_V;
	mEnable_pool_after_conv = n.mEnable_pool_after_conv;
	mPooling_height_D = n.mPooling_height_D;
	mPooling_width_E = n.mPooling_width_E;
	mVertical_pooling_F = n.mVertical_pooling_F;
	mHorizontal_pooling_G = n.mHorizontal_pooling_G;
	mWinograd = n.mWinograd;
	mLayer_type = n.mLayer_type;
	mFc_batch_size = n.mFc_batch_size;
	mDRAM_bw_limit = n.mDRAM_bw_limit;
	mFrequency = n.mFrequency;
	mNumber_of_mul = n.mNumber_of_mul;
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
			ceil( mInput_height_H/mVertical_conv_H) : ceil((mInput_height_H-mFilter_height_R)/mVertical_conv_H + 1);
};

void nvdla_layer::calc_width_after_conv()
{
	mWidth_after_conv = mEnable_zero_pad_Z ?
			ceil((mInput_width_W/mHorizonal_conv_V)): ceil((mInput_width_W-mFilter_width_S)/mHorizonal_conv_V + 1);
};

void	nvdla_layer::calc_height_after_pool()
{
	mHeight_after_pool = mEnable_pool_after_conv ?
			(ceil((mHeight_after_conv-mPooling_height_D)/mVertical_pooling_F) + 1) : mWidth_after_conv;

	//cout << "mHeight_after_pool = " << mHeight_after_pool << endl;
};

void	nvdla_layer::calc_width_after_pool()
{
	mWidth_after_pool = mEnable_pool_after_conv ?
			ceil((mWidth_after_conv-mPooling_width_E)/mHorizontal_pooling_G) : mWidth_after_conv;
};

void	nvdla_layer::calc_num_of_calculations()
{
	mCalculations = mNum_of_input_channels_C * mHeight_after_conv * mWidth_after_conv * mNum_of_output_K * mFilter_height_R * mFilter_width_S;
	//cout << "mNum_of_input_channels_C = " << mNum_of_input_channels_C << endl;
	//cout << "mHeight_after_conv = " << mHeight_after_conv << endl;
	//cout << "mWidth_after_conv = " << mWidth_after_conv << endl;
	//cout << "mNum_of_output_K = " << mNum_of_output_K << endl;
	//cout << "mFilter_height_R = " << mFilter_height_R << endl;
	//cout << "mFilter_width_S = " << mFilter_width_S << endl;
};

void	nvdla_layer::calc_input_data_size()
{
	mInput_data_size = 0;

	int convolution_size = ROUND((int)(mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int input_size = ceil((mInput_height_H / mVertical_conv_H)) * ceil(mInput_width_W/mHorizonal_conv_V);
	double type = INPUT_DATA_TYPE/1024.0; //KB

	mInput_data_size = convolution_size * input_size * type;
	//cout << "mInput_data_size = " << mInput_data_size << endl;

};

void	nvdla_layer::calc_min_input_data_size()
{
	mMin_input_data_size = 0;

	int convolution_size = ROUND((int)(mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int input_size = MIN(mFilter_height_R + 1, ceil((mInput_height_H / mVertical_conv_H))) * ceil(mInput_width_W/mHorizonal_conv_V);
	double type = INPUT_DATA_TYPE/1024.0; //KB

	mMin_input_data_size = convolution_size * input_size * type;
	//cout << "type = " << type << endl;
};

void	nvdla_layer::calc_output_data_size()
{
	mOutput_data_size = 0;
	int height = mHeight_after_pool * mHeight_after_pool;
	int output = ROUND((int)mNum_of_output_K, 16);
	double type = INPUT_DATA_TYPE/1024.0; //KB
	//cout << "mNum_of_output_K = " << mNum_of_output_K << endl;

	mOutput_data_size = height * output * type;
	//cout << "mOutput_data_size = " << mOutput_data_size << endl;
};

void	nvdla_layer::calc_weight_data_size()
{
	mWeight_data_size = 0;

	int convolution_size = ROUND((int)(mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int filter = ceil((mFilter_height_R / mVertical_conv_H)) * ceil((mFilter_width_S/mHorizonal_conv_V));
	int output = mNum_of_output_K * WEIGHT_DATA_TYPE;
	double type = (1-COMPRESION_RATE)/1024.0;//KB
	double winograd = mWinograd==1 ? (16.0/9) : 1;

	mWeight_data_size = convolution_size * filter * output * type * winograd;
	//cout << "winograd = " << winograd << endl;
}

void	nvdla_layer::calc_min_weight_data_size()
{
	mMin_weight_data_size = 0;

	int convolution_size = ROUND((int)(mNum_of_input_channels_C * mVertical_conv_H * mHorizonal_conv_V), 16);
	int filter = ceil((double) (mFilter_height_R / mVertical_conv_H)) * ceil((mFilter_width_S/mHorizonal_conv_V));
	int output = 32 * WEIGHT_DATA_TYPE;
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
	//cout << "mWeight_data_size = " << mWeight_data_size << endl;
	//cout << "mOutput_data_size = " << mOutput_data_size << endl;
	//cout << "mInput_data_size = " << mInput_data_size << endl;

};

void	nvdla_layer::calc_dram_cycles()
{
	mDRAM_cycles = ceil(mDRAM_traffic/mDRAM_bw_limit * mFrequency);
	//cout << "mDRAM_traffic = " << mDRAM_traffic << endl;

};

void	nvdla_layer::calc_mac_cycles()
{
	mMAC_cycles = 0;

	double input_size = ROUND((int)(mNum_of_input_channels_C*mVertical_conv_H*mHorizonal_conv_V), 16);
	double output_size = 0;

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
		output_size = ROUND((int)(mNum_of_output_K * mFc_batch_size), 16) * mFilter_height_R * mFilter_width_S / mVertical_conv_H / mHorizonal_conv_V / mNumber_of_mul / mFc_batch_size;
	}
	else
	{
		output_size = ROUND((int)mNum_of_output_K, 16) * mFilter_height_R * mFilter_width_S / mVertical_conv_H / mHorizonal_conv_V / mNumber_of_mul;
	}

	mMAC_cycles = input_size * output_size;
	//cout << "mFc_batch_size = " << mFc_batch_size << endl;

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
	//cout << "mDRAM_cycles = " << mDRAM_cycles << endl;
};


//Helper functions
acc_perf_t sim_nvdla(config_nvdla_t config)
{
	//Create an instance of the accelerator
	//You need to set the number of layers you want (in the example it is 1)
	nvdla_acc accelerator(1);

	//Create a neural network layer
	nvdla_layer* first_layer = new nvdla_layer();

	//Set all the parameters of a layer
	first_layer->set_num_of_inputs(config.num_of_inputs);
	first_layer->set_input_height(config.input_height);
	first_layer->set_input_width(config.input_width);
	first_layer->set_num_of_outputs(config.num_of_outputs);
	first_layer->set_filter_height(config.filter_height);
	first_layer->set_filter_width(config.filter_width);
	first_layer->set_zero_padding(config.zero_pad);
	first_layer->set_vertical_conv_dim(config.vertical_conv_dim);
	first_layer->set_horizontal_conv_dim(config.horizintal_conv_dim);
	first_layer->set_pooling_after_conv(config.pooling);
	first_layer->set_pooling_height(config.pool_height);
	first_layer->set_pooling_width(config.pool_width);
	first_layer->set_vertical_pooling_dim(config.vertical_pool_dim);
	first_layer->set_horizontal_pooling_dim(config.horizontal_pool_dim);
	first_layer->set_winograd(config.winograd);
	first_layer->set_layer_type(config.type); // Only conv or fc
	first_layer->set_fc_batch_size(config.batch_size);
	first_layer->set_dram_bw_limit(config.dram_bw_limit);
	first_layer->set_frequency(config.frequency);
	first_layer->set_num_of_mul(config.num_of_mul);

	//Insert the layer into the
	accelerator.add_layer(first_layer);

	//Calculate all performance attributes of the current accelerator
	accelerator.commit();

	acc_perf_t	nvdla_perf;

	nvdla_perf.cycles = accelerator.get_total_max_cycles();
	nvdla_perf.bytes = accelerator.get_total_dram_traffic() * 1000;
	nvdla_perf.area = NVDLA_AREA;
	nvdla_perf.power = NVDLA_POWER;
	nvdla_perf.bandwidth = nvdla_perf.bytes*1000/nvdla_perf.cycles;

	return nvdla_perf;
};
