/* Copyright 2018 Columbia University, SLD Group */

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "sdp.h"

#include "sdp_wrap.h"

#include "esp_templates.hpp"

const size_t MEM_SIZE = 2048 * 2048 * 16;

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
    public:

        // -- Modules
        sdp_wrapper *acc;

        // -- Module constructor

        SC_HAS_PROCESS(system_t);
        system_t(sc_module_name name)
		: esp_system<DMA_WIDTH, MEM_SIZE>(name)

        {
            // Instantiate the accelerator
            acc = new sdp_wrapper("wrapper");

            // Binding the accelerator
            acc->clk(this->clk);
            acc->rst(this->rst);
            acc->dma_read_ctrl(this->dma_read_ctrl);
            acc->dma_write_ctrl(this->dma_write_ctrl);
            acc->dma_read_chnl(this->dma_read_chnl);
            acc->dma_write_chnl(this->dma_write_chnl);
            acc->conf_info(this->conf_info);
            acc->conf_done(this->conf_done);
            acc->acc_done(this->acc_done);
            acc->debug(this->debug);
        }

        // -- Module destructor

        ~system_t()
        {
            delete acc;
        }

        // -- Processes

        // Configure accelerator
        void config_proc();

        // -- Functions

        // Load internal memory
        void load_memory();

        // Dump internal memory
        void dump_memory();

        // Validate results
        int validate();

        // -- Private data
};

#endif // __SYSTEM_HPP__
