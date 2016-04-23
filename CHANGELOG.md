# Changelog

### v0.5.1

- Fix bug in timestamp parsing. The old code used an 8-bit unsigned temp, so the
millisecond values would wrap every 256. The new code uses a 16-bit unsigned
temp, so all possible values will be handled.

### v0.5.0

- Add ability to construct from a saved state
- Fix the `nmea_coord_u` bug. `nmea_coord_t` structs now use `nmea_coord_u`
instead of `int32_t`. Member access is `loc.latitude.i` for latitude as integer,
`loc.longitude.f` for longitude as float, etc.

### v0.4.0

- Change from `double` to `float` (no effect on AVR or SAM)
- Add `store()` and `load()` API for bulk data transfer
- Updated documentation

### v0.3.0

- Add a GLL sentence parser
- Decouple next-character string traversal from incrementing-pointer memory
traversal

### v0.2.0

- Add a GGA sentence parser

### v0.1.1

- Use the proper architecture type for the Due (AtmelSAM, not AtmelAVR)
- Add this changelog

### v0.1.0

- Create primary public API: `parse(char* sentence)`
- Implement RMC sentence parser
- Implement specific data parsers
- Set PlatformIO to use an Arduino-IDE-like structure
- Set PlatformIO to use the two boards I have on hand for testing
