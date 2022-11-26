#include <stdexcept>

#include "../util/dwt_handler.h"

struct Wavelet_Spec {
	size_t win_length_samples; // Window length in samples
	bool win_window_fn; // Apply window function

	double* fft_freq_weighing = NULL; // abs(fft(window)) weighing
	double fft_dispersion; // arg(fft(window)) freq. dependent weighing
	BPSW_Phase fft_phase; // Type of phase manipulation for inverse trafo
	double fft_phase_const; // for fft_phase == BPSW_Phase.Constant constant phase value

	size_t crop_length_samples; // Displayed length of the window
	size_t crop_offset; // Display window offset

	unsigned int c_center_x, c_center_y; // Circle center location
	double c_rad_base, c_rad_extr; // Radius base and extrusion scaling
};

class Wavelet : public VisualizationHandler {
private:
	Wavelet_Spec params;
	RollingWindow<double> rollingWindow;
	DWTHandler dwtHandler;
	bool const should_weigh = false;

	SDL_Point* points;
	double* circle_angles;

	void visualize (double const* data) {
		// Update the rolling window and
		double* const window_data = rollingWindow.update(data, VisualizationHandler::spec.samples);

		// Execute fourier transformation
		memcpy(dwtHandler.real, window_data, params.win_length_samples * sizeof(double));
	    dwtHandler.exec_dwt();

		/*

	    // Convert to polar basis
	    const size_t c_length = params.win_length_samples / 2 + 1;
	    double* abs_vals = new double[c_length];
	    double* arg_vals = new double[c_length];
	    for (size_t i = 0; i < c_length; i++) {
	        std::complex<double> c(dwtHandler.complex[i][0], dwtHandler.complex[i][1]);
	        abs_vals[i] = std::abs(c);
	        arg_vals[i] = std::arg(c);
	    }

	    // Transform polar frequency spectrum
	    for (size_t i = 0; i < c_length; i++) {
	        double bin_phase = 2 * M_PI * rollingWindow.current_index() / ((double) params.win_length_samples);
	        double abs_weighted = abs_vals[i] * params.fft_freq_weighing[i];
	        double arg_shifted = params.fft_phase == BPSW_Phase::Constant ?
	        	i * i * params.fft_dispersion :
	        	arg_vals[i] - (i + params.fft_dispersion) * bin_phase;
	        std::complex<double> c = std::polar(abs_weighted, arg_shifted);

	        dwtHandler.complex[i][0] = std::real(c);
	        dwtHandler.complex[i][1] = std::imag(c);
	    }
	    delete[] abs_vals;
	    delete[] arg_vals;

	    // Execute inverse fourier transformation
	    dwtHandler.exec_c2r();

	    */

        // Crop to display area
        for (size_t i = 0; i < params.crop_length_samples; i++) {
	    	// Scaling is not preserved: irfft(rfft(x))[i] = x[i] * len(x)
	    	double const irfft_value = dwtHandler.real[params.crop_offset + i]/* / params.win_length_samples*/;
	        double const radius = params.c_rad_base + params.c_rad_extr * irfft_value;

	        points[i].x = params.c_center_x + radius * std::cos(circle_angles[i]);
	        points[i].y = params.c_center_y + radius * std::sin(circle_angles[i]);
        }
	}

	void render (SDL_Renderer* renderer) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 225);
		SDL_RenderDrawLines(renderer, points, params.crop_length_samples);
	}

public:

	Wavelet (SDL_AudioSpec const& spec, Wavelet_Spec const& params) :
		VisualizationHandler(spec),
		params(params),
		rollingWindow(params.win_length_samples, 0, params.win_window_fn),
		dwtHandler(params.win_length_samples, 1),
		should_weigh(params.fft_freq_weighing != NULL)
	{
		points = new SDL_Point[params.crop_length_samples];

		if (spec.samples > params.win_length_samples) {
			throw std::invalid_argument("Window cannot be shorter than samples per update");
		}

		circle_angles = new double[params.crop_length_samples];
		math::lin_space(circle_angles, params.crop_length_samples, M_PI, 3.0 * M_PI, true);
	}

	~Wavelet () {
		delete[] points;
		delete[] circle_angles;
	}
};
