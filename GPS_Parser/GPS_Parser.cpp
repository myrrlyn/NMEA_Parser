#include "GPS_Parser.hpp"

#include <Arduino.h>

GPS_Parser::GPS_Parser() {
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

#ifdef ARDUINO
void GPS_Parser::print_info() {
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
