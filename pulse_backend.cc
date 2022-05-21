#include "pulse_backend.h"

namespace audio {

	namespace callbacks {
		void source_info_list_cb (pa_context*, pa_source_info* i, int eol, void* userdata) {
			UD* ud = (UD*) userdata;
			ud->backend->add_source(i, (bool) eol);
		}

		void sink_info_list_cb (pa_context*, pa_sink_info* i, int eol, void* userdata) {
			UD* ud = (UD*) userdata;
			ud->backend->add_sink(i, (bool) eol);
		}

		void sink_input_info_list_cb(pa_context*, pa_sink_input_info* i, int eol, void* userdata) {
			UD* ud = (UD*) userdata;
			ud->backend->add_sink_input(i, (bool) eol);
		}

		void server_info_cb (pa_context*, pa_server_info* const server_info, void* userdata) {
			UD* ud = (UD*) userdata;
			ud->backend->server_info = *server_info;
			ud->backend->query_all_source_info();
			ud->backend->query_all_sink_info();
			ud->backend->query_all_sink_input_info();
		}

		void context_state_cb (pa_context*, void* userdata) {
			UD* ud = (UD*) userdata;
			pa_context_state_t state = pa_context_get_state(ud->c);

			std::cout << "context_state changed: " << state << "\n\n";

			if (state == PA_CONTEXT_READY) {
				ud->backend->query_server_info();
			}
		}
	} // namespace callbacks

	PulseBackend::PulseBackend (char const* force_sink_name) : force_sink_name(force_sink_name) { }

	void PulseBackend::start () {
		pa_threaded_mainloop* m = pa_threaded_mainloop_new();
		pa_mainloop_api* mainloop_api = pa_threaded_mainloop_get_api(m);
		pa_context* c = pa_context_new(mainloop_api, "sloth2");

		ud = new UD{ .m = m, .c = c, .backend = this };

		// Set context state callback and connect to server
		pa_context_set_state_callback(c, callbacks::context_state_cb, ud);
	    pa_context_connect(c, NULL, PA_CONTEXT_NOFLAGS, NULL);
	    pa_threaded_mainloop_start(m);
	}

	void PulseBackend::stop () {
		reset();
		pa_threaded_mainloop_stop(ud->m);
		pa_threaded_mainloop_free(ud->m);

		pa_context_disconnect(ud->c);
		pa_context_unref(ud->c);

		delete ud;
	}

	void PulseBackend::on_partial_data () {
		if (eol_sinks_received && eol_sources_received && eol_sink_inputs_received) {
			data_valid = true;
			std::cout << "All data received!" << std::endl;
		}
	}

	void PulseBackend::reset () {
		eol_sinks_received = false;
		eol_sources_received = false;
		eol_sink_inputs_received = false;
		data_valid = false;
		source_infos.clear();
	}

	void PulseBackend::add_source (pa_source_info* i, bool eol) {
		if (eol) {
			eol_sources_received = true;
			on_partial_data();
		} else {
			source_infos[i->name] = *i;
			std::cout << "Source added: " << i->name << std::endl;
		}
	}

	void PulseBackend::add_sink (pa_sink_info* i, bool eol) {
		if (eol) {
			eol_sinks_received = true;
			on_partial_data();
		} else {
			sink_infos[i->name] = *i;
			std::cout << "Sink added: " << i->name << std::endl;
		}
	}

	void PulseBackend::add_sink_input (pa_sink_input_info* i, bool eol) {
		if (eol) {
			eol_sink_inputs_received = true;
			on_partial_data();
		} else {
			sink_input_infos[i->name] = *i;
			std::cout << "Sink input added: " << *i << std::endl;
		}
	}

	void PulseBackend::connect_record_stream_to_source_by_name () {
		pa_operation* o;
		// samplespec = pa_sample_spec{
		// 	.rate = 
		// }
	}

	void PulseBackend::query_all_source_info () {
		pa_operation* o = pa_context_get_source_info_list(ud->c,
			pa_source_info_cb_t(callbacks::source_info_list_cb), ud);
		pa_operation_unref(o);
	}

	void PulseBackend::query_all_sink_info () {
		pa_operation* o = pa_context_get_sink_info_list(ud->c,
			pa_sink_info_cb_t(callbacks::sink_info_list_cb), ud);
		pa_operation_unref(o);
	}

	void PulseBackend::query_all_sink_input_info () {
		pa_operation* o = pa_context_get_sink_input_info_list(ud->c,
			pa_sink_input_info_cb_t(callbacks::sink_input_info_list_cb), ud);
		pa_operation_unref(o);
	}

	void PulseBackend::query_server_info () {
		reset();
		pa_operation* o = pa_context_get_server_info(ud->c,
			pa_server_info_cb_t(callbacks::server_info_cb), ud);
		pa_operation_unref(o);
	}

} // namespace audio