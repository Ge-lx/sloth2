#include "fft_handler.h"

FFTHandler::FFTHandler (size_t n) : n(n) {
    real = fftw_alloc_real(n);
    complex = fftw_alloc_complex(n/2 + 1);
    plan_r2c = fftw_plan_dft_r2c_1d(n, real, complex, 0);
    plan_c2r = fftw_plan_dft_c2r_1d(n, complex, real, 0);
}

FFTHandler::~FFTHandler () {
    fftw_destroy_plan(plan_r2c);
    fftw_destroy_plan(plan_c2r);
    fftw_free(real);
    fftw_free(complex);
}

void FFTHandler::exec_r2c () {
    fftw_execute(plan_r2c);
}

void FFTHandler::exec_c2r () {
    fftw_execute(plan_c2r);
}
