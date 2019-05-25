/* Copyright 2017 Columbia University, SLD Group */

#ifndef __SDP_FUNCTIONS_HPP__
#define __SDP_FUNCTIONS_HPP__

// Computational kernels

// Utility functions
inline void sdp::load_store_handshake()
{
    HLS_DEFINE_PROTOCOL("load-store-handshake");

    output_done.ack.ack();
}

inline void sdp::store_load_handshake()
{
    HLS_DEFINE_PROTOCOL("store-load-handshake");

    output_done.req.req();
}


#endif // __SDP_FUNCTIONS_HPP__
