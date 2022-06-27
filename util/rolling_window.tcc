#include <cstring>

template <typename SampleT>
class RollingWindow {
private:
	static constexpr size_t sample_bytes = sizeof(SampleT);
	SampleT* data;
	size_t window_length_samples;

public:
	RollingWindow (size_t window_length_samples, SampleT const& default_value) :
	window_length_samples(window_length_samples) {
		data = new SampleT[window_length_samples];
		for (size_t i = 0; i < window_length_samples; i++) {
			data[i] = default_value;
		}
	}

	~RollingWindow () {
		delete[] data;
	}

	SampleT* update (SampleT* const update, size_t update_length) {
		const static size_t window_len_bytes = window_length_samples * sample_bytes;
		const size_t update_len_bytes = update_length * sample_bytes;

		// Shift the existing data update_len_bytes bytes towands end
		memmove(data, data + update_length, window_len_bytes - update_len_bytes);
		// Add the new data to the front
		memcpy(data + window_length_samples - update_length, update, update_len_bytes);

		return data;
	}

	size_t window_length () {
		return window_length_samples;
	}
};