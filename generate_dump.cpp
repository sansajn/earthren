#include <iostream>
int main() {
	int *p = nullptr;
	*p = 0;  // This will cause a segmentation fault
	return 0;
}
