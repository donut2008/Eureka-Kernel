atic long double
    _TG_ATTRS
    __tg_asin(long double __x) {return asinl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_asin(float _Complex __x) {return casinf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_asin(double _Complex __x) {return casin(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_asin(long double _Complex __x) {return casinl(__x);}

#undef asin
#define asin(__x) __tg_asin(__tg_promote1((__x))(__x))

// atan

static float
    _TG_ATTRS
    __tg_atan(float __x) {return atanf(__x);}

static double
    _TG_ATTRS
    __tg_atan(double __x) {return atan(__x);}

static long double
    _TG_ATTRS
    __tg_atan(long double __x) {return atanl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_atan(float _Complex __x) {return catanf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_atan(double _Complex __x) {return catan(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_atan(long double _Complex __x) {return catanl(__x);}

#undef atan
#define atan(__x) __tg_atan(__tg_promote1((__x))(__x))

// acosh

static float
    _TG_ATTRS
    __tg_acosh(float __x) {return acoshf(__x);}

static double
    _TG_ATTRS
    __tg_acosh(double __x) {return acosh(__x);}

static long double
    _TG_ATTRS
    __tg_acosh(long double __x) {return acoshl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_acosh(float _Complex __x) {return cacoshf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_acosh(double _Complex __x) {return cacosh(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_acosh(long double _Complex __x) {return cacoshl(__x);}

#undef acosh
#define acosh(__x) __tg_acosh(__tg_promote1((__x))(__x))

// asinh

static float
    _TG_ATTRS
    __tg_asinh(float __x) {return asinhf(__x);}

static double
    _TG_ATTRS
    __tg_asinh(double __x) {return asinh(__x);}

static long double
    _TG_ATTRS
    __tg_asinh(long double __x) {return asinhl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_asinh(float _Complex __x) {return casinhf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_asinh(double _Complex __x) {return casinh(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_asinh(long double _Complex __x) {return casinhl(__x);}

#undef asinh
#define asinh(__x) __tg_asinh(__tg_promote1((__x))(__x))

// atanh

static float
    _TG_ATTRS
    __tg_atanh(float __x) {return atanhf(__x);}

static double
    _TG_ATTRS
    __tg_atanh(double __x) {return atanh(__x);}

static long double
    _TG_ATTRS
    __tg_atanh(long double __x) {return atanhl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_atanh(float _Complex __x) {return catanhf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_atanh(double _Complex __x) {return catanh(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_atanh(long double _Complex __x) {return catanhl(__x);}

#undef atanh
#define atanh(__x) __tg_atanh(__tg_promote1((__x))(__x))

// cos

static float
    _TG_ATTRS
    __tg_cos(float __x) {return cosf(__x);}

static double
    _TG_ATTRS
    __tg_cos(double __x) {return cos(__x);}

static long double
    _TG_ATTRS
    __tg_cos(long double __x) {return cosl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_cos(float _Complex __x) {return ccosf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_cos(double _Complex __x) {return ccos(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_cos(long double _Complex __x) {return ccosl(__x);}

#undef cos
#define cos(__x) __tg_cos(__tg_promote1((__x))(__x))

// sin

static float
    _TG_ATTRS
    __tg_sin(float __x) {return sinf(__x);}

static double
    _TG_ATTRS
    __tg_sin(double __x) {return sin(__x);}

static long double
    _TG_ATTRS
    __tg_sin(long double __x) {return sinl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_sin(float _Complex __x) {return csinf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_sin(double _Complex __x) {return csin(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_sin(long double _Complex __x) {return csinl(__x);}

#undef sin
#define sin(__x) __tg_sin(__tg_promote1((__x))(__x))

// tan

static float
    _TG_ATTRS
    __tg_tan(float __x) {return tanf(__x);}

static double
    _TG_ATTRS
    __tg_tan(double __x) {return tan(__x);}

static long double
    _TG_ATTRS
    __tg_tan(long double __x) {return tanl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_tan(float _Complex __x) {return ctanf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_tan(double _Complex __x) {return ctan(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_tan(long double _Complex __x) {return ctanl(__x);}

#undef tan
#define tan(__x) __tg_tan(__tg_promote1((__x))(__x))

// cosh

static float
    _TG_ATTRS
    __tg_cosh(float __x) {return coshf(__x);}

static double
    _TG_ATTRS
    __tg_cosh(double __x) {return cosh(__x);}

static long double
    _TG_ATTRS
    __tg_cosh(long double __x) {return coshl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_cosh(float _Complex __x) {return ccoshf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_cosh(double _Complex __x) {return ccosh(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_cosh(long double _Complex __x) {return ccoshl(__x);}

#undef cosh
#define cosh(__x) __tg_cosh(__tg_promote1((__x))(__x))

// sinh

static float
    _TG_ATTRS
    __tg_sinh(float __x) {return sinhf(__x);}

static double
    _TG_ATTRS
    __tg_sinh(double __x) {return sinh(__x);}

static long double
    _TG_ATTRS
    __tg_sinh(long double __x) {return sinhl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_sinh(float _Complex __x) {return csinhf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_sinh(double _Complex __x) {return csinh(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_sinh(long double _Complex __x) {return csinhl(__x);}

#undef sinh
#define sinh(__x) __tg_sinh(__tg_promote1((__x))(__x))

// tanh

static float
    _TG_ATTRS
    __tg_tanh(float __x) {return tanhf(__x);}

static double
    _TG_ATTRS
    __tg_tanh(double __x) {return tanh(__x);}

static long double
    _TG_ATTRS
    __tg_tanh(long double __x) {return tanhl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_tanh(float _Complex __x) {return ctanhf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_tanh(double _Complex __x) {return ctanh(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_tanh(long double _Complex __x) {return ctanhl(__x);}

#undef tanh
#define tanh(__x) __tg_tanh(__tg_promote1((__x))(__x))

// exp

static float
    _TG_ATTRS
    __tg_exp(float __x) {return expf(__x);}

static double
    _TG_ATTRS
    __tg_exp(double __x) {return exp(__x);}

static long double
    _TG_ATTRS
    __tg_exp(long double __x) {return expl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_exp(float _Complex __x) {return cexpf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_exp(double _Complex __x) {return cexp(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_exp(long double _Complex __x) {return cexpl(__x);}

#undef exp
#define exp(__x) __tg_exp(__tg_promote1((__x))(__x))

// log

static float
    _TG_ATTRS
    __tg_log(float __x) {return logf(__x);}

static double
    _TG_ATTRS
    __tg_log(double __x) {return log(__x);}

static long double
    _TG_ATTRS
    __tg_log(long double __x) {return logl(__x);}

static float _Complex
    _TG_ATTRS
    __tg_log(float _Complex __x) {return clogf(__x);}

static double _Complex
    _TG_ATTRS
    __tg_log(double _Complex __x) {return clog(__x);}

static long double _Complex
    _TG_ATTRS
    __tg_log(long double _Complex __x) {return clogl(__x);}

#undef log
#define log(__x) __tg_log(__tg_promote1((__x))(__x))

// pow

static float
    _TG_ATTRS
    __tg_pow(float __x, float __y) {return powf(__x, __y);}

static double
    _TG_ATTRS
    __tg_pow(double __x, double __y) {return pow(__x, __y);}

static long double
    _TG_ATTR