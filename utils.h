#include <iostream>

#include "pulse/pulseaudio.h"

std::ostream& operator<< (std::ostream& os, pa_context_state_t const& state);
std::ostream& operator<< (std::ostream& os, pa_sample_format const& format);
std::ostream& operator<< (std::ostream& os, pa_encoding const& encoding);
std::ostream& operator<< (std::ostream& os, pa_sample_spec const& spec);
std::ostream& operator<< (std::ostream& os, pa_proplist const& plist);
std::ostream& operator<< (std::ostream& os, pa_source_info const& spec);
std::ostream& operator<< (std::ostream& os, pa_sink_info const& info);
std::ostream& operator<< (std::ostream& os, pa_sink_input_info const& info);
std::ostream& operator<< (std::ostream& os, pa_server_info const& info);
std::ostream& operator<< (std::ostream& os, pa_format_info const& info);