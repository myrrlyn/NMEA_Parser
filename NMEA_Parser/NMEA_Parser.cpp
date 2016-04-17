#include "NMEA_Parser.hpp"

#include <Arduino.h>

NMEA_Parser::NMEA_Parser() :
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
	_hdop(0.0),
	_magvar(0.0),
	_satellites_visible(0),
	_fix_quality(nmea_fix_invalid),
	_fix(false) {
}

nmea_err_t NMEA_Parser::parse(char* nmea, uint8_t len) {
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

nmea_coord_t NMEA_Parser::coordinates() {
	return _coordinates;
}

nmea_timestamp_t NMEA_Parser::timestamp() {
	return _timestamp;
}

nmea_velocity_t NMEA_Parser::velocity() {
	return _velocity;
}

double NMEA_Parser::hdop() {
	return _hdop;
}

nmea_magvar_t NMEA_Parser::magnetic_variation() {
	return _magvar;
}

uint8_t NMEA_Parser::satellites() {
	return _satellites_visible;
}

nmea_fix_quality_t NMEA_Parser::fix_quality() {
	return _fix_quality;
}

bool NMEA_Parser::fix() {
	return _fix;
}

#ifdef ARDUINO
void NMEA_Parser::print_info() {
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
	Serial.print("Satellites in View: ");
	Serial.println(_satellites_visible);
	Serial.print("Fix Status: ");
	Serial.println(_fix ? "Acquired" : "Void");
	Serial.print("Fix Quality: ");
	Serial.println((uint8_t)_fix_quality);
	Serial.print("Location: ");
	Serial.print((double)_coordinates.latitude / 100.0);
	Serial.print(", ");
	Serial.println((double)_coordinates.longitude / 100.0);
	Serial.print("Horizontal Dilution of Precision: ");
	Serial.println(_hdop);
	Serial.print("Velocity: ");
	Serial.print(_velocity.speed);
	Serial.print(" knots at ");
	Serial.print(_velocity.heading);
	Serial.println(" degrees true");
	Serial.print("Magnetic variation: ");
	Serial.println(_magvar);
}
#endif

uint8_t NMEA_Parser::parse_hex(char c) {
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

uint8_t NMEA_Parser::parse_hex(char h, char l) {
	register uint8_t ret = 0x00;
	ret |= parse_hex(l);
	ret |= parse_hex(h) << 4;
	return ret;
}

//  Protected

nmea_err_t NMEA_Parser::delegate_parse(char* nmea, uint8_t len) {
	if (strstr(nmea, "$GPGGA")) {
		return parse_gga(nmea, len);
	}
	if (strstr(nmea, "$GPRMC")) {
		return parse_rmc(nmea, len);
	}
	return nmea_err_unknown;
}

nmea_err_t NMEA_Parser::validate_checksum(char* nmea, uint8_t len) {
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

nmea_err_t NMEA_Parser::parse_gga(char* nmea, uint8_t len) {
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

	//  Seek to the second data field -- latitude
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_coord(&nmea);
	if (err != nmea_success) {
		return err;
	}

	//  Seek to the fourth data field -- longitude
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_coord(&nmea);
	if (err != nmea_success) {
		return err;
	}

	//  Seek to the sixth data field -- fix quality
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	switch (nmea[1]) {
		case '0':
		case '1':
		case '2':
			//  Fallthroughs are intentional
			_fix_quality = (nmea_fix_quality_t)(nmea[1] - '0');
			break;
		case ',':
		default:
			return nmea_err_baddata;
	}

	//  Seek to the seventh data field -- satellite count
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_int(nmea, &_satellites_visible);
	if (err != nmea_success) {
		return err;
	}

	//  Seek to the eighth data field -- HDOP
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_double(nmea, &_hdop);
	if (err != nmea_success) {
		return err;
	}

	Serial.println(nmea);

	return nmea_success;
}

nmea_err_t NMEA_Parser::parse_rmc(char* nmea, uint8_t len) {
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
		//  Fast-forward to date
		for (uint8_t idx = 0; idx < 7; ++idx) {
			nmea = strchr(++nmea, ',');
		}
		err = parse_date(nmea);
		if (err != nmea_success) {
			return err;
		}
		//  Now that we've parsed the date, give up and report missing data
		return nmea_err_nofix;
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
	/*
	if (_velocity.heading >= 360.0) {
		_velocity.heading = 0.0;
		return nmea_err_baddata;
	}
	*/

	//  Seek to the ninth field -- date
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_err_baddata;
	}
	err = parse_date(nmea);
	if (err != nmea_success) {
		return err;
	}

	//  Seek to the tenth field -- magnetic variation
	nmea = strchr(++nmea, ',');
	if (nmea == NULL) {
		return nmea_success;
	}
	err = parse_double(nmea, &_magvar);
	if (err == nmea_success) {
		nmea = strchr(++nmea, ',');
		if (nmea == NULL) {
			return nmea_success;
		}
		if (nmea[1] == 'W') {
			_magvar *= -1;
		}
	}

	return nmea_success;
}

nmea_err_t NMEA_Parser::parse_coord(char** nmea) {
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

nmea_err_t NMEA_Parser::parse_date(char* nmea) {
	register uint8_t tmp;

	tmp  = (nmea[1] - '0') * 10;
	tmp += (nmea[2] - '0');
	if (tmp > 0 && tmp < 32) {
		_timestamp.day = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	tmp  = (nmea[3] - '0') * 10;
	tmp += (nmea[4] - '0');
	if (tmp > 0 && tmp < 13) {
		_timestamp.month = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	tmp  = (nmea[5] - '0') * 10;
	tmp += (nmea[6] - '0');
	if (tmp > 0 && tmp < 100) {
		_timestamp.year = tmp;
	}
	else {
		return nmea_err_baddata;
	}

	return nmea_success;
}

nmea_err_t NMEA_Parser::parse_time(char* nmea) {
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

nmea_err_t NMEA_Parser::parse_int(char* nmea, uint8_t* store) {
	*store = 0;
	if (nmea[1] == ',') {
		return nmea_err_baddata;
	}
	for (uint8_t idx = 1; nmea[idx] >= '0' && nmea[idx] <= '9'; ++idx) {
		*store *= 10;
		*store += (nmea[idx] - '0');
	}
	return nmea_success;
}

nmea_err_t NMEA_Parser::parse_double(char* nmea, double* store) {
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
