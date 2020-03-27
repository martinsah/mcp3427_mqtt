#include <stdio.h>
#include <errno.h>
#include <unistd.h>				
#include <fcntl.h>				
#include <sys/ioctl.h>			
#include <linux/i2c-dev.h>		
#include <iostream>
#include <boost/exception/all.hpp>
#include <boost/shared_ptr.hpp>


class mcp3427 {
	int file_i2c;
	unsigned char buffer[4];
	int channel, rx_channel;
	int continuous_mode;
	int bit_mode;
	int pga_bits;
	int rx_val;
	double rx_val_d;
	const double c_lsb = 62.5e-6;
	bool rx_ready;
	bool ready = false;
	
public:
	mcp3427(std::string bus, int addr);
	void config(int channel, int continuous_mode, int bit_mode, int pga_bits);
	int config(char word);
	int config_write();
	int get(int channel);
	int getbuffer();
	void print_rx_buffer();
	double get_float(int channel);
	operator bool() {return this->ready;}
};

mcp3427::mcp3427(std::string bus, int addr)
{
	if(addr == 0)
		return;
	
	//char *filename = (char*)"/dev/i2c-1";
	char *filename;
	//filename = bus;
	if ((file_i2c = open(bus.c_str(), O_RDWR)) < 0)
	{
		BOOST_THROW_EXCEPTION(std::runtime_error("Failed to open i2c bus"));
	}
	int length;

	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		BOOST_THROW_EXCEPTION(std::runtime_error("Failed to acquire bus access and/or talk to slave.\n"));
	}

	//----- WRITE BYTES -----
	char word = 0x98; // 1 00 1 10 00
						// 1 = no effect
						// 00 = channel 0
						// 1 = continuous conversion mode
						// 10 = 16 bit mode
						// 00 = x1 PGA
	config(word);
	
	config(0, 1, 2, 0);
	this->ready = true;
}

void mcp3427::config(int channel, int continuous_mode, int bit_mode, int pga_bits)
{
	this->channel = channel;
	this->continuous_mode = continuous_mode;
	this->bit_mode = bit_mode;
	this->pga_bits = pga_bits;
}

int mcp3427::config_write()
{
	return this->config(
		((char)channel << 5) |
		((char)continuous_mode << 4) | 
		((char)bit_mode << 2) | 
		((char)pga_bits & 0x3)
	);
}

int mcp3427::config(char word)
{
	if (write(this->file_i2c, &word, 1) != 1)
	{
		BOOST_THROW_EXCEPTION(std::runtime_error("Failed to write to i2c bus"));
	}
	return 1;
}

int mcp3427::getbuffer()
{
	if (read(this->file_i2c, this->buffer, 3) != 3)
	{
		BOOST_THROW_EXCEPTION(std::runtime_error("Failed to read i2c bus"));
	}
	rx_channel = ((buffer[2] >> 5) & 0x3);
	rx_val = ((int)(buffer[0]))<<8 | (int)buffer[1];
	if(rx_val & 0x8000){
		rx_val ^= 0xFFFF;
		rx_val += 1;
		rx_val *= -1;
	}
	rx_val_d = ((double)rx_val) * c_lsb;
	rx_ready = (buffer[2] & 0x80) == 0;
	return 1;
}

void mcp3427::print_rx_buffer()
{
	for(int x=0;x<3;x++){
		printf("0x%X ", buffer[x]);
	}
}

int mcp3427::get(int channel)
{
	if(this->channel != channel){
		this->channel = channel;
		config_write();
	}
	do{
		if (this->getbuffer())
		{
			if(rx_channel == channel && rx_ready)
				return this->rx_val;
		} 
	} while(rx_channel != channel || !rx_ready);
	return 0;
}

double mcp3427::get_float(int channel)
{
	get(channel);
	return rx_val_d;
}

