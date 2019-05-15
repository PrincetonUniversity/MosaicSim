/* Copyright 2018 Columbia University, SLD Group */

#ifndef __NMF_MULTT_CONFIG_HPP__
#define __NMF_MULTT_CONFIG_HPP__

// Class to configure the accelerator

class conf_info_t
{
    public:

        // Size d1 of the matrix 1
        uint32_t d1;

        // Size d2 of the matrix 1
        uint32_t d2;

        // Size d2 of the matrix 2
        uint32_t d3;

        // Input offset (matrix 1)
        uint32_t ld_offset1;

        // Input offset (matrix 2)
        uint32_t ld_offset2;

        // Output offset
        uint32_t st_offset;

        conf_info_t()
            : d1(0)
            , d2(0)
            , d3(0)
            , ld_offset1(0)
            , ld_offset2(0)
            , st_offset(0)
        {
            // Nothing to do
        }

        inline bool operator==(const conf_info_t &rhs) const
        {
            return (rhs.d1 == d1)
                && (rhs.d2 == d2)
                && (rhs.d3 == d3)
                && (rhs.ld_offset1 == ld_offset1)
                && (rhs.ld_offset2 == ld_offset2)
                && (rhs.st_offset == st_offset);
        }

        inline conf_info_t &operator=(const conf_info_t &other)
        {
            d1 = other.d1;
            d2 = other.d2;
            d3 = other.d3;
            ld_offset1 = other.ld_offset1;
            ld_offset2 = other.ld_offset2;
            st_offset = other.st_offset;
            return *this;
        }

        friend ostream& operator<<(ostream &stream, const conf_info_t &conf)
        {
           // Note: not supported
           return stream;
        }

        friend void sc_trace(sc_trace_file *tf, const conf_info_t &conf,
          const std::string &name)
        {
            // Note: not supported
        }
};

#endif // __NMF_MULTT_CONFIG_HPP__
