#include <stdint.h>
#include <string>
#include <signal.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <wiringPi.h>
#include <syslog.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <systemd/sd-daemon.h> // needed for sd_notify, if compiler error try to run >> sudo apt-get install libsystemd-dev
#include "cc1101.h"
#include "sensorKNXRF.h"

using namespace std;

#define GDO0idx 0
#define GDO2idx 1

volatile sig_atomic_t stopprogram;

void handle_signal(int sig)
{
	if (sig != 17) {
		syslog(LOG_INFO, "MonitorKNXRF received: %d", sig);
		stopprogram = 1;
	}
}

// Setting debug level to !0 equal verbose
uint8_t cc1101_freq_select = 3;
uint8_t cc1101_mode_select = 2;
uint8_t cc1101_channel_select = 0;
uint8_t cc1101_debug = 0;

// Global variables
CC1101 cc1101;
SensorKNXRF *sensorBuffer = NULL;

/*
 * Interrupts:
 *********************************************************************************
 */
void cc1101InterruptGDO2(void) {
	uint8_t dataBuffer[CC1101_BUFFER_LEN] ={0xFF}, dataLen;
	piLock(GDO2idx);
	cc1101.rx_payload_burst(dataBuffer, dataLen);
	saveSensorData(dataBuffer, dataLen, sensorBuffer);
	piUnlock(GDO2idx);
}


// Ugly hack to write sensor values into output file
void writeSensorToFile(char* id, int temp, int set) {
	char temperature[5];
	char setpoint[5];
        float tempTemp = (float)temp/(float)100;
	float tempSet = (float)set/(float)100;
	vector <string> names;
	vector <string> ids;
	vector <string> temperatures;
	vector <string> setpoints;

        string file = "knxrfSensors.json";
        sprintf(temperature, "%.2f", tempTemp);
        sprintf(setpoint, "%.2f", tempSet);

	// Read the file into vectors
	string line;
	ifstream infile (file);
	if (infile.is_open())
	{
		while ( getline (infile,line) )
		{
			names.push_back( line.substr( line.find("\"")+1, line.find("id")-line.find("\"")-7 ) );
			ids.push_back(line.substr(line.find("id")+6, line.find("temperature") - line.find("id") - 10));
			temperatures.push_back(line.substr(line.find("temperature")+14, line.find("setpoint") - line.find("temperature") - 17));
			setpoints.push_back(line.substr(line.find("setpoint")+11, line.length() - line.find("setpoint") - 14));
		}
		infile.close();
	}

	// Push new or modify existing values if id is found in the vector or not. Check if value is reasonable.
	std::vector<string>::iterator it;
        it = find (ids.begin(), ids.end(), id);
	if (it != ids.end()) {
		if (temp < 4000) temperatures[it - ids.begin()] = temperature;
		if (set < 4000) setpoints[it - ids.begin()] = setpoint;
	}
	else {
	        names.push_back("UnnamedSensor");
	        ids.push_back(id);
	        temperatures.push_back(temperature);
	        setpoints.push_back(setpoint);
	}

        // Write the file
	ofstream outfile;
	outfile.open(file);
        outfile << "{";

	for (unsigned int i = 0; i < names.size(); i++) {
	        outfile << "\"" << names[i] << "\": { \"id\": \"" << ids[i] << "\", \"temperature\": " << temperatures[i] << ", \"setpoint\": " << setpoints[i] << " }";
                if (i < names.size()-1) outfile << "," << endl;
                else outfile << "}" << endl;
	}
	outfile.close();
}

/*
 *********************************************************************************
 * main
 *********************************************************************************
 */
int main (int argc, char* argv[])
{
	char s[256];
        char sensorId[13];
	uint8_t addrCC1101 = 0;
	int internalWD = 0;
	int exitCode = EXIT_SUCCESS;

	if (argc > 1) {
		cc1101_debug = atoi(argv[1]);
	} else {
		cc1101_debug = 0;
	}
	syslog(LOG_INFO, "MonitorKNXRF started");

	for (int n = 1; n < 32; n++) {
		signal(n, handle_signal);
	}

	stopprogram = 0;
	cc1101.begin(addrCC1101);		//setup cc1101 RF IC
	wiringPiSetup();			//setup wiringPi library
	wiringPiISR (GDO2, INT_EDGE_RISING, &cc1101InterruptGDO2) ;
	cc1101.show_register_settings();

	try {
		while (!stopprogram) {
			delay(15000);
			sd_notify(0,"WATCHDOG=1");
			internalWD++;
			while (sensorBuffer) {
                                int sensorTemperature = transformTemperature(sensorBuffer->sensorData[1]);
                                int sensorSetpoint = transformTemperature(sensorBuffer->sensorData[2]);
				sprintf(sensorId, "%04X%08X", sensorBuffer->serialNoHighWord, sensorBuffer->serialNoLowWord);

				sprintf(s, "MonitorKNXRF got data from sensor %s reading %d and setpoint %d.", sensorId, sensorTemperature, sensorSetpoint);
				syslog(LOG_INFO, s);

				piLock(GDO2idx);
				writeSensorToFile(sensorId, sensorTemperature, sensorSetpoint);
                                sensorBuffer = NULL;
				piUnlock(GDO2idx);
				delay(1);
				internalWD = 0;
			}
			if (internalWD > 8) {
				stopprogram = 1;
				syslog(LOG_ERR, "MonitorKNXRF stopping due to no data received from CC1101");
				exitCode = EXIT_FAILURE;
			}
		}
	} catch (const std::exception& e) {
		syslog(LOG_ERR, "A standard exception was caught, with message: '%s'", e.what());
		exitCode = EXIT_FAILURE;
    }

    cc1101.end();
    syslog(LOG_INFO, "MonitorKNXRF stopped");
    return exitCode;
}
