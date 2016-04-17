#ifndef __MYRRLYN_GPS_PARSER_H
#define __MYRRLYN_GPS_PARSER_H

#include <stdint.h>

typedef enum : uint8_t {
	nmea_success     = 0x00,
	nmea_err_null    = 0xFF,
	nmea_err_unknown = 0xBF,
	nmea_err_nocsum  = 0xBE,
	nmea_err_badcsum = 0xBD,
} nmea_err_t;

class GPS_Parser {
public:
	GPS_Parser(void);
	nmea_err_t parse(char* nmea, uint8_t len = 0);

#ifdef ARDUINO
	void print_info(void);
#endif

	static uint8_t parse_hex(char c);
	static uint8_t parse_hex(char h, char l);
protected:
	virtual nmea_err_t delegate_parse(char* nmea, uint8_t len = 0);
	static nmea_err_t validate_checksum(char* nmea, uint8_t len);
};

#endif//__MYRRLYN_GPS_PARSER_H
