/* Copyright 2018 Columbia University, SLD Group */

#ifndef __SDP_CONFIG_HPP__
#define __SDP_CONFIG_HPP__

// Class to configure the accelerator

class conf_info_t
{
public:

    // Working mode of the accelerator
    // 0: tensors add
    // 1: tensors sub
    // 2: tensors mul
    // 3: tensors div
    // 4: scalar-tensor add
    // 5: scalar-tensor sub
    // 6: scalar-tensor mul
    // 7: scalar-tensor div
    // 8: ReLU activation
    uint32_t working_mode;

    // Size of the matrix A
    uint32_t sizeA;

    // Size of the matrix B
    uint32_t sizeB;

    // Scalar value
    uint32_t scalar;

    // Input offset (matrix 1)
    uint32_t ld_offsetA;

    // Input offset (matrix 2)
    uint32_t ld_offsetB;

    // Output offset
    uint32_t st_offset;

    conf_info_t()
	: working_mode(0)
	, sizeA(0)
	, sizeB(0)
	, scalar(0)
	, ld_offsetA(0)
	, ld_offsetB(0)
	, st_offset(0)
        {
            // Nothing to do
        }

    inline bool operator==(const conf_info_t &rhs) const
        {
            return (rhs.working_mode == working_mode)
                && (rhs.sizeA == sizeA)
                && (rhs.sizeB == sizeB)
                && (rhs.scalar == scalar)
                && (rhs.ld_offsetA == ld_offsetA)
                && (rhs.ld_offsetB == ld_offsetB)
                && (rhs.st_offset == st_offset);
        }

    inline conf_info_t &operator=(const conf_info_t &other)
        {
            working_mode = other.working_mode;
            sizeA = other.sizeA;
            sizeB = other.sizeB;
            scalar = other.scalar;
            ld_offsetA = other.ld_offsetA;
            ld_offsetB = other.ld_offsetB;
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

#endif // __SDP_CONFIG_HPP__
