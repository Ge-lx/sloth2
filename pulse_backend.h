#pragma once

#include <iostream>
#include <map>

#include "pulse/pulseaudio.h"
#include "utils.h"

namespace audio {

	namespace callbacks {
		void source_info_list_cb (pa_context*, pa_source_info*, int, void*);
		void sink_info_list_cb (pa_context*, pa_sink_info* const, int, void*);
		void server_info_cb (pa_context*, pa_server_info* const, void*);
		void context_state_cb (pa_context*, void*);
	}

	class PulseBackend;
	struct UD {
		pa_threaded_mainloop* m;
		pa_context* c;
		PulseBackend* backend;
	};

	class PulseBackend {
		typedef void (*server_info_source_list_cb_t)(PulseBackend*);

	private:
		UD* ud;
		server_info_source_list_cb_t callback;
		std::map<char const*, pa_source_info> infos;
		pa_sink_info default_sink;
		char const* force_sink_name;
		bool eol_received = false;
		bool default_sink_received = false;
		bool data_valid = false;

		void on_partial_data ();
		void reset ();
		void register_default_sink(pa_sink_info* i);
		void add_source(pa_source_info* i, bool eol = false);
		void query_all_source_info ();
		void query_sink_info_by_name (char const* name);
		void query_server_info ();
		void connect_record_stream_to_source_by_name ();

	public:
		PulseBackend(char const* force_sink_name = NULL);

		void start();
		void stop();

		friend void callbacks::source_info_list_cb (pa_context*, pa_source_info*, int, void*);
		friend void callbacks::sink_info_list_cb (pa_context*, pa_sink_info* const, int, void*);
		friend void callbacks::server_info_cb (pa_context*, pa_server_info* const, void*);
		friend void callbacks::context_state_cb (pa_context*, void*);
	};
} // namespace audio