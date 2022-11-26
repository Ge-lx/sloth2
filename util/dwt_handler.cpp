#include "dwt_handler.h"
#include "wavelet2d.h"

#include <iostream>

DWTHandler::DWTHandler (size_t len, int level) : len(len), level(level) {
	real = new double[len];
	inverse = NULL;
}

DWTHandler::~DWTHandler () {
	delete[] real;
}

void DWTHandler::exec_dwt () {
	vector<double> data;
	data.assign(real, real + len);

	vector<double> output, flag;
	vector<int> length;
	dwt_sym(data, level, "db5", output, flag, length);
	// std::cout << " --------- DWT -------------- " << std::endl;
	// std::cout << "input length: " << data.size() << std::endl;
	// std::cout << "output length: " << output.size() << std::endl;
	// std::cout << "flag length: " << flag.size() << std::endl;
	// std::cout << "length length: " << length.size() << std::endl;

	idwt_sym(output, flag, "db5", data, length);
	memcpy(real, data.data(), len * sizeof(double));
	// std::cout << " --------- IDWT -------------- " << std::endl;
	// std::cout << "input length: " << data.size() << std::endl;
	// std::cout << "output length: " << output.size() << std::endl;
	// std::cout << "flag length: " << flag.size() << std::endl;
	// std::cout << "length length: " << length.size() << std::endl;
	// std::cout << std::endl;
}