#include "GPS_Parser.hpp"

#include <Arduino.h>

GPS_Parser::GPS_Parser() :
	_coordinates({
		.latitude  = 0,
		.longitude = 0,
	}),
	_timestamp({
		.year = 0,
		.month = 0,
		.day = 0,
		.hour = 0,
		.minute = 0,
		.second = 0,
		.millisecond = 0,
	}),
	_velocity({
		.speed = 0.0,
		.heading = 0.0,
	}),
	_fix(false) {
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

nmea_coord_t GPS_Parser::coordinates() {
	return _coordinates;
}

nmea_timestamp_t GPS_Parser::timestamp() {
	return _timestamp;
}

nmea_velocity_t GPS_Parser::velocity() {
	return _velocity;
}

bool GPS_Parser::fix() {
	return _fix;
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
	Serial.print("Fix status: ");
	Serial.println(_fix ? "Acquired" : "Void");
	Serial.print("Location: ");
	Serial.print((double)_coordinates.latitude / 100.0);
	Serial.print(", ");
	Serial.println((double)_coordinates.longitude / 100.0);
	Serial.print("Velocity: ");
	Serial.print(_velocity.speed);
	Serial.print(" knots at ");
	Serial.print(_velocity.heading);
	Serial.println(" degrees true");
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

	//  Seek to the second data field -- state
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	if (nmea[1] == 'A') {
		_fix = true;
	}
	else if (nmea[1] == 'V') {
		_fix = false;
	}
	else {
		return nmea_err_baddata;
	}

	//  Seek to the third data field -- latitude
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_coord(&nmea);
	if (err != nmea_success) {
		return err;
	}
	//  Seek to the fifth data field -- longitude
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_coord(&nmea);
	if (err != nmea_success) {
		return err;
	}

	//  Seek to the seventh data field -- speed
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_double(nmea, &_velocity.speed);
	if (err != nmea_success) {
		return err;
	}

	//  Seek to the eighth data field -- heading
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_double(nmea, &_velocity.heading);
	if (err != nmea_success) {
		return err;
	}

	return nmea_success;
}

nmea_err_t GPS_Parser::parse_coord(char** nmea) {
	int32_t tmp = 0;

	//  I cannot explain this for the LIFE of me. This is the minimum delay
	//  needed, on an Atmel AVR ATMega2560, for this parse to succeed.
	//  I have no idea if it's different on different cores or why it's needed
	//  in the first place, but without it, the coordinate parsing fails.
#ifdef ARDUINO
	delayMicroseconds(921);
#endif
	if (*nmea[1] == ',') {
		return nmea_err_baddata;
	}
	for (uint8_t idx = 1; (*nmea)[idx] != ','; ++idx) {
		if ((*nmea)[idx] == '.') {
			continue;
		}
		if ((*nmea)[idx] < '0' || (*nmea)[idx] > '9') {
			return nmea_err_baddata;
		}
		tmp *= (int32_t)10;
		tmp += (int32_t)((*nmea)[idx] - '0');
	}
	//  Seek to the next data field, the hemisphere indicator.
	*nmea = strchr(++(*nmea), ',');
	switch ((*nmea)[1]) {
		case 'N': _coordinates.latitude = tmp; break;
		case 'E': _coordinates.longitude = tmp; break;
		case 'S': _coordinates.latitude = -tmp; break;
		case 'W': _coordinates.longitude = -tmp; break;
		case ',': //  Intentional fallthrough.
		default: return nmea_err_baddata;
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

nmea_err_t GPS_Parser::parse_double(char* nmea, double* store) {
	*store = 0.0;
	register uint8_t fracs = 0;
	register bool in_fracs = false;
	register bool sign = false;
	if (nmea[1] == ',') {
		return nmea_err_baddata;
	}
	for (uint8_t idx = 1; nmea[idx] != ','; ++idx) {
		switch (nmea[idx]) {
			case '.':
				in_fracs = true;
				continue;
			case '-':
				sign = !sign;
				//  Intentional fallthrough
			case '+':
				continue;
		}
		*store *= 10.0;
		*store += (double)(nmea[idx] - '0');
		if (in_fracs) {
			++fracs;
		}
	}
	for (uint8_t idx = 0; idx < fracs; ++idx) {
		*store /= 10.0;
	}
	if (sign) {
		*store *= -1.0;
	}
	return nmea_success;
}
