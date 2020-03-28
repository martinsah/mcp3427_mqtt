#include <iostream>
#include "mcp3427.hpp"
#include <unistd.h>				

#include <boost/program_options.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 

#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include "mqtt/async_client.h"


class ptx_convert {
	bool usa = false;
	const double A = 3.9083e-3;
	const double B = -5.775e-7;
	double R0 = 100.0;
	
	public:
	ptx_convert(double R0, bool usa){
		this->usa = usa;
		this->R0 = R0;
	}
	double operator()(double r){
		using namespace std;
		double c = (-R0 * A + sqrt(pow(R0,2) * pow(A,2) - 4 * R0 * B * (R0 - r)))/(2*R0*B);
		return c;
	}
};

int main(int ac, char **av)
{
	namespace po = boost::program_options;

	const std::string TOPIC { "data/rand" };
	const int	 QOS = 1;
	const auto PERIOD = std::chrono::seconds(5);
	const int MAX_BUFFERED_MSGS = 120;	// 120 * 5sec => 10min off-line buffering
	const std::string PERSIST_DIR { "data-persist" };
	const double alpha = 0.003851;
	
	
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
		("topic3",	po::value<std::string>()->default_value("adc/3"), "mqtt publish topic for adc3")
		("topic4",	po::value<std::string>()->default_value("adc/4"), "mqtt publish topic for adc4")
		("factor1",	po::value<double>()->default_value(1.0), "output factor for pt100/1")
		("factor2",	po::value<double>()->default_value(1.0), "output factor for pt100/2")
		("offset1",	po::value<double>()->default_value(0.0), "resistance offset for pt100/1")
		("offset2",	po::value<double>()->default_value(0.0), "resistance offset for pt100/2")
		("avg1",	po::value<int>()->default_value(1), "runtime average 'n' readings for adc1")
		("avg2",	po::value<int>()->default_value(1), "runtime average 'n' readings for adc2")
		("verbose",	"report to stdout")
		("pt100",	"treat ADC as PT100 readout")
		("csv",	"csv readout to stdout")
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
	int delay = 0;
	std::string topic1,topic2,topic3,topic4;
	
	std::string i2cbus(vm["i2cbus"].as<std::string>());
	std::string server(vm["server"].as<std::string>());
	
	int avg1, avg2, addr1, addr2, port;
	double factor1, factor2, offset1, offset2;
	
	port = vm["port"].as<int>();
	topic1 = vm["topic1"].as<std::string>();
	topic2 = vm["topic2"].as<std::string>();
	topic3 = vm["topic3"].as<std::string>();
	topic4 = vm["topic4"].as<std::string>();
	factor1 = vm["factor1"].as<double>();
	factor2 = vm["factor2"].as<double>();
	offset1 = vm["offset1"].as<double>();
	offset2 = vm["offset2"].as<double>();
	avg1 = vm["avg1"].as<int>();
	avg2 = vm["avg2"].as<int>();
	addr1 = vm["addr1"].as<int>();
	addr2 = vm["addr2"].as<int>();
	bool verbose = false;
	bool mode_pt100 = false;
	bool usa = false;
	bool mode_csv = false;
	
	delay = vm["delay"].as<int>();
	
	if (vm.count("verbose")){
		std::cout << "verbose set\n";
		verbose = true;
	}

	if(verbose){
		std::cout << "mqtt server set to " << vm["server"].as<std::string>() << "\n";
		std::cout << "mqtt port set to " << vm["port"].as<int>() << "\n";
		std::cout << "i2cbus is set to " << vm["i2cbus"].as<std::string>() << "\n";
		std::cout << "delay is set to " << vm["delay"].as<int>() << "\n";
		std::cout << "mqtt topic for adc1/1 or pt100/1 is set to " << vm["topic1"].as<std::string>() << "\n";
		std::cout << "mqtt topic for adc1/2 or pt100/2 is set to " << vm["topic2"].as<std::string>() << "\n";
		std::cout << "mqtt topic for adc2/1 is set to " << vm["topic3"].as<std::string>() << "\n";
		std::cout << "mqtt topic for adc2/2 is set to " << vm["topic4"].as<std::string>() << "\n";
		std::cout << "factor1 is set to " << vm["factor1"].as<double>() << "\n";
		std::cout << "factor2 is set to " << vm["factor2"].as<double>() << "\n";
		std::cout << "offset1 is set to " << vm["offset1"].as<double>() << "\n";
		std::cout << "offset2 is set to " << vm["offset2"].as<double>() << "\n";
		std::cout << "avg1 is set to " << vm["avg1"].as<int>() << "\n";
		std::cout << "avg2 is set to " << vm["avg2"].as<int>() << "\n";
		std::cout << "addr1 is set to " << vm["addr1"].as<int>() << "\n";
		std::cout << "addr2 is set to " << vm["addr2"].as<int>() << "\n";
	}
	
	if (vm.count("pt100")){
		mode_pt100 = true;
		if(verbose)
			std::cout << "pt100 mode is set\n";
	}
	
	if (vm.count("csv")){
		mode_csv = true;
		if(verbose)
			std::cout << "csv output mode is set\n";
	}
	
	if (vm.count("usa")){
		usa = true;
		if(verbose)
			std::cout << "pt100 temperature in Fahrenheit mode is set\n";
	}
	ptx_convert pt100(100, usa);
	
	// sensors
	mcp3427 adc(i2cbus, addr1);
	mcp3427 adc2(i2cbus, addr2);
	
	// mqtt
	std::string address = std::string("tcp://") + server + std::string(":") + std::to_string(port);
	mqtt::async_client cli(address, "", MAX_BUFFERED_MSGS, PERSIST_DIR);
	
	
	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(MAX_BUFFERED_MSGS * PERIOD);
	connOpts.set_clean_session(true);
	connOpts.set_automatic_reconnect(true);

	mqtt::topic pub_topic1(cli, topic1, QOS, true);
	mqtt::topic pub_topic2(cli, topic2, QOS, true);
	mqtt::topic pub_topic3(cli, topic3, QOS, true);
	mqtt::topic pub_topic4(cli, topic4, QOS, true);
	
	std::cout << "Connecting to server '" << address << "'..." << std::flush;
	cli.connect(connOpts)->wait();
	std::cout << "OK\n" << std::endl;
	std::string payload;		
	
	double wire1, wire2, res1, res2;
	int n=0;
	do{
		if(mode_pt100){
			if(!(n & 0xf)){
				wire1 = adc.get_float(1) * 1000.0;
				if(adc2)
					wire2 = adc2.get_float(1) * 1000.0;
			}
			n++;
			
			res1 = adc.get_float(0) * 1000.0 - wire1 * 2 + offset1;
			res1 = pt100(res1);
			payload = std::to_string(res1);
			if(verbose)
				std::cout << topic1 << " " << payload << std::endl;
			pub_topic1.publish(payload);
			
			if(adc2){
				res2 = adc2.get_float(0) * 1000.0 - wire2 * 2 + offset2;
				res2 = pt100(res2);
				payload = std::to_string(res2);
				if(verbose)
					std::cout << topic2 << " " << payload << std::endl;
				pub_topic2.publish(payload);
			}

		} else {
			res1 = adc.get_float(0);
			payload = std::to_string(res1);
			if(verbose)
				std::cout << topic1 << " " << payload << std::endl;
	
			pub_topic1.publish(payload);
			if(adc2){
				res2 = adc2.get_float(0);
				payload = std::to_string(res2);
				if(verbose)
					std::cout << topic2 << " " << payload << std::endl;
				pub_topic2.publish(payload);
			}
			if(mode_csv)
				std::cout << res1 << ", " << res2;
			
			res1 = adc.get_float(1);
			payload = std::to_string(res1);
			if(verbose)
				std::cout << topic3 << " " << payload << std::endl;
			pub_topic3.publish(payload);
			if(adc2){
				res2 = adc2.get_float(1);
				payload = std::to_string(res2);
				if(verbose)
					std::cout << topic4 << " " << payload << std::endl;
				pub_topic4.publish(payload);
			}
			if(mode_csv)
				std::cout << ", " << res1 << ", " << res2 << std::endl;
		}
		boost::this_thread::sleep_for(boost::chrono::milliseconds(delay));
		
	} while (1);

}