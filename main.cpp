#include <cstring>
#include <cmath>
#include <complex.h>

#include <SDL2/SDL.h>

#include "util/fft_handler.h"
#include "util/ring_buffer.tcc"
#include "util/math.tcc"
#include "util/rolling_window.tcc"
#include "sdl/sdl_audio.tcc"

template <typename SampleT>
static void get_points(RingBuffer<SampleT>* rb, FFTHandler* fh, RollingWindow<double>* rw, SDL_AudioSpec const& spec, SDL_Point* points) {

    double* mono = new double[spec.samples];
    const size_t window_length = rw->window_length();

    { // Fetch audio fragment from buffer and mean-downmix stereo to mono
        SampleT* const buf = rb->dequeue_dirty();
        for (size_t i = 0; i < spec.samples; i++) {
            mono[i] = (buf[2*i] + buf[2*i + 1]) / ((double) std::pow(2, 17));
        }
        memset(buf, spec.silence, spec.channels * sizeof(SampleT) * spec.samples);
        rb->enqueue_clean(buf);
    }

    // Update rolling_window
    double* const window_data = rw->update(mono, spec.samples);
    delete[] mono;
    memcpy(fh->real, window_data, window_length * sizeof(double));

    fh->exec_r2c();
    const size_t c_length = window_length / 2 + 1;
    const size_t upper_cutoff = c_length - 0;
    for (size_t i = 0; i < upper_cutoff; i++) {
        std::complex<double> c(fh->complex[i][0], fh->complex[i][1]);
        double abs = std::abs(c);
        double arg = std::arg(c);
        // if (i == 0) 
        //     printf("fft[0][abs] = %f\n", abs);
        // arg = M_PI * (1 + i / (double) c_length);
        arg = (3 * M_PI) / 2;
        // abs = 1;
        c = std::polar(abs, arg);
        fh->complex[i][0] = std::real(c);
        fh->complex[i][1] = std::imag(c);
    }
    for (size_t i = 0; i < 3; i++) {
        fh->complex[i][0] = 0;
        fh->complex[i][1] = 0;
    }
    fh->exec_c2r();
    size_t len_tot = window_length * sizeof(double);
    double* tmp = new double[window_length / 2];
    memcpy(tmp, fh->real + window_length / 2, len_tot / 2);
    memmove(fh->real + window_length / 2, fh->real, len_tot / 2);
    memcpy(fh->real, tmp, len_tot / 2);
    delete[] tmp;
    // Why is the scaling is not preserved through irfft(rfft(x)) ?

    static math::ExpFilter<double> max_exp_filter{1, 0.1, 0.1, 0};
    // double max = math::max_value(fh->real, window_length);
    // max = *(max_exp_filter.update(&max));
    // double max = 150;
    // max = max < 0.2 ? 0.2 : (max > 8 ? 8 : max);
    // printf("Max: %f\n", max);

    // printf("Window length: %ld\n", window_length);
    for (size_t i = 0; i < window_length; i++) {
        points[i].x = i;
        points[i].y = ((fh->real[i] / window_length + 0.5) * 720); // (int) (720 * (0.5 + val / ((double) std::pow(2, 16))));
    }
}

template <typename SampleT>
int init_ui (RingBuffer<SampleT>* rb, FFTHandler* fh, RollingWindow<double>* rw, SDL_AudioSpec const& spec) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;

    const size_t window_length = rw->window_length();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(window_length, 720, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    SDL_Point* points = new SDL_Point[window_length];

    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        get_points<SampleT>(rb, fh, rw, spec, points);
        SDL_RenderDrawLines(renderer, points, window_length);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

int main() {
    using namespace audio;

    sdl_init();
    auto device_names = get_audio_device_names();

    for (unsigned int i = 0; i < device_names.size(); i++) {
        SDL_Log("Audio device %d: %s", i, device_names[i].c_str());
    }

    typedef int16_t SampleT;
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    const static int window_length_ms = 40;

    size_t window_length_samples = ((double) window_length_ms) / 1000 * spec.freq;
    size_t window_length_adj = window_length_samples; // std::pow(2, std::ceil(std::log2(window_length_samples)));
    // const static size_t window_length_samples = (size_t) spec.freq / (0.5 * cutoff_freq);
    printf("window_length_samples: %ld\n", window_length_samples);
    printf("window_length_adj: %ld\n", window_length_adj);
    spec.samples = ((size_t) window_length_adj / 8);

    RingBuffer<SampleT>* ringBuffer = new RingBuffer<SampleT>(spec.channels * spec.samples, 4);

    auto stop_audio_stream = start_audio_stream(ringBuffer, spec, 3);


    RollingWindow<double>* rollingWindow = new RollingWindow<double>(window_length_adj, 0);
    FFTHandler* fftHandler = new FFTHandler(window_length_adj);

    init_ui<SampleT>(ringBuffer, fftHandler, rollingWindow, spec);

    stop_audio_stream();
    delete ringBuffer;
    delete fftHandler;
    delete rollingWindow;
}