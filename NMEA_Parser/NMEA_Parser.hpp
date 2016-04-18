#ifndef __MYRRLYN_NMEA_PARSER_H
#define __MYRRLYN_NMEA_PARSER_H

#include <stdint.h>

#define __MYRRLYN_NMEA_PARSER_VERSION "0.1.1"

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
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t millisecond;
} nmea_timestamp_t;

typedef struct {
	int32_t latitude;
	int32_t longitude;
} nmea_coord_t;

typedef struct {
	double speed;
	double heading;
} nmea_velocity_t;

/*
typedef struct {
	uint16_t id;
	uint16_t age;
} nmea_dgps_t;
*/

typedef double nmea_magvar_t;

typedef enum : uint8_t {
	nmea_fix_invalid = 0,
	nmea_fix_normal  = 1,
	nmea_fix_dgps    = 2,
} nmea_fix_quality_t;

class NMEA_Parser {
public:
	NMEA_Parser(void);
	nmea_err_t parse(char* nmea, uint8_t len = 0);

	nmea_timestamp_t timestamp(void);
	nmea_coord_t coordinates(void);
	double altitude(char ref = 's');
	nmea_velocity_t velocity(void);
/*
	nmea_dgps_t dpgs(void);
*/
	double hdop(void);
	nmea_magvar_t magnetic_variation(void);
	uint8_t satellites(void);
	nmea_fix_quality_t fix_quality(void);
	bool fix(void);

#ifdef ARDUINO
	void print_info(void);
#endif

	static uint8_t parse_hex(char c);
	static uint8_t parse_hex(char h, char l);
protected:
	virtual nmea_err_t delegate_parse(char* nmea, uint8_t len = 0);
	virtual nmea_err_t validate_checksum(char* nmea, uint8_t len);

	virtual nmea_err_t parse_gga(char* nmea, uint8_t len);
	virtual nmea_err_t parse_rmc(char* nmea, uint8_t len);

	virtual nmea_err_t parse_coord(char** nmea);
	virtual nmea_err_t parse_date(char* nmea);
	virtual nmea_err_t parse_time(char* nmea);
	virtual nmea_err_t parse_int(char* nmea, uint8_t* store);
	virtual nmea_err_t parse_int(char* nmea, uint16_t* store);
	virtual nmea_err_t parse_double(char* nmea, double* store);

	nmea_timestamp_t _timestamp;
	nmea_coord_t _coordinates;
	double _alt_sea;
	double _alt_wgs;
	nmea_velocity_t _velocity;
/*
	nmea_dgps_t _dgps;
*/
	double _hdop;
	nmea_magvar_t _magvar;
	uint8_t _satellites_visible;
	nmea_fix_quality_t _fix_quality;
	bool _fix;
};

#endif//__MYRRLYN_NMEA_PARSER_H
