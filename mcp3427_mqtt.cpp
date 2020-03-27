#include <iostream>
#include "mcp3427.hpp"
#include <unistd.h>				
#include <mosquittopp.h>
#include <boost/program_options.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 



int main(int ac, char **av)
{
	namespace po = boost::program_options;
	
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("server", 	po::value<std::string>()->default_value("localhost"), "mqtt server")
		("port", 	po::value<int>()->default_value(1883),"mqtt port")
		("i2cbus", 	po::value<std::string>()->default_value("/dev/i2c-1"), "i2c bus, e.g. /dev/i2c-1")
		("delay",	po::value<int>()->default_value(100), "delay between readings (integer milliseconds)")
		("topic1",	po::value<std::string>()->default_value("adc/1"), "mqtt publish topic for adc1")
		("topic2",	po::value<std::string>()->default_value("adc/2"), "mqtt publish topic for adc2")
		("factor1",	po::value<double>()->default_value(1.0), "output factor for adc1")
		("factor2",	po::value<double>()->default_value(1.0), "output factor for adc2")
		("offset1",	po::value<double>()->default_value(0.0), "output offset for adc1")
		("offset2",	po::value<double>()->default_value(0.0), "output offset for adc2")
		("avg1",	po::value<int>()->default_value(1), "runtime average 'n' readings for adc1")
		("avg2",	po::value<int>()->default_value(1), "runtime average 'n' readings for adc2")
		("verbose",	"report to stdout")
		("pt100",	"treat ADC as PT100 readout")
		("addr1",	po::value<int>()->default_value(0x68),"i2c address of ADC1")
		("addr2",	po::value<int>()->default_value(0x6a),"i2c address of ADC2")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("server"))
		std::cout << "mqtt server set to " << vm["server"].as<std::string>() << "\n";
	
	if (vm.count("port"))
		std::cout << "mqtt port set to " << vm["port"].as<int>() << "\n";
	
	if (vm.count("i2cbus"))
		std::cout << "i2cbus is set to " << vm["i2cbus"].as<std::string>() << "\n";
	
	int delay = 0;
	if (vm.count("delay")){
		std::cout << "delay is set to " << vm["delay"].as<int>() << "\n";
		delay = vm["delay"].as<int>();
	}
	
	std::string topic1,topic2;
	topic1 = vm["topic1"].as<std::string>();
	topic2 = vm["topic2"].as<std::string>();
	
	if (vm.count("topic1"))
		std::cout << "mqtt topic for adc1 is set to " << vm["topic1"].as<std::string>() << "\n";
	
	if (vm.count("topic2"))
		std::cout << "mqtt topic for adc2 is set to " << vm["topic2"].as<std::string>() << "\n";
	
	double factor1, factor2, offset1, offset2;
	factor1 = vm["factor1"].as<double>();
	factor2 = vm["factor2"].as<double>();
	offset1 = vm["offset1"].as<double>();
	offset2 = vm["offset2"].as<double>();
	
	if (vm.count("factor1"))
		std::cout << "factor1 is set to " << vm["factor1"].as<double>() << "\n";
	
	if (vm.count("factor2"))
		std::cout << "factor2 is set to " << vm["factor2"].as<double>() << "\n";
	
	if (vm.count("offset1"))
		std::cout << "offset1 is set to " << vm["offset1"].as<double>() << "\n";
	
	if (vm.count("offset2"))
		std::cout << "offset2 is set to " << vm["offset2"].as<double>() << "\n";
	
	int avg1, avg2, addr1, addr2;
	avg1 = vm["avg1"].as<int>();
	avg2 = vm["avg2"].as<int>();
	addr1 = vm["addr1"].as<int>();
	addr2 = vm["addr2"].as<int>();
	
	if (vm.count("avg1"))
		std::cout << "avg1 is set to " << vm["avg1"].as<int>() << "\n";
	
	if (vm.count("avg2"))
		std::cout << "avg2 is set to " << vm["avg2"].as<int>() << "\n";

	if (vm.count("addr1"))
		std::cout << "addr1 is set to " << vm["addr1"].as<int>() << "\n";

	if (vm.count("addr2"))
		std::cout << "addr2 is set to " << vm["addr2"].as<int>() << "\n";

	bool verbose = false;
	if (vm.count("verbose")){
		std::cout << "verbose set\n";
		verbose = true;
	}

	bool pt100 = false;
	if (vm.count("pt100")){
		std::cout << "pt100 set\n";
		pt100 = true;
	}
	
	std::string i2cbus(vm["i2cbus"].as<std::string>());
	
	mcp3427 adc(i2cbus, addr1);
	mcp3427 adc2(i2cbus, addr2);
	
	double wire1, wire2, res1, res2;
	int n=0;
	do{
		if(pt100){
			if(!(n & 0xf)){
				wire1 = adc.get_float(1) * 1000.0;
				if(adc2)
					wire2 = adc2.get_float(1) * 1000.0;
			}
			n++;
			
			res1 = adc.get_float(0) * 1000.0 - wire1 * 2;
			std::cout << topic1 << " " << res1;
			if(adc2){
				res2 = adc2.get_float(0) * 1000.0 - wire2 * 2;
				std::cout << topic2 << " " << res2;
			}
			std::cout << std::endl;
		} else {
			res1 = adc.get_float(0);
			std::cout << topic1 << "/1 " << res1;
			if(adc2){
				res2 = adc2.get_float(0);
				std::cout << topic2 << "/1 " << res2;
			}
			std::cout << std::endl;
			
			res1 = adc.get_float(1);
			std::cout << topic1 << "/2 " << res1;
			if(adc2){
				res2 = adc2.get_float(1);
				std::cout << topic2 << "/2 " << res2;
			}
			std::cout << std::endl;
		}
		boost::this_thread::sleep_for(boost::chrono::milliseconds(delay));
		
	} while (1);

}