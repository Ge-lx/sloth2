#include <cstring>
#include <cmath>
#include <complex.h>
#include <iostream>
#include <iomanip>
#include <chrono>

#include "util/fft_handler.h"
#include "util/ring_buffer.tcc"
#include "util/math.tcc"
#include "util/rolling_window.tcc"

#include "visualization/bandpass_standing_wave.tcc"

#include <SDL2/SDL.h>
#include "sdl/sdl_audio.tcc"
#define WINDOW_HEIGHT 1000
#define WINDOW_WIDTH 1800

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Event event;

typedef std::chrono::high_resolution_clock clk;
double time_diff_us (std::chrono::time_point<clk> const& a, std::chrono::time_point<clk> const& b) {
    const std::chrono::duration<double, std::micro> diff = b - a;
    return diff.count();
}

int ui_init () {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    return 0;
}

bool ui_should_quit () {
    SDL_PollEvent(&event);
    return event.type == SDL_QUIT;
}

void ui_shutdown () {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

template <typename SampleT>
void sloth_mainloop (uint16_t device_id, SDL_AudioSpec& spec, size_t num_buffers_delay,
    VisualizationHandler** handlers, size_t num_handlers, double print_interval_ms) {

    using namespace audio;

    printf("Allocating ring buffer of %.3f kB\n", 2 * spec.samples * spec.channels * num_buffers_delay / 1000.0);
    printf("Audio input delay of %.1f ms\n", num_buffers_delay * spec.samples / (double) spec.freq * 1000.0);
    RingBuffer<SampleT>* ringBuffer = new RingBuffer<SampleT>(spec.channels * spec.samples, num_buffers_delay);
    double* mono = new double[spec.samples];
    math::ExpFilter<double> max_filter(1, 0.90, 0.04, 1);

    auto device_names = get_audio_device_names();
    std::cout << "Starting audio stream on \"" << device_names[device_id] << "\"" << std::endl;
    auto stop_audio_stream = start_audio_stream(ringBuffer, spec, device_id);

    printf("Starting UI\n\n");
    ui_init();

    auto last_frame = clk::now();
    auto last_print = clk::now();
    double frame_us_nominal = (spec.samples / (double) spec.freq) * 1000000;
    double frame_us_acc = 0;
    size_t frame_counter = 0;

    while (ui_should_quit() == false) {

        // SDL_ResizeEvent event = event;
        // double const resize_to[2] = event.type == SDL_VIDEORESIZE ? {event.w, event.h}
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 225);

        auto now = clk::now();
        frame_counter++;
        frame_us_acc += time_diff_us(last_frame, now);
        if (time_diff_us(last_print, now) / 1000 >= print_interval_ms) {
            double frame_avg_us = frame_us_acc / frame_counter;
            frame_counter = 0;
            frame_us_acc = 0;
            last_print = now;
            std::cout << "Frame after " << std::setw(10) << frame_avg_us << " us | "
                << std::setw(8) << std::fixed << std::setprecision(2)
                << frame_avg_us / frame_us_nominal * 100 << "% utilization\n";
        }

        SampleT* const buf = ringBuffer->dequeue_dirty();
        last_frame = clk::now();

        for (size_t i = 0; i < spec.samples; i++) {
            mono[i] = (buf[2*i] + buf[2*i + 1]) / ((double) std::pow(2, 17));
        }

        memset(buf, spec.silence, spec.channels * sizeof(SampleT) * spec.samples);
        ringBuffer->enqueue_clean(buf);

        double maxval = math::max_value(mono, spec.samples);
        maxval = *max_filter.update(&maxval);
        maxval = maxval > 2 ? 2 : (maxval < 0.01 ? 0.02 : maxval);
        for (size_t i = 0; i < spec.samples; i++) {
            mono[i] /= 2.5 * maxval;
        }

        for (size_t i = 0; i < num_handlers; i++) {
            handlers[i]->process_ring_buffer(mono);
        }

        for (size_t i = 0; i < num_handlers; i++) {
            handlers[i]->await_and_render(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    printf("\n\nStopping audio stream and deconstructing.\n");

    ui_shutdown();
    stop_audio_stream();

    delete ringBuffer;
    delete[] mono;
}

int main (int argc, char** argv) {
    std::cout << "Starting sloth2 realtime audio visualizer..." << std::endl;

    using namespace audio;
    sdl_init();

    uint16_t device_id = 0;
    if (argc > 1) {
	   device_id = atoi(argv[1]);
    } else {
        auto device_names = get_audio_device_names();
        std::cout << "\nNo audio device specified. Please choose one!" << std::endl;
        std::cout << "Usage: \"./sloth2 <device_id>\"\n" << std::endl;
        std::cout << "Available devices:" << std::endl;
        for (size_t i = 0; i < device_names.size(); i++) {
            std::cout << "\t" << i << ": " << device_names[i] << std::endl;
        }

        return 1;
    }

    std::cout << std::endl << std::endl;

    /* -------------------- CONFIGURATION --------------------------------- */

    SDL_AudioSpec spec;
    SDL_zero(spec);

    typedef int16_t SampleT;
    spec.format = AUDIO_S16SYS;
    spec.freq = 96000;
    spec.channels = 2;

    const static double update_interval_ms = 1000 / 59;
    const static double window_length_ms = 80;
    const static double print_interval_ms = 2000;
    const static int num_buffers_delay = 3;

    size_t window_length_samples = window_length_ms / 1000 * spec.freq;
    spec.samples = (size_t) update_interval_ms / 1000.0 * spec.freq;

    BPSW_Spec params {
        .win_length_samples = window_length_samples / 2,
        .win_window_fn = true,
        .fft_dispersion = 2 * 1.41421356237,
        .fft_phase = BPSW_Phase::Constant,
        .fft_phase_const = 1.22222,
        .crop_length_samples = window_length_samples / 2,
        .crop_offset = 0,
        .c_center_x = WINDOW_WIDTH / 2,
        .c_center_y = WINDOW_HEIGHT / 2,
        .c_rad_base = WINDOW_HEIGHT / 2 - 220,
        .c_rad_extr = 400,
    };
    size_t c_length = params.win_length_samples / 2 + 1;
    double* freq_weighing = new double[c_length];
    for (size_t i = 0; i < c_length; i++) {
        freq_weighing[i] = i < 10 ? 1.5 :
                           i < 20 ? 1 : 0.2;
                           // i < 80 ? 0.2 : 0;
    }
    params.fft_freq_weighing = freq_weighing;


    BPSW_Spec params_i {
        .win_length_samples = window_length_samples / 2,
        .win_window_fn = true,
        .fft_dispersion = 0,//2 * 1.41421356237,
        .fft_phase = BPSW_Phase::Standing,
        .fft_phase_const = 0,
        .crop_length_samples = window_length_samples / 2,
        .crop_offset = 0,
        .c_center_x = WINDOW_WIDTH / 2,
        .c_center_y = WINDOW_HEIGHT / 2,
        .c_rad_base = WINDOW_HEIGHT / 2 - 400,
        .c_rad_extr = 100,
    };
    size_t c_length_i = params_i.win_length_samples / 2 + 1;
    double* freq_weighing_i = new double[c_length_i];
    for (size_t i = 0; i < c_length_i; i++) {
        freq_weighing_i[i] = i < 10 ? 1.5 :
                           i < 20 ? 1 : 0.2;
                           // i < 80 ? 0.2 : 0;
    }
    params_i.fft_freq_weighing = freq_weighing_i;


    BPSW_Spec params2 {
        .win_length_samples = window_length_samples / 2,
        .win_window_fn = true,
        .fft_dispersion = 0,
        .fft_phase = BPSW_Phase::Standing,
        .fft_phase_const = 0,
        .crop_length_samples = window_length_samples / 2,
        .crop_offset = 0,
        .c_center_x = WINDOW_WIDTH / 2,
        .c_center_y = WINDOW_HEIGHT / 2,
        .c_rad_base = WINDOW_HEIGHT / 2 - 50,
        .c_rad_extr = 300,
    };
    size_t c_length_2 = params2.win_length_samples / 2 + 1;
    double* freq_weighing_2 = new double[c_length_2];
    for (size_t i = 0; i < c_length_2; i++) {
        freq_weighing_2[i] = i > 110 ? 3 :
                             i > 50 ? 1.5 : 0;
    }
    params2.fft_freq_weighing = freq_weighing_2;

    /* -------------------- CONFIGURATION END ----------------------------- */

    printf("Instantiating visualizations\n");
    BandpassStandingWave bpsw {spec, params};
    BandpassStandingWave bpsw_i {spec, params_i};
    BandpassStandingWave bpsw2 {spec, params2};

    constexpr size_t num_handlers = 3;
    VisualizationHandler* handlers[num_handlers] = {&bpsw, &bpsw_i, &bpsw2};

    sloth_mainloop<SampleT>(device_id, spec, num_buffers_delay, handlers, num_handlers, print_interval_ms);
}
