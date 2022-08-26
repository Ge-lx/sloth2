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

template <typename SampleT>
std::chrono::time_point<clk> process_ring_buffer (RingBuffer<SampleT>* ringBuffer, VisualizationHandler** handlers, size_t num_handlers, SDL_AudioSpec& spec) {
    SampleT* const buf = ringBuffer->dequeue_dirty();
    auto received = clk::now();

    double* mono = new double[spec.samples];
    for (size_t i = 0; i < spec.samples; i++) {
        mono[i] = (buf[2*i] + buf[2*i + 1]) / ((double) std::pow(2, 17));
    }
    memset(buf, spec.silence, spec.channels * sizeof(SampleT) * spec.samples);
    ringBuffer->enqueue_clean(buf);

    for (size_t i = 0; i < num_handlers; i++) {
        handlers[i]->process_ring_buffer(mono);
    }

    for (size_t i = 0; i < num_handlers; i++) {
        handlers[i]->await_buffer_processed();
    }

    delete[] mono;
    return received;
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

// template <typename SampleT>
// int init_ui (VisualizationHandler** handlers, size_t num_handlers) {
    

    

//     while (1) {
        

        // if (time_diff_us(last_print, last_update) >= 1000 * 1000) {
        //     last_print = last_update;
        //     double acc_util = (acc_render + acc_getpoints[1] + acc_getpoints[2] + acc_idle);
        //     std::cout << "Stats:  "
        //         << std::setw(3) << frame_count << " frames | "
        //         << std::setw(9) << acc_render / frame_count << " us/f render | "
        //         << std::setw(9) << acc_getpoints[0] / frame_count << " us/f buffer wait | "
        //         << std::setw(9) << acc_getpoints[1] / frame_count << " us/f copy | "
        //         << std::setw(9) << acc_getpoints[2] / frame_count << " us/f visualization | "
        //         << std::setw(9) << acc_idle / frame_count << " us/f events | "
        //         << std::setw(7) << acc_util / (acc_getpoints[0] + acc_util) << " util\n";
        //     acc_render = 0;
        //     for (size_t i = 0; i < 3; i++) acc_getpoints[i] = 0;
        //     acc_idle = 0;
        //     frame_count = 0;
        // }
    // }

  
// }

int main (int argc, char** argv) {
    std::cout << "Starting sloth2 realtime audio visualizer..." << std::endl;

    using namespace audio;
    sdl_init();

    uint16_t device_id = 3;
    if (argc > 1) {
	   device_id = atoi(argv[1]);
    } else {
        std::cout << "\nNo audio device specified. Please choose one!" << std::endl;
        std::cout << "Usage: \"./sloth2 <device_id>\"\n" << std::endl;

        std::cout << "Available devices:" << std::endl;
        auto device_names = get_audio_device_names();
        for (size_t i = 0; i < device_names.size(); i++) {
            std::cout << "\t" << i << ": " << device_names[i] << std::endl;
        }

        return 1;
    }

    std::cout << std::endl << std::endl;

    SDL_AudioSpec spec;
    SDL_zero(spec);

    typedef int16_t SampleT;
    spec.format = AUDIO_S16SYS;
    spec.freq = 44100;//96000;
    spec.channels = 2;

    const static double update_interval_ms = 1000 / 59;
    const static double window_length_ms = 80;//update_interval_ms * 8;
    const static int num_ring_buffers = 2;

    size_t window_length_samples = ((double) window_length_ms) / 1000 * spec.freq;
    size_t update_fragment_samples = update_interval_ms / 1000.0 * spec.freq;
    spec.samples = (size_t) update_fragment_samples;
    printf("Allocating ring buffer of %f kB\n", 2 * update_fragment_samples * spec.channels * num_ring_buffers / 1000.0);
    RingBuffer<SampleT>* ringBuffer = new RingBuffer<SampleT>(spec.channels * spec.samples, num_ring_buffers);


    BPSW_Spec params {
        .win_length_samples = window_length_samples / 2,
        .win_window_fn = true,
        .fft_dispersion = 0,
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


    BPSW_Spec params2 {
        .win_length_samples = window_length_samples / 2,
        .win_window_fn = true,
        .fft_dispersion = 0,
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
        freq_weighing_2[i] = i > 80 ? 2 : 0;
    }
    params2.fft_freq_weighing = freq_weighing_2;


    // printf("update_fragment_samples: %ld\n", update_fragment_samples);
    // printf("window_length_samples: %ld\n", window_length_samples);
    printf("Starting audio stream on device %d\n", device_id);
    auto stop_audio_stream = start_audio_stream(ringBuffer, spec, device_id);


    printf("Instantiating visualizations\n");

    BandpassStandingWave bpsw {spec, params};
    BandpassStandingWave bpsw2 {spec, params2};
    constexpr size_t num_handlers = 2;
    VisualizationHandler* handlers[num_handlers] = {&bpsw, &bpsw2};

    printf("Starting UI\n");
    ui_init();

    auto last_frame = clk::now();
    double frame_us_nominal = (update_fragment_samples / (double) spec.freq) * 1000000;
    size_t frame_counter = 0;

    while (ui_should_quit() == false) {

        // SDL_ResizeEvent event = event;
        // double const resize_to[2] = event.type == SDL_VIDEORESIZE ? {event.w, event.h}
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);

        if (frame_counter++ % 60 == 0) {
            auto now = clk::now();
            double diff = time_diff_us(last_frame, now);
            std::cout << "Frame after " << std::setw(10) << diff << " us | "
                << std::setw(10) << diff / frame_us_nominal << " utilization\n";
        }

        last_frame = process_ring_buffer(ringBuffer, handlers, num_handlers, spec);

        for (size_t i = 0; i < num_handlers; i++) {
            handlers[i]->await_and_render(renderer);
        }

        SDL_RenderPresent(renderer);

    }

    printf("Stopping audio stream and deconstructing.\n");

    ui_shutdown();
    stop_audio_stream();

    delete ringBuffer;
}
