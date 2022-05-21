#include "utils.h"

std::ostream& operator<< (std::ostream& os, pa_context_state_t const& state) {
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
	return os;
}

std::ostream& operator<< (std::ostream& os, pa_sample_format const& format) {
	switch (format) {
		case PA_SAMPLE_U8:
			return os << "PA_SAMPLE_U8";
		case PA_SAMPLE_ALAW:
			return os << "PA_SAMPLE_ALAW";
		case PA_SAMPLE_ULAW:
			return os << "PA_SAMPLE_ULAW";
		case PA_SAMPLE_S16LE:
			return os << "PA_SAMPLE_S16LE";
		case PA_SAMPLE_S16BE:
			return os << "PA_SAMPLE_S16BE";
		case PA_SAMPLE_FLOAT32LE:
			return os << "PA_SAMPLE_FLOAT32LE";
		case PA_SAMPLE_FLOAT32BE:
			return os << "PA_SAMPLE_FLOAT32BE";
		case PA_SAMPLE_S32LE:
			return os << "PA_SAMPLE_S32LE";
		case PA_SAMPLE_S32BE:
			return os << "PA_SAMPLE_S32BE";
		case PA_SAMPLE_S24LE:
			return os << "PA_SAMPLE_S24LE";
		case PA_SAMPLE_S24BE:
			return os << "PA_SAMPLE_S24BE";
		case PA_SAMPLE_S24_32LE:
			return os << "PA_SAMPLE_S24_32LE";
		case PA_SAMPLE_S24_32BE:
			return os << "PA_SAMPLE_S24_32BE";
		case PA_SAMPLE_MAX:
			return os << "PA_SAMPLE_MAX";
		case PA_SAMPLE_INVALID:
			return os << "PA_SAMPLE_INVALID";
	}
	return os;
}

std::ostream& operator<< (std::ostream& os, pa_encoding const& encoding) {
	switch (encoding) {
		case PA_ENCODING_ANY:
			return os << "PA_ENCODING_ANY";
		case PA_ENCODING_PCM:
			return os << "PA_ENCODING_PCM";
		case PA_ENCODING_AC3_IEC61937:
			return os << "PA_ENCODING_AC3_IEC61937";
		case PA_ENCODING_EAC3_IEC61937:
			return os << "PA_ENCODING_EAC3_IEC61937";
		case PA_ENCODING_MPEG_IEC61937:
			return os << "PA_ENCODING_MPEG_IEC61937";
		case PA_ENCODING_DTS_IEC61937:
			return os << "PA_ENCODING_DTS_IEC61937";
		case PA_ENCODING_MPEG2_AAC_IEC61937:
			return os << "PA_ENCODING_MPEG2_AAC_IEC61937";
		case PA_ENCODING_TRUEHD_IEC61937:
			return os << "PA_ENCODING_TRUEHD_IEC61937";
		case PA_ENCODING_DTSHD_IEC61937:
			return os << "PA_ENCODING_DTSHD_IEC61937";
		case PA_ENCODING_MAX:
			return os << "PA_ENCODING_MAX";
		case PA_ENCODING_INVALID:
			return os << "PA_ENCODING_INVALID";
	}
	return os;
}

std::ostream& operator<< (std::ostream& os, pa_sample_spec const& spec) {
	return os << "pa_sample_spec {\n"
		<< "\t\tformat = " << spec.format << "\n"
		<< "\t\trate = " << spec.rate << " Hz\n"
		<< "\t\tchannels = " << (uint16_t) spec.channels << "\n\t}";
}

std::ostream& operator<< (std::ostream& os, pa_proplist const& plist) {
	char* const out = pa_proplist_to_string_sep(&plist, "\n\t\t");
	os << "pa_proplist {\n\t\t" << out << "\n\t}\n";
	pa_xfree(out);
	return os;
}

std::ostream& operator<< (std::ostream& os, pa_source_info const& spec) {
	os << "pa_source_info {\n"
		<< "\tname = " << spec.name << "\n"
		<< "\tindex = " << spec.index << "\n"
		<< "\tdescription = " << spec.description << "\n"
		<< "\tsample_spec = " << spec.sample_spec << "\n"
		<< "\tlatency = " << spec.latency << "us\n"
		<< "\tconfigured_latency = " << spec.configured_latency << "us\n"
		<< "\tproplist = " << *(spec.proplist) << "\n"
		<< "\tformats = [\n";

	for (uint8_t i = 0; i < spec.n_formats; i++) {
		os << "\t" << *(*(spec.formats + i));
	}
	return os << "]\n\n}";
}

std::ostream& operator<< (std::ostream& os, pa_format_info const& info) {
	return os << "pa_format_info {\n"
		<< "\t\tencoding = " << info.encoding << "\n"
		<< "\t\tplist = " << *(info.plist) << "\n\t}";
}