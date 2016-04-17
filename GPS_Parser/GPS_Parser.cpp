#include "GPS_Parser.hpp"

#include <Arduino.h>

GPS_Parser::GPS_Parser() {
	_timestamp = {
		.year = 0,
		.month = 0,
		.day = 0,
		.hour = 0,
		.minute = 0,
		.second = 0,
		.millisecond = 0,
	};
}

nmea_err_t GPS_Parser::parse(char* nmea, uint8_t len) {
	register nmea_err_t err;

	//  Guard against NULL idiocy
	if (nmea == NULL) {
		return nmea_err_null;
	}

	if (len == 0) {
		len = strlen(nmea);
	}

	err = validate_checksum(nmea, len);
	if (err != nmea_success) {
		return err;
	}

	return delegate_parse(nmea, len);
}

nmea_timestamp_t GPS_Parser::timestamp() {
	return _timestamp;
}

#ifdef ARDUINO
void GPS_Parser::print_info() {
	Serial.print("Timestamp: ");
	Serial.print(_timestamp.year);
	Serial.print('-');
	Serial.print(_timestamp.month);
	Serial.print('-');
	Serial.print(_timestamp.day);
	Serial.print('T');
	Serial.print(_timestamp.hour);
	Serial.print(':');
	Serial.print(_timestamp.minute);
	Serial.print(':');
	Serial.print(_timestamp.second);
	Serial.print('.');
	Serial.print(_timestamp.millisecond);
	Serial.println('Z');
}
#endif

uint8_t GPS_Parser::parse_hex(char c) {
	register uint8_t ret = 0x00;
	if (c >= '0' && c <= '9') {
		ret = c - '0';
	}
	else if (c >= 'A' && c <= 'F') {
		ret = c - 'A' + 10;
	}
	else if (c >= 'a' && c <= 'f') {
		ret = c - 'a' + 10;
	}
	return ret;
}

uint8_t GPS_Parser::parse_hex(char h, char l) {
	register uint8_t ret = 0x00;
	ret |= parse_hex(l);
	ret |= parse_hex(h) << 4;
	return ret;
}

//  Protected

nmea_err_t GPS_Parser::delegate_parse(char* nmea, uint8_t len) {
	if (strstr(nmea, "$GPRMC")) {
		return parse_rmc(nmea, len);
	}
	return nmea_err_unknown;
}

nmea_err_t GPS_Parser::validate_checksum(char* nmea, uint8_t len) {
	char* p = strchr(nmea, '*');
	if (p != NULL) {
		len = strlen(p);
		//  NMEA sentences end in /\*[0-9A-F]{2}\r?\n?\0/
		//  Therefore, if the '*' is less than 3, or more than 5, bytes from the
		//  end, the sentence is corrupt.
		if (len < 3 || len > 5) {
			return nmea_err_badcsum;
		}
		uint8_t csum = parse_hex(p[1], p[2]);
		for (uint8_t idx = 1; nmea[idx] != '*'; ++idx) {
			csum ^= nmea[idx];
		}
		if (csum != 0x00) {
			return nmea_err_badcsum;
		}
		return nmea_success;
	}
	return nmea_err_nocsum;
}

nmea_err_t GPS_Parser::parse_rmc(char* nmea, uint8_t len) {
	register nmea_err_t err;

	//  Seek to the first data field -- time
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_time(nmea);
	if (err != nmea_success) {
		return err;
	}

	return nmea_success;
}

nmea_err_t GPS_Parser::parse_time(char* nmea) {
	register uint8_t tmp;

	tmp  = (nmea[1] - '0') * 10;
	tmp += (nmea[2] - '0');
	if (tmp < 24) {
		_timestamp.hour = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	tmp  = (nmea[3] - '0') * 10;
	tmp += (nmea[4] - '0');
	if (tmp < 60) {
		_timestamp.minute = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	tmp  = (nmea[5] - '0') * 10;
	tmp += (nmea[6] - '0');
	if (tmp < 60) {
		_timestamp.second = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	//  nmea[7] is a decimal point '.'
	tmp  = (nmea[8] - '0') * 100;
	tmp += (nmea[9] - '0') * 10;
	tmp += (nmea[10] - '0');
	if (tmp < 1000) {
		_timestamp.millisecond = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	return nmea_success;
}

