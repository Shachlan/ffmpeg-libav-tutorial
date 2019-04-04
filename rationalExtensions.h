#include <libavutil/rational.h>

AVRational make_rounded(int num, int den);

AVRational multiply_by_int(AVRational rational, long mult);

AVRational subtract_int(AVRational rational, long subtracted);

AVRational add_int(AVRational rational, long added);