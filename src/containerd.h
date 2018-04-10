#ifndef __containerd_H__
#define __containerd_H__


namespace containerd
{


enum Algo {
    ALGO_crxday,
    ALGO_crxday_LITE,
};


enum Variant {
    VARIANT_AUTO = -1,
    VARIANT_NONE = 0,
    VARIANT_V1   = 1
};

}


#endif