#include "./rationalExtensions.h"

AVRational make_rounded(int num, int den)
{
    AVRational newRational;
    if (av_reduce(&newRational.num, &newRational.den, num, den, LONG_MAX) == 1)
    {
        return newRational;
    }
    return av_make_q(num, den);
}

AVRational multiply_by_int(AVRational rational, long mult)
{
    return make_rounded(rational.num * mult, rational.den);
}

AVRational add_int(AVRational rational, long added)
{
    return av_add_q(rational, av_make_q(added, 1));
}

AVRational subtract_int(AVRational rational, long subtracted)
{
    return av_sub_q(rational, av_make_q(subtracted, 1));
}