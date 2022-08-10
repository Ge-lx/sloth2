#include <cstring>

template <typename SampleT>
class RollingWindow {
private:
	static constexpr size_t sample_bytes = sizeof(SampleT);
	SampleT* data;
	SampleT* windowed;
	size_t window_length_samples;
	// int i = 0;

public:
	SampleT* window_function;

	RollingWindow (size_t window_length_samples, SampleT const& default_value) :
	window_length_samples(window_length_samples) {
		data = new SampleT[window_length_samples];
	    window_function = new SampleT[window_length_samples];
	    windowed = new SampleT[window_length_samples];

		for (size_t i = 0; i < window_length_samples; i++) {
			data[i] = default_value;
			windowed[i] = default_value;
		}

	    for (size_t i = 0; i < window_length_samples; i++) {
	        window_function[i] = (.5 * (1 - std::cos(2*M_PI*i)/(window_length_samples)));
	    }
	}

	~RollingWindow () {
		delete[] data;
		delete[] window_function;
		delete[] windowed;
	}

	SampleT* update (SampleT* const update, size_t update_length) {
		const static size_t window_len_bytes = window_length_samples * sample_bytes;
		const size_t update_len_bytes = update_length * sample_bytes;

		// if (++i % 2 == 0) {
		// 	for (size_t i = 0; i < update_length; i++) {
		// 		update[i] = 0;
		// 	}
		// }

		// Shift the existing data update_len_bytes bytes towands end
		memmove(data, data + update_length, window_len_bytes - update_len_bytes);
		// Add the new data to the front
		memcpy(data + window_length_samples - update_length, update, update_len_bytes);

		for (size_t i = 0; i < window_length_samples; i++) {
			windowed[i] = window_function[i] * data[i];
		}

		return windowed;
	}

	size_t window_length () {
		return window_length_samples;
	}
};