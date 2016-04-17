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
	nmea_err_nofix   = 0x9E,
} nmea_err_t;

typedef struct {
	int32_t latitude;
	int32_t longitude;
} nmea_coord_t;

typedef struct {
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t millisecond;
} nmea_timestamp_t;

typedef double nmea_magvar_t;

typedef struct {
	double speed;
	double heading;
} nmea_velocity_t;

class GPS_Parser {
public:
	GPS_Parser(void);
	nmea_err_t parse(char* nmea, uint8_t len = 0);

	nmea_coord_t coordinates(void);
	nmea_timestamp_t timestamp(void);
	nmea_velocity_t velocity(void);
	nmea_magvar_t magnetic_variation(void);
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

	virtual nmea_err_t parse_coord(char** nmea);
	virtual nmea_err_t parse_date(char* nmea);
	virtual nmea_err_t parse_time(char* nmea);
	virtual nmea_err_t parse_double(char* nmea, double* store);

	nmea_coord_t _coordinates;
	nmea_timestamp_t _timestamp;
	nmea_velocity_t _velocity;
	nmea_magvar_t _magvar;
	bool _fix;
};

#endif//__MYRRLYN_GPS_PARSER_H
