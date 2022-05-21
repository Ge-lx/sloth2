#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "pulse/pulseaudio.h"

// FIXME: This should be handled better
volatile bool stop = false;

struct MyUserdata {
	pa_threaded_mainloop* m;
	pa_context* c;
} ud;

struct SourceInfoList {
	std::vector<pa_source_info> infos;
	pa_sink_info default_sink;
	bool eol_received = false;

	void register_default_sink(pa_sink_info* i) {
		default_sink = *i;
		std::cout << "Default sink added: " << default_sink.name << std::endl;
	}

	void add_source(pa_source_info* i, bool eol = false) {
		if (eol) {
			eol_received = true;
		} else {
			infos.push_back(*i);
			std::cout << "Source added: " << i->name << std::endl;
		}
	}
} source_info_list;

void source_info_list_cb (pa_context* c, pa_source_info* i, int eol, void* userdata) {
	source_info_list.add_source(i, (bool) eol);
}

void query_all_source_info (MyUserdata* ud) {
	pa_operation* o = pa_context_get_source_info_list(ud->c,
		pa_source_info_cb_t(source_info_list_cb), ud);
	pa_operation_unref(o);
}

void sink_info_list_cb (pa_context* c, pa_sink_info* i, int eol, void* userdata) {
	if (!eol) {
		source_info_list.register_default_sink(i);
	}
}

void query_sink_info_by_name (MyUserdata* ud, char const* name) {
	pa_operation* o = pa_context_get_sink_info_by_name(ud->c,
		name, pa_sink_info_cb_t(sink_info_list_cb), ud);
	pa_operation_unref(o);
}

void server_info_cb (pa_context* c, pa_server_info* const server_info, void* userdata) {
	MyUserdata* ud = (MyUserdata*) userdata;
	query_sink_info_by_name(ud, server_info->default_sink_name);
	query_all_source_info(ud);
}

void query_server_info (MyUserdata* ud) {
	pa_operation* o = pa_context_get_server_info(ud->c,
		pa_server_info_cb_t(server_info_cb), ud);
	pa_operation_unref(o);
}

void connect_record_stream_to_source_by_name (MyUserdata* ud, char* const name) {
	pa_operation* o;
	// samplespec = pa_sample_spec{
	// 	.rate = 
	// }
}

std::ostream& operator<< (std::ostream& os, pa_context_state_t const state) {
	switch (state) {
		case PA_CONTEXT_UNCONNECTED:
			return os << "PA_CONTEXT_UNCONNECTED";
		case PA_CONTEXT_CONNECTING:
			return os << "PA_CONTEXT_CONNECTING";
		case PA_CONTEXT_AUTHORIZING:
			return os << "PA_CONTEXT_AUTHORIZING";
		case PA_CONTEXT_SETTING_NAME:
			return os << "PA_CONTEXT_SETTING_NAME";
		case PA_CONTEXT_READY:
			return os << "PA_CONTEXT_READY";
		case PA_CONTEXT_FAILED:
			return os << "PA_CONTEXT_FAILED";
		case PA_CONTEXT_TERMINATED:
			return os << "PA_CONTEXT_TERMINATED";
	}
}

void context_state_cb (pa_context* c, void* userdata) {
	MyUserdata* ud = (MyUserdata*) userdata;
	pa_context_state_t state = pa_context_get_state(ud->c);

	std::cout << "context_state changed: " << state << std::endl;

	if (state == PA_CONTEXT_READY) {
		query_server_info(ud);
	}
}

int main () {

	ud.m = pa_threaded_mainloop_new();
	pa_mainloop_api* const mainloop_api = pa_threaded_mainloop_get_api(ud.m);
	ud.c = pa_context_new(mainloop_api, "Pulseaudio test");

	// Set context state callback
	pa_context_set_state_callback(ud.c, context_state_cb, &ud);
    // Connect to default server
    pa_context_connect(ud.c, NULL, PA_CONTEXT_NOFLAGS, NULL);

    pa_threaded_mainloop_start(ud.m);
    double const sleep_duration = 1000.0 / 60.0;
    while (!stop) {
    	std::this_thread::sleep_for(std::chrono::milliseconds((int) sleep_duration));
    }
    pa_threaded_mainloop_stop(ud.m);
    pa_threaded_mainloop_free(ud.m);

	return 0;
}