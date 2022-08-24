#include <cstring>
#include <cmath>
#include <complex.h>

#include <SDL2/SDL.h>

#include "util/fft_handler.h"
#include "util/ring_buffer.tcc"
#include "util/math.tcc"
#include "util/rolling_window.tcc"
#include "sdl/sdl_audio.tcc"

#define WINDOW_HEIGHT 1000
#define WINDOW_WIDTH 1800

template <typename SampleT>
static void get_points(RingBuffer<SampleT>* rb, FFTHandler* fh, RollingWindow<double>* rw, RollingWindow<double>* rw_d, SDL_AudioSpec const& spec, SDL_Point* points) {

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

    double* abs_vals = new double[c_length];
    double* arg_vals = new double[c_length];
    // double* freqs = new double[c_length];
    // math::lin_space<double>(freqs, c_length, 0, spec.freq, false, spec.samples / (double) c_length);

    for (size_t i = 0; i < c_length; i++) {
        std::complex<double> c(fh->complex[i][0], fh->complex[i][1]);
        abs_vals[i] = std::abs(c);
        arg_vals[i] = std::arg(c);
    }

    size_t max_freq_arg = math::max_value_arg(abs_vals, c_length);
    double phase_at_max_freq = arg_vals[max_freq_arg];

    for (size_t i = 0; i < c_length; i++) {
        double phase = rw->current_index() / ((double) window_length);
        double freq_phase = (i == 0 ? 0 : (1+i) * 2 * M_PI * phase);
        std::complex<double> c = std::polar(abs_vals[i], arg_vals[i] - freq_phase);
        fh->complex[i][0] = std::real(c);
        fh->complex[i][1] = std::imag(c);
    }

    delete[] abs_vals;
    delete[] arg_vals;
        // if (i == 0) 
        //     printf("fft[0][abs] = %f\n", abs);
        // arg = 2 * M_PI *  i / (double) c_length;
        // arg = 0;
        // abs = 1;
        
    // }
    // for (size_t i = 0; i < 3; i++) {
    //     fh->complex[i][0] = 0;
    //     fh->complex[i][1] = 0;
    // }
    fh->exec_c2r();
    size_t len_tot = window_length * sizeof(double);
    // double* tmp = new double[window_length / 2];
    // memcpy(tmp, fh->real + window_length / 2, len_tot / 2);
    // memmove(fh->real + window_length / 2, fh->real, len_tot / 2);
    // memcpy(fh->real, tmp, len_tot / 2);
    // delete[] tmp;

    // Scaling is not preserved: irfft(rfft(x))[i] = x[i] * len(x)
    for (size_t i = 0; i < window_length; i++) {
        fh->real[i] /= window_length;
    }

    // static math::ExpFilter<double> max_exp_filter{1, 0.1, 0.1, 0};
    // double max = math::min_value(fh->real, window_length);
    // max = *(max_exp_filter.update(&max));
    // max = max < 0.2 ? 0.2 : (max > 8 ? 8 : max);

    double const radius_base = WINDOW_HEIGHT / 2 - 200;
    double const extrusion_scale = 800;
    double* angles = new double[window_length];
    phase_at_max_freq = 2 * M_PI / 2;
    math::lin_space(angles, window_length, 0.0 + phase_at_max_freq, 2.0 * M_PI + phase_at_max_freq, true);


    // printf("Window length: %ld\n", window_length);
    double const center[2] = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    for (size_t i = 0; i < window_length; i++) {
        double const radius = radius_base + extrusion_scale * (fh->real[i]);
        points[i].x = center[0] + radius * std::cos(angles[i]);
        points[i].y = center[1] + radius * std::sin(angles[i]);
    }
}

template <typename SampleT>
int init_ui (RingBuffer<SampleT>* rb, FFTHandler* fh, RollingWindow<double>* rw, RollingWindow<double>* rw_d, SDL_AudioSpec const& spec) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;

    const size_t window_length = rw->window_length();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    SDL_Point* points = new SDL_Point[window_length];

    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        // SDL_ResizeEvent event = event;
        // double const resize_to[2] = event.type == SDL_VIDEORESIZE ? {event.w, event.h}
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        get_points<SampleT>(rb, fh, rw, rw_d, spec, points);
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

    uint16_t device_id = 3;

    typedef int16_t SampleT;
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    const static int update_interval_ms = 1000 / 60;
    const static int window_length_ms = 1000 / 1;

    size_t update_fragment_samples = ((double) update_interval_ms) / 1000 * spec.freq;
    spec.samples = (size_t) update_fragment_samples;

    size_t window_length_samples = ((double) window_length_ms) / 1000 * spec.freq;
    printf("update_fragment_samples: %ld\n", update_fragment_samples);
    printf("window_length_samples: %ld\n", window_length_samples);
    printf("Instantiating visualizations\n");

    RingBuffer<SampleT>* ringBuffer = new RingBuffer<SampleT>(spec.channels * spec.samples, 4);
    RollingWindow<double>* rollingWindow = new RollingWindow<double>(window_length_samples, 0, true);
    RollingWindow<double>* rwDisplay = new RollingWindow<double>(window_length_samples, 0, false);
    FFTHandler* fftHandler = new FFTHandler(window_length_samples);

    printf("Starting audio stream on device %d\n", device_id);
    auto stop_audio_stream = start_audio_stream(ringBuffer, spec, device_id);

    printf("Starting UI\n");
    init_ui<SampleT>(ringBuffer, fftHandler, rollingWindow, rwDisplay, spec);

    printf("Stopping audio stream and deconstructing.\n");
    stop_audio_stream();
    delete ringBuffer;
    delete fftHandler;
    delete rollingWindow;
}
