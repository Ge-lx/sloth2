#include <iostream>

#include "pulse/pulseaudio.h"

std::ostream& operator<< (std::ostream& os, pa_context_state_t const state);