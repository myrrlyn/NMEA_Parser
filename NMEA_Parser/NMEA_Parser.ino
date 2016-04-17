#include "NMEA_Parser.hpp"

NMEA_Parser demo;
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

	//  Expect NULL pointer detection and abort
	err = demo.parse(NULL);
	result &= test(err, nmea_err_null);

	//  Expect absent checksum and abort
	err = demo.parse("$GPRMC,");
	result &= test(err, nmea_err_nocsum);

	//  Expect malformed checksum and abort
	err = demo.parse("$GPRMC,*");
	result &= test(err, nmea_err_badcsum);

	err = demo.parse("$GPRMC,*0");
	result &= test(err, nmea_err_badcsum);

	err = demo.parse("$GPRMC,*00");
	result &= test(err, nmea_err_badcsum);

	//  Expect missing data and abort
	err = demo.parse("$GPRMC,203826.123,V,,,,,,,160416,,,D*4E");
	result &= test(err, nmea_err_nofix);

	demo.print_info();

	//  Expect success with all required fields
	err = demo.parse("$GPRMC,203826.123,A,4137.8868,N,08500.4129,W,1.02,297.04,160416,,,D*77");
	result &= test(err, nmea_success);

	demo.print_info();

	//  Expect success with optional fields
	err = demo.parse("$GPRMC,203826.123,A,4137.8868,N,08500.4129,W,1.02,297.04,160416,0.01,W,D*3F");
	result &= test(err, nmea_success);

	demo.print_info();

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
			case nmea_err_baddata:
				Serial.println("SUCCESS: Detected missing data fields.");
				break;
			case nmea_err_nofix:
				Serial.println("SUCCESS: Detected lack of satellite fix.");
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
			case nmea_err_baddata:
				Serial.println("ERROR: Failed to detect missing data fields.");
				break;
			case nmea_err_nofix:
				Serial.println("ERROR: Failed to detect lack of satellite fix");
				break;
			default:
				Serial.print("TEST FAILURE: Return code 0x");
				Serial.print(err);
				Serial.println(" is not currently handled.");
		}
	}
	return ret;
}
