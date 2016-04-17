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
5. `nmea_err_nocsum` &ndash; Returned when the given string lacks a full
checksum footer.
4. `nmea_err_badcsum` &ndash; Returned when the given string's checksum is
present, but does not match actual contents.

### Public Instance Methods

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

### Protected Instance Methods

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

### Protected Static Methods

#### Checksum Validator

```cpp
static nmea_err_t validate_checksum(char* nmea, uint8_t len);
```

Validates an NMEA sentence against its checksum.

Returns `nmea_err_nocsum` if the string does not include a checksum. Returns
`nmea_err_badcsum` if the string includes, but does not match, a checksum.

A checksum is an 8-bit hexadecimal number calculated by XOR-ing together all
payload characters (those between `$` and `*` in the string).

This is marked as protected so that subclasses can alter it, such as permitting
sentences with no checksum to be permitted.
