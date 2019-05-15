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

// This removes the tags
#define dift_uint32_t(x, y) (y)

        config.d1 = dift_uint32_t(SRC_TAG, matrix_in1->dims[0]);
        config.d2 = dift_uint32_t(SRC_TAG, matrix_in1->dims[1]);
        config.d3 = dift_uint32_t(SRC_TAG, matrix_in2->dims[0]);

        config.ld_offset1 = dift_uint32_t(SRC_TAG, 0);

        config.ld_offset2 = index1;

        config.st_offset  = dift_uint32_t(SRC_TAG, index2);

#undef dift_uint32_t

        // ESP_REPORT_INFO("offsets: %d %d", config.ld_offset2.val, config.st_offset.val);

        this->conf_info.write(config);
        this->conf_done.write(true);
    }

    // Compute

    {
        // Print information about begin time
        sc_time begin_time = sc_time_stamp();
        ESP_REPORT_TIME(begin_time, "BEGIN - multt");

        // Wait the the termination of the accelerator
        do { wait(); } while (!this->acc_done.read());

        // Check the error code reported by the accelerator
        debug_info_t debug_code = debug.read();
        ESP_REPORT_INFO("exit code of multt is %u", debug_code);

        // Print information about end time
        sc_time end_time = sc_time_stamp();
        ESP_REPORT_TIME(end_time, "END - multt");

        // Log the latency for Stratus HLS
        sc_time latency = end_time - begin_time;
        esc_log_latency(this->clock_cycle(latency));
        wait(); this->conf_done.write(false);
    }

    // Conclude

    {
        dump_memory(); // store the output matrix in the data structure

        validate(); // check the results with the golden model

        debug_info_t debug_code = debug.read();
        if (debug_code != 0) { goto free_and_terminate; }

        store_double_matrix_t(matrix_out, esc_argv()[3]);

        free_double_matrix_t(&matrix_out_gold);

        free_double_matrix_t(&matrix_out);

free_and_terminate:

        free_double_matrix_t(&matrix_in1);

        free_double_matrix_t(&matrix_in2);

        sc_stop();
    }
}

void system_t::load_memory()
{
    //  Memory allocation (without considering the tags):
    //
    //  =========================   ^
    //  |     matrix (input1)   |   |  (matrix_in1->dim[0] * matrix_in1->dim[1])
    //  =========================   ^
    //  |     matrix (input2)   |   |  (matrix_in2->dim[0] * matrix_in2->dim[1])
    //  =========================   v
    //  |     matrix (output)   |   |  (matrix_in1->dim[0] * matrix_in2->dim[1])
    //  =========================   v

    sc_dt::sc_bv<64> data;

    index1 = 0;
    index2 = 0;

    if (esc_argc() != 4)
    {
        ESP_REPORT_ERROR("usage: %s <input1> <input2> <output> \n", esc_argv()[0]);
        sc_stop();
    }

    //
    // Matrix 1
    //

    if (load_double_matrix_t(&matrix_in1, esc_argv()[1]) < 0)
    {
        ESP_REPORT_ERROR("reading matrix 1 failed!\n");
        sc_stop();
    }

    ESP_REPORT_INFO("dimension matrix1 d1 %d", matrix_in1->dims[0]);
    ESP_REPORT_INFO("dimension matrix1 d2 %d", matrix_in1->dims[1]);

    for (uint32_t d1 = 0; d1 < matrix_in1->dims[0]; ++d1)
    {
       for (uint32_t d2 = 0; d2 < matrix_in1->dims[1]; ++d2)
       {
            uint32_t k = d1 * matrix_in1->dims[1] + d2;

            data = fp2bv<FPDATA, 64>(FPDATA(matrix_in1->data[k]));

            this->mem[index1++] = data.range(63, 32);

            this->mem[index1++] = data.range(31, 0);
 	}
    }

    //
    // Matrix 2
    //

    index2 = index1;

    if (load_double_matrix_t(&matrix_in2, esc_argv()[2]) < 0)
    {
        ESP_REPORT_ERROR("reading matrix 2 failed!\n");
        sc_stop();
    }

    assert(matrix_in1->dims[1] == matrix_in2->dims[1]);

    ESP_REPORT_INFO("dimension matrix2 d1 %d", matrix_in2->dims[0]);
    ESP_REPORT_INFO("dimension matrix2 d2 %d", matrix_in2->dims[1]);

    for (uint32_t d1 = 0; d1 < matrix_in2->dims[0]; ++d1)
    {
       for (uint32_t d2 = 0; d2 < matrix_in2->dims[1]; ++d2)
       {
            uint32_t k = d1 * matrix_in2->dims[1] + d2;

            data = fp2bv<FPDATA, 64>(FPDATA(matrix_in2->data[k]));

            this->mem[index2++] = data.range(63, 32);

            this->mem[index2++] = data.range(31, 0);
 	}
    }

    ESP_REPORT_INFO("load memory completed");
}

void system_t::dump_memory()
{
#ifndef ENABLE_WRONG_FIRST_TAG

    FPDATA elem;
    sc_dt::sc_bv<64> data;

    size_t sizes[2] = {matrix_in1->dims[0],
                       matrix_in2->dims[0]};

    create_double_matrix_t(&matrix_out, sizes, 2);

    // for (int i = index2; i < index2 + 200; i++)
    // { ESP_REPORT_INFO("mem[%d] ==> %08x", i, this->mem[i].to_uint()); }

    for (uint32_t d1 = 0; d1 < matrix_in1->dims[0]; ++d1)
    {
       for (uint32_t d2 = 0; d2 < matrix_in2->dims[0]; ++d2)
       {
            uint32_t k = d1 * matrix_in2->dims[0] + d2;

            data.range(63, 32) = this->mem[index2++];

            data.range(31, 0) = this->mem[index2++];

            bv2fp<FPDATA, WORD_SIZE>(elem, data);

            matrix_out->data[k] = elem.to_double();
        }
    }

    ESP_REPORT_INFO("dump memory completed");

#else // DISABLE_WRONG_FIRST_TAG

    uint32_t i = index2;
    while (this->mem[i] != sc_dt::sc_bv<32>(0)) i++;

    ESP_REPORT_INFO("leakage of #%d out of #%d (%.2lf%%)",
      i - index2, matrix_in1->dims[0] * matrix_in2->dims[0],
      ((((double) (i - index2)) / (double) (matrix_in1->dims[0]
      * matrix_in2->dims[0]) * 100.0));

#endif // ENABLE_WRONG_FIRST_TAG
}

int system_t::validate()
{
#ifndef ENABLE_WRONG_FIRST_TAG

    double rel_error = 0.0;
    double avg_error = 0.0;
    double max_error = 0.0;

    uint32_t tot_errors = 0;

    // Call the programmer's view function
    multt(matrix_in1, matrix_in2, &matrix_out_gold);

    for (uint32_t d2 = 0; d2 < matrix_out_gold->dims[0] * matrix_out_gold->dims[1]; ++d2)
    {
        if (check_error_threshold(matrix_out->data[d2], matrix_out_gold->data[d2], rel_error))
        {
            if (tot_errors < REPORT_THRESHOLD)
            {
                ESP_REPORT_INFO("multt[%d] = %lf (%lf) error: %.2lf%%", d2,
                  matrix_out->data[d2], matrix_out_gold->data[d2], rel_error * 100);
            }

            tot_errors++;
        }

        // Tracking the maximum error w.r.t. the programmer's view
        if (rel_error > max_error) { max_error = rel_error; }

        // Tracking the average error w.r.t. the programmer's view
        avg_error += rel_error;
    }

    avg_error /= (double) (matrix_out_gold->dims[0] * matrix_out_gold->dims[1]);

    ESP_REPORT_INFO("errors #%d out of #%d", tot_errors,(
      matrix_out_gold->dims[0] * matrix_out_gold->dims[1]));
    ESP_REPORT_INFO("average error: %.2lf%%", avg_error * 100);
    ESP_REPORT_INFO("maximum error: %.2lf%%", max_error * 100);

    if (tot_errors > 0)
        { ESP_REPORT_ERROR("validation failed!"); }
    else
        { ESP_REPORT_INFO("validation succeeded!"); }

    ESP_REPORT_INFO("total memory #%d bytes",
      (unsigned int) (index2 * 4UL) / 1000UL);

#endif // ENABLE_WRONG_FIRST_TAG

    return 0;
}
