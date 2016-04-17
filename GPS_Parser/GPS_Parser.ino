#include "GPS_Parser.hpp"

GPS_Parser demo;
nmea_err_t err;
bool result = true;

bool test(nmea_err_t err, nmea_err_t expected);

void setup() {
	Serial.begin(115200);
	delay(500);

	Serial.println("Testbed online");
	Serial.println();

	Serial.println("Testing master parser...");
	Serial.println();

	err = demo.parse(NULL);
	result &= test(err, nmea_err_null);

	err = demo.parse("$GPRMC,");
	result &= test(err, nmea_err_nocsum);

	err = demo.parse("$GPRMC,*");
	result &= test(err, nmea_err_badcsum);

	err = demo.parse("$GPRMC,*0");
	result &= test(err, nmea_err_badcsum);

	err = demo.parse("$GPRMC,*00");
	result &= test(err, nmea_err_badcsum);

	err = demo.parse("$GPRMC,203826.123,A,4137.8868,N,08500.4129,W,1.02,297.04,160416,,,D*77");
	result &= test(err, nmea_success);

	Serial.println();
	Serial.println("--------------------");
	Serial.println(result ? "TESTS SUCCEEDED" : "TESTS FAILED");
	Serial.println("--------------------");
	Serial.println();

	err = demo.parse("$GPGGA,203827.123,4137.8873,N,08500.4143,W,2,05,1.37,308.3,M,-33.8,M,0000,0000*56");
	result &= test(err, nmea_success);

	err = demo.parse("$GPGLL,4137.8873,N,08500.4143,W,203827.123,A,D*48");
	result &= test(err, nmea_success);

	//  Optional, satellites in active use
	err = demo.parse("$GPGSA,A,3,11,27,23,28,08,,,,,,,,1.66,1.37,0.94*0D");
	result &= test(err, nmea_err_unknown);

	//  Optional, satellites in view
	err = demo.parse("$GPGSV,3,1,10,08,72,126,17,07,59,314,,27,51,059,20,09,47,223,*7F");
	result &= test(err, nmea_err_unknown);

	demo.print_info();
}

void loop() {
}

bool test(nmea_err_t err, nmea_err_t expected) {
	bool ret = (err == expected);
	if (ret) {
		switch (expected) {
			case nmea_success:
				Serial.println("SUCCESS: Parse completed.");
				break;
			case nmea_err_null:
				Serial.println("SUCCESS: Detected NULL pointer.");
				break;
			case nmea_err_unknown:
				Serial.println("SUCCESS: Determined unknown NMEA sentence.");
				break;
			case nmea_err_nocsum:
				Serial.println("SUCCESS: Detected missing checksum.");
				break;
			case nmea_err_badcsum:
				Serial.println("SUCCESS: Detected invalid checksum.");
				break;
			default:
				Serial.print("TEST FAILURE: Return code 0x");
				Serial.print(err);
				Serial.println(" is not currently handled.");
		}
	}
	else {
		switch (expected) {
			case nmea_success:
				Serial.println("ERROR: Parse failed.");
				break;
			case nmea_err_null:
				Serial.println("ERROR: Failed to detect NULL pointer.");
				break;
			case nmea_err_unknown:
				Serial.println("ERROR: Unable to parse NMEA sentence type.");
				break;
			case nmea_err_nocsum:
				Serial.println("ERROR: Failed to detect missing checksum.");
				break;
			case nmea_err_badcsum:
				Serial.println("ERROR: Failed to detect invalid checksum.");
				break;
			default:
				Serial.print("TEST FAILURE: Return code 0x");
				Serial.print(err);
				Serial.println(" is not currently handled.");
		}
	}
	return ret;
}
