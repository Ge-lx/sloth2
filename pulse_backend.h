#pragma once

#include <iostream>
#include <map>

#include "pulse/pulseaudio.h"
#include "utils.h"

namespace audio {

	namespace callbacks {
		void source_info_list_cb (pa_context*, pa_source_info*, int, void*);
		void sink_info_list_cb (pa_context*, pa_sink_info*, int, void*);
		void sink_input_info_list_cb(pa_context*, pa_sink_input_info*, int, void*);
		void server_info_cb (pa_context*, pa_server_info*, void*);
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
		std::map<char const*, pa_source_info> source_infos;
		std::map<char const*, pa_sink_info> sink_infos;
		std::map<char const*, pa_sink_input_info> sink_input_infos;
		pa_server_info server_info;
		char const* force_sink_name;
		bool eol_sinks_received = false;
		bool eol_sink_inputs_received = false;
		bool eol_sources_received = false;
		bool data_valid = false;

		void on_partial_data ();
		void reset ();
		void add_source (pa_source_info* i, bool eol = false);
		void add_sink (pa_sink_info* i, bool eol = false);
		void add_sink_input (pa_sink_input_info* i, bool eol = false);
		void query_all_source_info ();
		void query_all_sink_info ();
		void query_all_sink_input_info ();
		void query_server_info ();
		void connect_record_stream_to_source_by_name ();

	public:
		PulseBackend(char const* force_sink_name = NULL);

		void start();
		void stop();

		friend void callbacks::source_info_list_cb (pa_context*, pa_source_info*, int, void*);
		friend void callbacks::sink_info_list_cb (pa_context*, pa_sink_info*, int, void*);
		friend void callbacks::sink_input_info_list_cb(pa_context*, pa_sink_input_info*, int, void*);
		friend void callbacks::server_info_cb (pa_context*, pa_server_info*, void*);
		friend void callbacks::context_state_cb (pa_context*, void*);
	};
} // namespace audio