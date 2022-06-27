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
static SDL_Point* get_points(RingBuffer<SampleT>* rb, FFTHandler* fh, RollingWindow<SampleT>* rw, SDL_AudioSpec const& spec) {

    double* mono = new double[spec.samples]; // Yes, this leaks memory (but only once)
    const size_t window_length = rw->window_length();

    { // Fetch audio fragment from buffer and mean-downmix stereo to mono
        SampleT* const buf = rb->dequeue_dirty();
        for (size_t i = 0; i < spec.samples; i++) {
            mono[i] = ( buf[2*i] + buf[2*i + 1]) / ((double) std::pow(2, 17));
        }
        memset(buf, spec.silence, spec.channels * sizeof(SampleT) * spec.samples);
        rb->enqueue_clean(buf);
    }

    // Update rolling_window
    // SampleT* const window_data = rw->update(mono, spec.samples);
    // memcpy(fh->real, mono, spec.samples * sizeof(SampleT));

    // fh->real = window_data;
    // fh->exec_r2c();
    // for (size_t i = 0; i < 0; i++) {
    //     fh->complex[i][0] = 0;
    //     fh->complex[i][1] = 0;
    // }
    // fh->exec_c2r();
    // Why is the scaling is not preserved through irfft(rfft(x)) ?

    static math::ExpFilter<double> max_exp_filter{1, 0.1, 0.1, 0};
    // double max = math::max_value(fh->real, spec.samples);
    // max = *(max_exp_filter.update(&max));
    // double max = 150;
    // max = max < 0.2 ? 0.2 : (max > 8 ? 8 : max);
    // printf("Max: %f\n", max);

    SDL_Point* points = new SDL_Point[spec.samples];
    for (size_t i = 0; i < spec.samples; i++) {
        points[i].x = i;
        points[i].y = ((mono[i] + 0.5) * 720); // (int) (720 * (0.5 + val / ((double) std::pow(2, 16))));
    }

    delete[] mono;
    return points;
}

template <typename SampleT>
int init_ui (RingBuffer<SampleT>* rb, FFTHandler* fh, RollingWindow<SampleT>* rw, SDL_AudioSpec const& spec) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(1280, 720, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        const SDL_Point* points = get_points<SampleT>(rb, fh, rw, spec);
        SDL_RenderDrawLines(renderer, points, spec.samples);
        delete[] points;
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
    spec.samples = 735;

    RingBuffer<SampleT>* ringBuffer = new RingBuffer<SampleT>(spec.channels * spec.samples, 4);

    auto stop_audio_stream = start_audio_stream(ringBuffer, spec, 3);

    const static int cutoff_freq = 20;
    const static size_t window_length_samples = 4100; // (size_t) spec.freq / (0.5 * cutoff_freq);
    printf("window_length_samples: %d\n", window_length_samples);

    RollingWindow<SampleT>* rollingWindow = new RollingWindow<SampleT>(window_length_samples, 0);
    FFTHandler* fftHandler = new FFTHandler(spec.samples);

    init_ui<SampleT>(ringBuffer, fftHandler, rollingWindow, spec);

    stop_audio_stream();
    delete ringBuffer;
    delete fftHandler;
    delete rollingWindow;
}