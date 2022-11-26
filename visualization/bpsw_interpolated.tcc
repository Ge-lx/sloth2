#include <stdexcept>
#include "../third_party/spline/src/spline.h"

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
	FFTHandler fftHandlerC2R;
	FFTHandler fftHandlerR2C;
	bool const should_weigh = false;
	size_t length_interpolated;

	SDL_Point* points;
	double* circle_angles;

	void visualize (double const* data) {
		// Update the rolling window and
		double* const window_data = rollingWindow.update(data, VisualizationHandler::spec.samples);

		// Execute fourier transformation
		memset(fftHandlerR2C.real, 0, params.win_length_samples * sizeof(double));
		memcpy(fftHandlerR2C.real, window_data, params.win_length_samples * sizeof(double));
	    fftHandlerR2C.exec_r2c();

	    const size_t c_length = params.win_length_samples / 2 + 1;
	    const size_t c_length_int = (params.win_length_samples * params.interpolation_mult) / 2 + 1;

	    std::vector<double> complex_abs;
	    std::vector<double> complex_arg;
	    std::vector<double> x_axis_pre;
	    complex_abs.resize(c_length);
	    complex_arg.resize(c_length);
	    x_axis_pre.resize(c_length);

	    for (size_t i = 0; i < c_length; i++) {
	        std::complex<double> c(fftHandlerR2C.complex[i][0], fftHandlerR2C.complex[i][1]);
	        complex_abs[i] = std::abs(c);
	        complex_arg[i] = std::arg(c);
	    	x_axis_pre[i] = i * params.interpolation_mult;
	    }

	    // std::cout << "x_axis_pre.size() :" << x_axis_pre.size() << "\n";
	    // std::cout << "complex_re.size() :" << complex_re.size() << "\n";
	    // std::cout << "complex_im.size() :" << complex_im.size() << "\n";

	    // Interpolate fftHandlerR2C.complex[win_length_samples *= params.interpolation_mult]
	    tk::spline spline_abs(x_axis_pre, complex_abs, tk::spline::spline_type::cspline);
	    tk::spline spline_arg(x_axis_pre, complex_arg, tk::spline::spline_type::linear);

	    // Convert to polar basis
	    double* abs_vals = new double[c_length_int];
	    double* arg_vals = new double[c_length_int];
	    for (size_t i = 0; i < c_length_int; i++) {
	    	abs_vals[i] = spline_abs(i);
	    	arg_vals[i] = spline_arg(i);
	    }

	    // Transform polar frequency spectrum
	    for (size_t i = 0; i < c_length_int; i++) {
	        double const abs = abs_vals[i] * params.fft_freq_weighing[i];
    		double const bin_phase = 2 * M_PI * rollingWindow.current_index() / (double) params.win_length_samples;

	        double arg;
	        switch (params.fft_phase) {
	        	case BPSW_Phase::Constant:
	        		arg = i * i * params.fft_dispersion;
	        		break;
	        	case BPSW_Phase::Standing:
	        		arg = arg_vals[i] - (i * params.interpolation_mult + params.fft_dispersion) * bin_phase;
	        		break;
	        	case BPSW_Phase::Unchanged:
	        		arg = arg_vals[i];
	        		break;
	        	default:
	        		arg = 0;
	        		break;
	        }

	        std::complex<double> c = std::polar(abs, arg);
	        fftHandlerC2R.complex[i][0] = std::real(c);
	        fftHandlerC2R.complex[i][1] = std::imag(c);
	    }
	    delete[] abs_vals;
	    delete[] arg_vals;

	    // Execute inverse fourier transformation
	    fftHandlerC2R.exec_c2r();

        // Crop to display area
        for (size_t i = 0; i < length_interpolated; i++) {
	    	// Scaling is not preserved: irfft(rfft(x))[i] = x[i] * len(x)
	    	double const irfft_value = fftHandlerC2R.real[i] / length_interpolated;
	        double const radius = params.c_rad_base + params.c_rad_extr * irfft_value;

	        points[i].x = params.c_center_x + radius * std::cos(circle_angles[i]);
	        points[i].y = params.c_center_y + radius * std::sin(circle_angles[i]);
        }

	}

	void render (SDL_Renderer* renderer) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 225);
		SDL_RenderDrawLines(renderer, points, length_interpolated);
	}

public:

	BPSWInterpolated (SDL_AudioSpec const& spec, BPSWI_Spec const& params) :
		VisualizationHandler(spec),
		params(params),
		rollingWindow(params.win_length_samples, 0, params.win_window_fn),
		fftHandlerC2R(params.win_length_samples * params.interpolation_mult),
		fftHandlerR2C(params.win_length_samples),
		should_weigh(params.fft_freq_weighing != NULL),
		length_interpolated(params.win_length_samples * params.interpolation_mult)
	{
		// assert(params.win_length_samples >= (params.crop_length_samples + params.crop_offset),
		// 	"Cropped area is too large, window insufficient");
		points = new SDL_Point[length_interpolated];

		if (spec.samples > params.win_length_samples) {
			throw std::invalid_argument("Window cannot be shorter than samples per update");
		}

		circle_angles = new double[length_interpolated];
		math::lin_space(circle_angles, length_interpolated, M_PI, 3.0 * M_PI, true);
	}

	~BPSWInterpolated () {
		delete[] points;
		delete[] circle_angles;
	}
};
