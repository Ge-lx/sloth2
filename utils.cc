#include "utils.h"

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
	return os;
}