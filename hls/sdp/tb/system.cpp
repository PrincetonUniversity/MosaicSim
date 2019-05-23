/* Copyright 2018 Columbia University, SLD Group */

#include "system.hpp"

#include "nmf_fpdata.hpp"

#include "validation.hpp"

// -- Processes

void system_t::config_proc()
{
    conf_info_t config;

    // Reset

    {
        this->conf_info.write(config);
        this->conf_done.write(false);
        wait();
    }

    // Config

    {
        // Provide the input data
        load_memory(); wait();

        this->conf_info.write(config);
        this->conf_done.write(true);
    }

    // Compute

    {
        // Print information about begin time
        sc_time begin_time = sc_time_stamp();
        ESP_REPORT_TIME(begin_time, "BEGIN - sdp");

        // Wait the the termination of the accelerator
        do { wait(); } while (!this->acc_done.read());

        // Check the error code reported by the accelerator
        debug_info_t debug_code = debug.read();
        ESP_REPORT_INFO("exit code of sdp is %u", debug_code);

        // Print information about end time
        sc_time end_time = sc_time_stamp();
        ESP_REPORT_TIME(end_time, "END - sdp");

        // Log the latency for Stratus HLS
        sc_time latency = end_time - begin_time;
        esc_log_latency(this->clock_cycle(latency));
        wait(); this->conf_done.write(false);
    }

    // Conclude

    {
        dump_memory(); // store the output matrix in the data structure

        validate(); // check the results with the golden model

        sc_stop();
    }
}

void system_t::load_memory()
{
    ESP_REPORT_INFO("load memory completed");
}

void system_t::dump_memory()
{
    ESP_REPORT_INFO("dump memory completed");
}

int system_t::validate()
{
    { ESP_REPORT_INFO("validation succeeded!"); }

    return 0;
}
