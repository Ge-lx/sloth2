#include <cstring>
#include <cmath>
#include <complex.h>

#include <SDL2/SDL.h>

#include "util/fft_handler.h"
#include "util/ring_buffer.tcc"
#include "sdl/sdl_audio.tcc"

SDL_Point* get_points(RingBuffer<int16_t>* rb, FFTHandler* fb, SDL_AudioSpec const& spec) {
    int16_t* buf = rb->dequeue_dirty();
    for (size_t i = 0; i < spec.samples; i++) {
        fb->real[i] = (buf[2*i] + buf[2*i + 1]) / ((double) std::pow(2, 17));
    }

    fb->exec_r2c();
    // for (size_t i = 0; i < 0; i++) {
    //     fb->complex[i][0] = 0;
    //     fb->complex[i][1] = 0;
    // }
    fb->exec_c2r();
    // Why is the scaling is not preserved through irfft(rfft(x)) ?

    SDL_Point* points = new SDL_Point[spec.samples];
    for (size_t i = 0; i < spec.samples; i++) {
        points[i].x = i;
        points[i].y = fb->real[i] + 200; // (int) (720 * (0.5 + val / ((double) std::pow(2, 16))));
    }

    memset(buf, spec.silence, spec.channels * sizeof(int16_t) * spec.samples);
    rb->enqueue_clean(buf);
    return points;
}

int init_ui (RingBuffer<int16_t>* rb, FFTHandler* fh, SDL_AudioSpec const& spec) {
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
        const SDL_Point* points = get_points(rb, fh, spec);
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

    typedef int16_t sample_t;
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = 735;

    RingBuffer<sample_t>* ringBuffer = new RingBuffer<sample_t>(spec.channels * spec.samples, 4);

    auto stop_audio_stream = start_audio_stream(ringBuffer, spec, device_names.size() - 1);

    FFTHandler* fftHandler = new FFTHandler(spec.samples);
    init_ui(ringBuffer, fftHandler, spec);

    stop_audio_stream();
    delete ringBuffer;
    delete fftHandler;
}