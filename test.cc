#include <iostream>
#include <chrono>
#include <thread>

#include "pulse_backend.h"


// FIXME: This should be handled better
volatile bool stop = false;

int main () {

	
	audio::PulseBackend backend{};
    backend.start();

    // double const sleep_duration = 3 * 1000.0;
    double const sleep_duration = 1000.0 / 60.0;
    while (!stop) {
    	std::this_thread::sleep_for(std::chrono::milliseconds((int) sleep_duration));
    	// pa_threaded_mainloop_lock(m);
    	// backend->query_server_info();
    	// pa_threaded_mainloop_unlock(m);
    }

    backend.stop();

	return 0;
}