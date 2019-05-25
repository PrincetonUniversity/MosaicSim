/* Copyright 2017 Columbia University, SLD Group */

#ifndef __WAMI_RGB_PIXEL_HPP__
#define __WAMI_RGB_PIXEL_HPP__

#include <systemc.h>

class rgb_pixel_t
{
    public:

        sc_uint<16> r;
        sc_uint<16> g;
        sc_uint<16> b;

        static const unsigned hardware_bits = 64;
        typedef sc_uint< hardware_bits > memory_word_t;

        // Constructors

        rgb_pixel_t()
            : r(0)
            , g(0)
            , b(0) { }

        rgb_pixel_t(unsigned short _r, unsigned short _g, unsigned short _b)
            : r(_r)
            , g(_g)
            , b(_b) { }

        // Equals operator
        inline bool operator==(const rgb_pixel_t &rhs) const
        {
            return (rhs.r == r)
                   && (rhs.g == g)
                   && (rhs.b == b);
        }

        // Assign operator
        inline rgb_pixel_t &operator=(const rgb_pixel_t &other)
        {
            r = other.r;
            g = other.g;
            b = other.b;

            return *this;
        }

        // Dump function
        friend void sc_trace(sc_trace_file *tf, const rgb_pixel_t &rgb_pixel,
                             const std::string &name)
        {
            sc_trace(tf, rgb_pixel.r, name + std::string(".r"));
            sc_trace(tf, rgb_pixel.g, name + std::string(".g"));
            sc_trace(tf, rgb_pixel.b, name + std::string(".b"));
        }

        // Dump operator
        friend ostream &operator<<(ostream &os, rgb_pixel_t const &rgb_pixel)
        {
            os << "(" << rgb_pixel.r << ", " << rgb_pixel.g << ", " << rgb_pixel.b << ")";
            return os;
        }

};

// cynw_interpret going to a memory word
inline void cynw_interpret(const rgb_pixel_t &in, rgb_pixel_t::memory_word_t &out)
{
    out = (sc_uint<16>(0), in.r, in.g, in.b);
}

// cynw_interpret going from a memory word
inline void cynw_interpret(const rgb_pixel_t::memory_word_t &in, rgb_pixel_t &out)
{
    out.r = in.range(47, 32);
    out.g = in.range(31, 16);
    out.b = in.range(15, 0);
}

#endif // __WAMI_RGB_PIXEL_HPP__
