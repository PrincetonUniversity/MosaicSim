/* Copyright 2017 Columbia University, SLD Group */

#ifndef __WAMI_FPDATA_HPP__
#define __WAMI_FPDATA_HPP__

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <systemc.h>

#if defined(FIXED_POINT)

// Stratus fixed point

#include "cynw_fixed.h"

// Data types

const unsigned int WORD_SIZE = 64;

const unsigned int DWORD_SIZE = 128;

const unsigned int FPDATA_WL = 64; // 64

const unsigned int FPDATA_PL = 32; // 32

const unsigned int FPDATA_IL = 32; // 32

typedef sc_dt::sc_uint<WORD_SIZE> FPDATA_WORD;

typedef sc_dt::sc_biguint<DWORD_SIZE> FPDATA_DWORD;

typedef cynw_fixed<FPDATA_WL, FPDATA_IL> FPDATA;

// Helper functions

template<typename T, size_t N>
T bv2fp(sc_dt::sc_bv<N> data_in)
{
    T data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "bv2fp1");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "bv2fp-loop");

            data_out[i] = data_in[i].to_bool();
        }
    }

    return data_out;
}

template<typename T, size_t N>
void bv2fp(T &data_out, sc_dt::sc_bv<N> data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "bv2fp2");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "bv2fp-loop");

            data_out[i] = data_in[i].to_bool();
        }
    }
}

template<typename T, size_t N>
sc_dt::sc_bv<N> fp2bv(T data_in)
{
    sc_dt::sc_bv<N> data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2bv1");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "fp2bv-loop");

            data_out[i] = (bool) data_in[i];
        }
    }

    return data_out;
}

template<typename T, size_t N>
void fp2bv(sc_dt::sc_bv<N> &data_out, T data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2bv2");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "fp2bv-loop");

            data_out[i] = (bool) data_in[i];
        }
    }
}

// Helper functions

// T <---> sc_dt::sc_uint<N>

template<typename T, size_t N>
T int2fp(sc_dt::sc_uint<N> data_in)
{
    T data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "int2fp1");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "int2fp-loop");

            data_out[i] = data_in[i].to_bool();
        }
    }

    return data_out;
}

template<typename T, size_t N>
void int2fp(T &data_out, sc_dt::sc_uint<N> data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "int2fp2");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "int2fp-loop");

            data_out[i] = data_in[i].to_bool();
        }
    }
}

template<typename T, size_t N>
sc_dt::sc_uint<N> fp2int(T data_in)
{
    sc_dt::sc_uint<N> data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2int1");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "fp2int-loop");

            data_out[i] = (bool) data_in[i];
        }
    }

    return data_out;
}

template<typename T, size_t N>
void fp2int(sc_dt::sc_uint<N> &data_out, T data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2int2");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "fp2int-loop");

            data_out[i] = (bool) data_in[i];
        }
    }
}

#elif defined(FLOAT_POINT)

// Stratus floating point

#include "cynw_cm_float.h"

// Data types

const unsigned int WORD_SIZE = 64; // 64

const unsigned int FPDATA_ML = 52; // 52

const unsigned int FPDATA_EL = 11;

typedef sc_dt::sc_uint<WORD_SIZE> FPDATA_WORD;

typedef cynw_cm_float<FPDATA_EL, FPDATA_ML, CYNW_BEST_ACCURACY> FPDATA;

// Helper functions

// T <---> sc_dt::sc_uint<N>

template<typename T, size_t N>
T int2fp(sc_dt::sc_uint<N> data_in)
{
    T data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "uint2fp1");

        data_out.raw_bits(data_in);
        return data_out;
    }
}

template<typename T, size_t N>
void int2fp(T &data_out, sc_dt::sc_uint<N> data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "uint2fp2");

        data_out.raw_bits(data_in);
    }
}

template<typename T, size_t N>
sc_dt::sc_uint<N> fp2int(T data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2uint1");

        return sc_dt::sc_uint<N>(data_in.raw_bits());
    }
}

template<typename T, size_t N>
void fp2int(sc_dt::sc_uint<N> &data_out, T data_in)
{
     {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2uint2");

        data_out = sc_dt::sc_uint<N>(data_in.raw_bits());
     }
}

// T <---> sc_dt::sc_bv<N>

template<typename T, size_t N>
T bv2fp(sc_dt::sc_bv<N> data_in)
{
    T data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "bv2fp1");

        data_out.raw_bits(data_in);
        return data_out;
    }
}

template<typename T, size_t N>
void bv2fp(T &data_out, sc_dt::sc_bv<N> data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "bv2fp2");

        data_out.raw_bits(data_in.to_uint64());
    }
}

template<typename T, size_t N>
sc_dt::sc_bv<N> fp2bv(T data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2bv1");

        return sc_dt::sc_bv<N>(data_in.raw_bits());
    }
}

template<typename T, size_t N>
void fp2bv(sc_dt::sc_bv<N> &data_out, T data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2bv2");

        data_out = sc_dt::sc_bv<N>(data_in.raw_bits());
    }
}

#endif // defined(FIXED_POINT)

#endif // __WAMI_FPDATA_HPP__
