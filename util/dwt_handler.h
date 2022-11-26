#include <cstring>

class DWTHandler {

private:
	size_t len;
	int level;

public:
	DWTHandler (size_t len, int level);
	~DWTHandler ();

	double* real;
	void* inverse;

	void exec_dwt ();
	void exec_idwt ();
};