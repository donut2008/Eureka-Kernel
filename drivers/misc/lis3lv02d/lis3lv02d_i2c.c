__x) {return expm1(__x);}

static long double
    _TG_ATTRS
    __tg_expm1(long double __x) {return expm1l(__x);}

#undef expm1
#define expm1(__x) __tg_expm1(__tg_promote1((__x))(__x))

// fdim

static float
    _TG_ATTRS
    __tg_fdim(float __x, float __y) {return fdimf(__x, __y);}

static double
    _TG_ATTRS
    __tg_fdim(double __x, double __y) {return fdim(__x, __y);}

static long double
    _TG_ATTRS
    __tg_fdim(long double __x, long double __y) {return fdiml(__x, __y);}

#undef fdim
#define fdim(__x, __y) __tg_fdim(__tg_promote2((__x), (__y))(__x), \
                                 __tg_promote2((__x), (__y))(__y))

// floor

static float
    _TG_ATTRS
    __tg_floor(float __x) {return floorf(__x);}

static double
    _TG_ATTRS
    __tg_floor(double __x) {return floor(__x);}

static long double
    _TG_ATTRS
    __tg_floor(long double __x) {return floorl(__x);}

#undef floor
#define floor(__x) __tg_floor(__tg_promote1((__x))(__x))

// fma

static float
    _TG_ATTRS
    __tg_fma(float __x, float __y, float __z)
    {return fmaf(__x, __y, __z);}

static double
    _TG_ATTRS
    __tg_fma(double __x, double __y, double __z)
    {return fma(__x, __y, __z);}

static long double
    _TG_ATTRS
    __tg_fma(long double __x,long double __y, long double __z)
    {return fmal(__x, __y, __z);}

#undef fma
#define fma(__x, __y, __z)                                \
        __tg_fma(__tg_promote3((__x), (__y), (__z))(__x), \
                 __tg_promote3((__x), (__y), (__z))(__y), \
                 __tg_promote3((__x), (__y), (__z))(__z))

// fmax

static float
    _TG_ATTRS
    __tg_fmax(float __x, float __y) {return fmaxf(__x, __y);}

static double
    _TG_ATTRS
    __tg_fmax(double __x, double __y) {return fmax(__x, __y);}

static long double
    _TG_ATTRS
    __tg_fmax(long double __x, long double __y) {return fmaxl(__x, __y);}

#undef fmax
#define fmax(__x, __y) __tg_fmax(__tg_promote2((__x), (__y))(__x), \
                                 __tg_promote2((__x), (__y))(__y))

// fmin

static float
    _TG_ATTRS
    __tg_fmin(float __x, float __y) {return fminf(__x, __y);}

static double
    _TG_ATTRS
    __tg_fmin(double __x, double __y) {return fmin(__x, __y);}

static long double
    _TG_ATTRS
    __tg_fmin(long double __x, long double __y) {return fminl(__x, __y);}

#undef fmin
#define fmin(__x, __y) __tg_fmin(__tg_promote2((__x), (__y))(__x), \
                                 __tg_promote2((__x), (__y))(__y))

// fmod

static float
    _TG_ATTRS
    __tg_fmod(float __x, float __y) {return fmodf(__x, __y);}

static double
    _TG_ATTRS
    __tg_fmod(double __x, double __y) {return fmod(__x, __y);}

static long double
    _TG_ATTRS
    __tg_fmod(long double __x, long double __y) {return fmodl(__x, __y);}

#undef fmod
#define fmod(__x, __y) __tg_fmod(__tg_promote2((__x), (__y))(__x), \
                                 __tg_promote2((__x), (__y))(__y))

// frexp

static float
    _TG_ATTRS
    __tg_frexp(float __x, int* __y) {return frexpf(__x, __y);}

static double
    _TG_ATTRS
    __tg_frexp(double __x, int* __y) {return frexp(__x, __y);}

static long double
    _TG_ATTRS
    __tg_frexp(long double __x, int* __y) {return frexpl(__x, __y);}

#undef frexp
#define frexp(__x, __y) __tg_frexp(__tg_promote1((__x))(__x), __y)

// hypot

static float
    _TG_ATTRS
    __tg_hypot(float __x, float __y) {return hypotf(__x, __y);}

static double
    _TG_ATTRS
    __tg_hypot(double __x, double __y) {return hypot(__x, __y);}

static long double
    _TG_ATTRS
    __tg_hypot(long double __x, long double __y) {return hypotl(__x, __y);}

#undef hypot
#define hypot(__x, __y) __tg_hypot(__tg_promote2((__x), (__y))(__x), \
                                   __tg_promote2((__x), (__y))(__y))

// ilogb

static int
    _TG_ATTRS
    __tg_ilogb(float __x) {return ilogbf(__x);}

static int
    _TG_ATTRS
    __tg_ilogb(double __x) {return ilogb(__x);}

static int
    _TG_ATTRS
    __tg_ilogb(long double __x) {return ilogbl(__x);}

#undef ilogb
#define ilogb(__x) __tg_ilogb(__tg_promote1((__x))(__x))

// ldexp

static float
    _TG_ATTRS
    __tg_ldexp(float __x, int __y) {return ldexpf(__x, __y);}

static double
    _TG_ATTRS
    __tg_ldexp(double __x, int __y) {return ldexp(__x, __y);}

static long double
    _TG_ATTRS
    __tg_ldexp(long double __x, int __y) {return ldexpl(__x, __y);}

#undef ldexp
#define ldexp(__x, __y) __tg_ldexp(__tg_promote1((__x))(__x), __y)

// lgamma

static float
    _TG_ATTRS
    __tg_lgamma(float __x) {return lgammaf(__x);}

static double
    _TG_ATTRS
    __tg_lgamma(double __x) {return lgamma(__x);}

static long double
    _TG_ATTRS
    __tg_lgamma(long double __x) {return lgammal(__x);}

#undef lgamma
#define lgamma(__x) __tg_lgamma(__tg_promote1((__x))(__x))

// llrint

static long long
    _TG_ATTRS
    __tg_llrint(float __x) {return llrintf(__x);}

static long long
    _TG_ATTRS
    __tg_llrint(double __x) {return llrint(__x);}

static long long
    _TG_ATTRS
    __tg_llrint(long double __x) {return llrintl(__x);}

#undef llrint
#define llrint(__x) __tg_llrint(__tg_promote1((__x))(__x))

// llround

static long long
    _TG_ATTRS
    __tg_llround(float __x) {return llroundf(__x);}

static long long
    _TG_ATTRS
    __tg_llround(double __x) {return llround(__x);}

static long long
    _TG_ATTRS
    __tg_llround(long double __x) {return llroundl(__x);}

#undef llround
#define llround(__x) __tg_llround(__tg_promote1((__x))(__x))

// log10

static float
    _TG_ATTRS
    __tg_log10(float __x) {return log10f(__x);}

static double
    _TG_ATTRS
    __tg_log10(double __x) {return log10(__x);}

static long double
    _TG_ATTRS
    __tg_log10(long double __x) {return log10l(__x);}

#undef log10
#define log10(__x) __tg_log10(__tg_promote1((__x))(__x))

// log1p

static float
    _TG_ATTRS
    __tg_log1p(float __x) {return log1pf(__x);}

static double
    _TG_ATTRS
    __tg_log1p(double __x) {return log1p(__x);}

static long double
    _TG_ATTRS
    __tg_log1p(long double __x) {return log1pl(__x);}

#undef log1p
#define log1p(__x) __tg_log1p(__tg_promote1((__x))(__x))

// log2

static float
    _TG_ATTRS
    __tg_log2(float __x) {return log2f(__x);}

static double
    _TG_ATTRS
    __tg_log2(double __x) {return log2(__x);}

static long double
    _TG_ATTRS
    __tg_log2(long double __x) {return log2l(__x);}

#undef log2
#define log2(__x) __tg_log2(__tg_promote1((__x))(__x))

// logb

static float
    _TG_ATTRS
    __tg_logb(float __x) {return logbf(__x);}

static double
    _TG_ATTRS
    __tg_logb(double __x) {return logb(__x);}

static long double
    _TG_ATTRS
    __tg_logb(long double __x) {return logbl(__x);}

#undef logb
#define logb(__x) __tg_logb(__tg_promote1((__x))(__x))

// lrint

static long
    _TG_ATTRS
    __tg_lrint(float __x) {return lrintf(__x);}

static long
    _TG_ATTRS
    __tg_lrint(double __x) {return lrint(__x);}

static long
    _TG_ATTRS
    __tg_lrint(long double __x) {return lrintl(__x);}

#undef lrint
#define lrint(__x) __tg_lrint(__tg_promote1((__x))(__x))

// lround

static long
    _TG_ATTRS
    __tg_lround(float __x) {return lroundf(__x);}

static long
    _TG_ATTRS
    __tg_lround(double __x) {return lround(__x);}

static long
    _TG_ATTRS
    __tg_lround(long double __x) {return lroundl(__x);}

#undef lround
#define lround(__x) __tg_lround(__tg_promote1((__x))(__x))

// nearbyint

static float
    _TG_ATTRS
    __tg_nearbyint(float __x) {return nearbyintf(__x);}

static double
    _TG_ATTRS
    __tg_nearbyint(double __x) {return nearbyint(__x);}

sta