/* Copyright 2018 Columbia University, SLD Group */

#ifndef __VALIDATION_HPP__
#define __VALIDATION_HPP__

// REPORT_THRESHOLD: the maximum number of errors to report
#define REPORT_THRESHOLD 10

// MAX_ERROR_THRESHOLD: the maximum relative error (percentage)
#define MAX_ERROR_THRESHOLD 0.0001

bool check_error_threshold(double out, double gold, double &error)
{
    assert(!isinf(gold) && !isnan(gold));

    // return an error if out is Infinite or NaN
    if (isinf(out) || isnan(out)) { return true; }

    if (gold != 0)
        error = fabs((gold - out) / gold);
    else
        error = fabs((out - gold) / out);

    return (error > MAX_ERROR_THRESHOLD);
}

#endif // __VALIDATION_HPP__
