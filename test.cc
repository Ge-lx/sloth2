#include <iostream>
#include <map>
#include <chrono>
#include <thread>

#include "pulse/pulseaudio.h"
#include "utils.h"


// FIXME: This should be handled better
volatile bool stop = false;

char const * const use_sink_name = NULL; 

struct MyUserdata;

struct SourceInfoList {
	typedef void (*server_info_source_list_cb_t)(MyUserdata*, SourceInfoList*);
	server_info_source_list_cb_t callback;
	MyUserdata* ud;

	std::map<char const*, pa_source_info> infos;
	pa_sink_info default_sink;
	bool eol_received = false;
	bool default_sink_received = false;
	bool data_valid = false;

	SourceInfoList(MyUserdata* ud);

	void process_trigger ();
	void reset ();
	void register_default_sink(pa_sink_info* i);
	void add_source(pa_source_info* i, bool eol = false);
	void query_all_source_info ();
	void query_sink_info_by_name (char const* name);
	void query_server_info ();
	void connect_record_stream_to_source_by_name ();
};

struct MyUserdata {
	pa_threaded_mainloop* m;
	pa_context* c;
	SourceInfoList* sl;
};

void source_info_list_cb (pa_context* c, pa_source_info* i, int eol, void* userdata) {
	MyUserdata* ud = (MyUserdata*) userdata;
	ud->sl->add_source(i, (bool) eol);
}

void sink_info_list_cb (pa_context* c, pa_sink_info* i, int eol, void* userdata) {
	MyUserdata* ud = (MyUserdata*) userdata;
	if (!eol) {
		ud->sl->register_default_sink(i);
	}
}

void server_info_cb (pa_context* c, pa_server_info* const server_info, void* userdata) {
	MyUserdata* ud = (MyUserdata*) userdata;
	ud->sl->query_sink_info_by_name(server_info->default_sink_name);
	ud->sl->query_all_source_info();
}

SourceInfoList::SourceInfoList(MyUserdata* ud) : ud(ud) {}

void SourceInfoList::process_trigger () {
	if (eol_received && default_sink_received) {
		data_valid = true;
		callback(ud, this);
	}
}

void SourceInfoList::reset () {
	eol_received = false;
	default_sink_received = false;
	data_valid = false;
	infos.clear();
}

void SourceInfoList::register_default_sink(pa_sink_info* i) {
	default_sink = *i;
	default_sink_received = true;
	process_trigger();
	std::cout << "Default sink added: " << default_sink.name << std::endl;
}

void SourceInfoList::add_source(pa_source_info* i, bool eol) {
	if (eol) {
		eol_received = true;
		process_trigger();
	} else {
		infos[i->name] = *i;
		std::cout << "Source added: " << i->name << std::endl;
	}
}

void SourceInfoList::query_all_source_info () {
	pa_operation* o = pa_context_get_source_info_list(ud->c,
		pa_source_info_cb_t(source_info_list_cb), ud);
	pa_operation_unref(o);
}

void SourceInfoList::query_sink_info_by_name (char const* name) {
	pa_operation* o = pa_context_get_sink_info_by_name(ud->c,
		name, pa_sink_info_cb_t(sink_info_list_cb), ud);
	pa_operation_unref(o);
}

void SourceInfoList::query_server_info () {
	reset();
	pa_operation* o = pa_context_get_server_info(ud->c,
		pa_server_info_cb_t(server_info_cb), ud);
	pa_operation_unref(o);
}

void SourceInfoList::connect_record_stream_to_source_by_name () {
	pa_operation* o;
	// samplespec = pa_sample_spec{
	// 	.rate = 
	// }
}

void context_state_cb (pa_context* c, void* userdata) {
	MyUserdata* ud = (MyUserdata*) userdata;
	pa_context_state_t state = pa_context_get_state(ud->c);

	std::cout << "context_state changed: " << state << std::endl;

	if (state == PA_CONTEXT_READY) {
		ud->sl->query_server_info();
	}
}

void server_info_source_list_cb (MyUserdata* ud, SourceInfoList* sl) {
	std::cout << "Updated server_sink_info" << std::endl;
}

int main () {

	pa_threaded_mainloop* m = pa_threaded_mainloop_new();
	pa_mainloop_api* mainloop_api = pa_threaded_mainloop_get_api(m);
	pa_context* c = pa_context_new(mainloop_api, "sloth2");

	MyUserdata* ud = new MyUserdata{ .m = m, .c = c };
	SourceInfoList* sl = new SourceInfoList(ud);
	sl->callback = &server_info_source_list_cb;
	ud->sl = sl;

	// Set context state callback
	pa_context_set_state_callback(c, context_state_cb, ud);
    // Connect to default server
    pa_context_connect(c, NULL, PA_CONTEXT_NOFLAGS, NULL);

    pa_threaded_mainloop_start(m);
    // double const sleep_duration = 3 * 1000.0;
    double const sleep_duration = 1000.0 / 60.0;
    while (!stop) {
    	std::this_thread::sleep_for(std::chrono::milliseconds((int) sleep_duration));
    	// pa_threaded_mainloop_lock(m);
    	// sl->query_server_info();
    	// pa_threaded_mainloop_unlock(m);
    }
    pa_threaded_mainloop_stop(m);
    pa_threaded_mainloop_free(m);

	return 0;
}