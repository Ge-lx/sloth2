#include "pulse_backend.h"

namespace audio {

	namespace callbacks {
		void source_info_list_cb (pa_context*, pa_source_info* i, int eol, void* userdata) {
			UD* ud = (UD*) userdata;
			ud->backend->add_source(i, (bool) eol);
		}

		void sink_info_list_cb (pa_context*, pa_sink_info* i, int eol, void* userdata) {
			UD* ud = (UD*) userdata;
			if (!eol) {
				ud->backend->register_default_sink(i);
			}
		}

		void server_info_cb (pa_context*, pa_server_info* const server_info, void* userdata) {
			UD* ud = (UD*) userdata;
			ud->backend->query_sink_info_by_name(server_info->default_sink_name);
			ud->backend->query_all_source_info();
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
		if (eol_received && default_sink_received) {
			data_valid = true;
			std::cout << "All data received!" << std::endl;
		}
	}

	void PulseBackend::reset () {
		eol_received = false;
		default_sink_received = false;
		data_valid = false;
		infos.clear();
	}

	void PulseBackend::register_default_sink(pa_sink_info* i) {
		default_sink = *i;
		default_sink_received = true;
		on_partial_data();
		std::cout << "Default sink added: " << default_sink.name << std::endl;
	}

	void PulseBackend::add_source(pa_source_info* i, bool eol) {
		if (eol) {
			eol_received = true;
			on_partial_data();
		} else {
			infos[i->name] = *i;
			std::cout << "Source added: " << *i << std::endl;
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

	void PulseBackend::query_sink_info_by_name (char const* name) {
		pa_operation* o = pa_context_get_sink_info_by_name(ud->c,
			name, pa_sink_info_cb_t(callbacks::sink_info_list_cb), ud);
		pa_operation_unref(o);
	}

	void PulseBackend::query_server_info () {
		reset();
		pa_operation* o = pa_context_get_server_info(ud->c,
			pa_server_info_cb_t(callbacks::server_info_cb), ud);
		pa_operation_unref(o);
	}

} // namespace audio