#include <stdexcept>

struct BPSWI_Spec {
	size_t win_length_samples; // Window length in samples
	bool win_window_fn; // Apply window function

	unsigned int interpolation_mult; // interpolation multiplication factor
	double* fft_freq_weighing = NULL; // abs(fft(window)) weighing
	double fft_dispersion; // arg(fft(window)) freq. dependent weighing
	BPSW_Phase fft_phase; // Type of phase manipulation for inverse trafo
	double fft_phase_const; // for fft_phase == BPSW_Phase.Constant constant phase value

	size_t crop_length_samples; // Displayed length of the window
	size_t crop_offset; // Display window offset

	unsigned int c_center_x, c_center_y; // Circle center location
	double c_rad_base, c_rad_extr; // Radius base and extrusion scaling
};

class BPSWInterpolated : public VisualizationHandler {
private:
	BPSWI_Spec params;
	RollingWindow<double> rollingWindow;
	FFTHandler fftHandler;
	bool const should_weigh = false;

	SDL_Point* points;
	double* circle_angles;

	void visualize (double const* data) {
		// Update the rolling window and
		double* const window_data = rollingWindow.update(data, VisualizationHandler::spec.samples);

		// Execute fourier transformation
		memset(fftHandler.real, 0, params.win_length_samples * params.interpolation_mult * sizeof(double));
		memcpy(fftHandler.real, window_data, params.win_length_samples * sizeof(double));
	    fftHandler.exec_r2c();

	    const size_t c_length = (params.win_length_samples * params.interpolation_mult) / 2 + 1;

	    // Convert to polar basis
	    double* abs_vals = new double[c_length];
	    double* arg_vals = new double[c_length];
	    for (size_t i = 0; i < c_length; i++) {
	        std::complex<double> c(fftHandler.complex[i][0], fftHandler.complex[i][1]);
	        abs_vals[i] = std::abs(c);
	        arg_vals[i] = std::arg(c);
	    }

	    // Transform polar frequency spectrum
	    for (size_t i = 0; i < c_length; i++) {
	        double const abs = abs_vals[i] * params.fft_freq_weighing[i];
    		double const bin_phase = 2 * M_PI * rollingWindow.current_index() / ((double) params.win_length_samples * params.interpolation_mult);

	        double arg;
	        switch (params.fft_phase) {
	        	case BPSW_Phase::Constant:
	        		arg = i * i * params.fft_dispersion;
	        		break;
	        	case BPSW_Phase::Standing:
	        		arg = arg_vals[i] - (i + params.fft_dispersion) * bin_phase;
	        		break;
	        	case BPSW_Phase::Unchanged:
	        		arg = arg_vals[i];
	        		break;
	        	default:
	        		arg = 0;
	        		break;
	        }

	        std::complex<double> c = std::polar(abs, arg);
	        fftHandler.complex[i][0] = std::real(c);
	        fftHandler.complex[i][1] = std::imag(c);
	    }
	    delete[] abs_vals;
	    delete[] arg_vals;

	    // Execute inverse fourier transformation
	    fftHandler.exec_c2r();

        // Crop to display area
        for (size_t i = 0; i < params.crop_length_samples * params.interpolation_mult; i++) {
	    	// Scaling is not preserved: irfft(rfft(x))[i] = x[i] * len(x)
	    	double const irfft_value = fftHandler.real[params.crop_offset + i] / (params.win_length_samples * params.interpolation_mult);
	        double const radius = params.c_rad_base + params.c_rad_extr * irfft_value;

	        points[i].x = params.c_center_x + radius * std::cos(circle_angles[i]);
	        points[i].y = params.c_center_y + radius * std::sin(circle_angles[i]);
        }

	}

	void render (SDL_Renderer* renderer) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 225);
		SDL_RenderDrawLines(renderer, points, params.crop_length_samples * params.interpolation_mult);
	}

public:

	BPSWInterpolated (SDL_AudioSpec const& spec, BPSWI_Spec const& params) :
		VisualizationHandler(spec),
		params(params),
		rollingWindow(params.win_length_samples, 0, params.win_window_fn),
		fftHandler(params.win_length_samples * params.interpolation_mult),
		should_weigh(params.fft_freq_weighing != NULL)
	{
		// assert(params.win_length_samples >= (params.crop_length_samples + params.crop_offset),
		// 	"Cropped area is too large, window insufficient");
		points = new SDL_Point[params.crop_length_samples * params.interpolation_mult];

		if (spec.samples > params.win_length_samples) {
			throw std::invalid_argument("Window cannot be shorter than samples per update");
		}

		circle_angles = new double[params.crop_length_samples * params.interpolation_mult];
		math::lin_space(circle_angles, params.crop_length_samples * params.interpolation_mult, M_PI, 3.0 * M_PI, true);
	}

	~BPSWInterpolated () {
		delete[] points;
		delete[] circle_angles;
	}
};
