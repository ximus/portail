// http://coactionos.com/embedded%20design%20tips/2013/10/04/Tips-An-Easy-to-Use-Digital-Filter/

#define STATS_EMA_ALPHA(x) ( (uint16_t)(x * 65535) )

static inline int16_t stats_ema_alpha(float x)
{
    return STATS_EMA_ALPHA(x);
}


static inline uint16_t stats_ema_ui16(uint16_t in, uint16_t average, uint16_t alpha)
{
    // optimized out:
    // if ( alpha == 65535 ) return in;
    // uint64 prevents overflow. At most I need 2^16*1 + 2^16 * 2^16 > 2^32
    uint64_t tmp0;
    tmp0 = (uint64_t)in * alpha + average * (65536 - alpha);
    //scale back to 32-bit (with rounding)
    return (uint16_t)((tmp0 + 32768) / 65536);
}