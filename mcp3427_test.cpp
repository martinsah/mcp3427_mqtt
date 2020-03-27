#include <iostream>
#include "mcp3427.hpp"
#include <unistd.h>				//Needed for I2C port

int main()
{
	mcp3427 adc("/dev/i2c-1", 0x68);
	mcp3427 adc2("/dev/i2c-1", 0x6a);
	double wire1, wire2, res1, res2;
	int n=0;
	do{
		
		if(!(n & 0xf)){
			wire1 = adc.get_float(1) * 1000.0;
			wire2 = adc2.get_float(1) * 1000.0;
		}
		n++;
		
		res1 = adc.get_float(0) * 1000.0 - wire1 * 2;
		res2 = adc2.get_float(0) * 1000.0 - wire2 * 2;
		
		std::cout << n << "sensor1: " << res1 << "\tsensor2: " << res2 << std::endl;
		
		// std::cout << "adcl[0]: " << adc.get_float(0) * 1000.0 << std::endl;
		// std::cout << "adc2[0]: " << adc2.get_float(0)* 1000.0 << std::endl;
		// //sleep(1);
		// std::cout << "adcl[1]: " << adc.get_float(1) * 1000.0 << std::endl;
		// std::cout << "adc2[1]: " << adc2.get_float(1) * 1000.0 << std::endl;
		
	} while (1);

}