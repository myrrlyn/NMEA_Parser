#ifndef __MYRRLYN_GPS_PARSER_H
#define __MYRRLYN_GPS_PARSER_H

#include <stdint.h>

typedef enum : uint8_t {
	nmea_success     = 0x00,
	nmea_err_null    = 0xFF,
	nmea_err_unknown = 0xBF,
	nmea_err_nocsum  = 0xBE,
	nmea_err_badcsum = 0xBD,
	nmea_err_baddata = 0x9F,
} nmea_err_t;

typedef struct {
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t millisecond;
} nmea_timestamp_t;

class GPS_Parser {
public:
	GPS_Parser(void);
	nmea_err_t parse(char* nmea, uint8_t len = 0);

	nmea_timestamp_t timestamp(void);
	bool fix(void);

#ifdef ARDUINO
	void print_info(void);
#endif

	static uint8_t parse_hex(char c);
	static uint8_t parse_hex(char h, char l);
protected:
	virtual nmea_err_t delegate_parse(char* nmea, uint8_t len = 0);
	virtual nmea_err_t validate_checksum(char* nmea, uint8_t len);

	virtual nmea_err_t parse_rmc(char* nmea, uint8_t len);

	virtual nmea_err_t parse_time(char* nmea);

	nmea_timestamp_t _timestamp;
	bool _fix;
};

#endif//__MYRRLYN_GPS_PARSER_H
