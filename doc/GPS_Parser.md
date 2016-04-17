# GPS Parsing

This library provides stateful parsing of NMEA sentences.

## API

The library uses a very simple public API. `parse()` takes a pointer to a
C-style string and returns a status code indicating parse performance. All data
is available through accessor functions.

### Public Types

##### `nmea_err_t`

This is the type returned by the master `parse()` function and all parsing
functions it calls. It indicates the status of a parsing call.

1. `nmea_success` &ndash; Returned when the parse completes successfully. This
value does not guarantee that *all* stored data is correct, as NMEA sentences
each carry a subset of the possible data.
2. `nmea_err_null` &ndash; Returned when the parser is passed a NULL pointer.
3. `nmea_err_unknown` &ndash; Returned when the given string is not an NMEA
sentence for which this library implements a parser.
4. `nmea_err_nocsum` &ndash; Returned when the given string lacks a full
checksum footer.
5. `nmea_err_badcsum` &ndash; Returned when the given string's checksum is
present, but does not match actual contents.
6. `nmea_err_baddata` &ndash; Returned when a data field contains garbage.

##### `nmea_timestamp_t`

Holds a timestamp as stated by NMEA sentences.

Two digit year, month, day, hour, minute, and second values; three-digit
millisecond value. Date values start from 1, time values start from 0. GPS time
does **NOT** recognize leap seconds, so there is no second 60.

### Public Methods

#### Constructor

```cpp
GPS_Parser::GPS_Parser(void);
```

Constructs a new GPS Parser instance and initializes instance data fields to 0.

#### Parser

```cpp
nmea_err_t GPS_Parser::parse(char* nmea, uint8_t len = 0);
```

Public interface for parsing NMEA sentences. Checks for NULL pointers and valid
checksums, then passes the sentence on to the individual type parsers.

This function can return at any time. The return code serves to indicate what
and where the failure occurred.

#### Accessors

These methods return copies of the indicated data field.

##### Timestamp

```cpp
nmea_timestamp_t GPS_Parser::timestamp(void);
```

#### Printer

```cpp
void GPS_Parser::print_info(void);
```

Prints the full contents of the parser's current storage to main Serial line.

This function is only available under the Arduino framework. This way, the
library can still be used on other platforms.

Future possibilities:

- Give this function a parameter for a specific Serial device to target when
printing.
- Make versions for other frameworks, such as UNIX's standard streams.

### Protected Methods

These functions are all marked as virtual so that subclasses can modify them if
needed, and have untouched functions call the correct version.

#### Parse Multiplexer

```cpp
virtual nmea_err_t delegate_parse(char* nmea, uint8_t len = 0);
```

Each NMEA sentence type contains different information fields in different
orders. A single master parse routine is, obviously, incompatible with this, so
the master parser function delegates the sentence to an appropriate parser.

This function does nothing but select an appropriate parsing function based on
the sentence it is given, pass the sentence to that function, and return its
error code.

To add more parsers to this function, simply override it in a subclass and call
the base class to use the base parsers.

Example Override:

```cpp
class Custom_Parser : public GPS_Parser {
protected:
    nmea_err_t delegate_parse(char* nmea, uint8_t len);
private:
    nmea_err_t parse_some_type(char* nmea, uint8_t len);
};

nmea_err_t Custom_Parser::parse_come_type(char* nmea, uint8_t len) {
    //  do your magic here
}

nmea_err_t Custom_Parser::delegate_parse(char* nmea, uint8_t len) {
    //  Check if Custom_Parser knows how to handle the sentence
    if (strstr(nmea, "$GPwhatever")) {
        return parse_some_type(nmea, len);
    }
    //  Give up and let the parent handle it
    else {
        return GPS_Parser::delegate_parse(nmea, len);
    }
}

GPS_Parser demoer;
Custom_Parser example;

demoer.parse("$GPRMC,data*cs");
//  - GPS_Parser::delegate_parse
//  -- GPS_Parser::parse_rmc
demoer.parse("$GPwhatever,data*cs");
//  - GPS_Parser::delegate_parse
//  FAILS
example.parse("$GPRMC,data*cs");
//  - Custom_Parser::delegate_parse
//  -- GPS_Parser::delegate_parse
//  --- GPS_Parser::parse_rmc
example.parse("$GPwhatever,data*cs");
//  - Custom_Parser::delegate_parse
//  -- Custom_Parser::parse_some_type
```

#### Checksum Validator

```cpp
virtual  nmea_err_t validate_checksum(char* nmea, uint8_t len);
```

Validates an NMEA sentence against its checksum.

Returns `nmea_err_nocsum` if the string does not include a checksum. Returns
`nmea_err_badcsum` if the string includes, but does not match, a checksum.

A checksum is an 8-bit hexadecimal number calculated by XOR-ing together all
payload characters (those between `$` and `*` in the string).

This is marked as virtual so that subclasses can alter it, such as permitting
sentences with no checksum to be permitted.

#### Sentence Parsers

The sentence-type parsers will fail if any of their internal sections fail.

##### RMC Parser

```cpp
virtual nmea_err_t GPS_Parser::parse_rmc(char* nmea, uint8_t len);
```

Parses an RMC-type NMEA sentence and stores the payload in the instance fields.

#### Data Parsers

The data-field parsers will fail if their given pointer does not target the `,`
beginning an appropriate data field, or if the contained data is invalid.

##### Timestamp Parsers

```cpp
virtual nmea_err_t GPS_Parser::parse_date(char* nmea);
virtual nmea_err_t GPS_Parser::parse_time(char* nmea);
```

Reads the timestamp out of an NMEA fragment and stores the data.

### Public Static Methods

The static methods are stateless and do not need an NMEA sentence or parser
instance to function.

#### Hexadecimal Digit Parsers

The checksum footer of NMEA sentences is a two-digit hexadecimal number. The
hex parser functions are provided as public for client convenience.

Characters `0` through `9` return those numbers. Characters `A` through `F` and
`a` through `f` return 10 through 15. Other characters return 0.

```cpp
static uint8_t GPS_Parser::parse_hex(char c);
```

Parses a single ASCII character as a hexadecimal digit and returns the number it
represents.

```cpp
static uint8_t GPS_Parser::parse_hex(char h, char l);
```

Parses two ASCII characters as hexadecimal digits and returns the number they
represent. The first argument, `h`, is the high digit and the second, `l`, is
the low digit.

# Appendix

## NMEA Sentence Structures

NMEA sentences consist of an identifying header, data fields, and a checksum
footer. Fields are separated by commas. Sentences always begin with a `$` before
the header and a `*` before the two-digit checksum.

Modules may choose to end the sentences in `\r`, `\n` or `\r\n`.

### RMC

Recommended Minimum Coordinates.

- `$GPRMC` &ndash; Header and identifier.
- `/[0-9]{6}/` &ndash; Time in UTC. GPS time is offset from UTC, and the GPS
signals include this offset for the receiver to calculate UTC time.
