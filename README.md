# `GPS_Parser`

This library provides parsing functions that operate on NMEA standard sentences.
It is designed to be suitable for use standalone or bundled into a GPS driver.

The parser takes in a pointer to the head of a C-style NULL-terminated string.
It does not return the parsed data, but stores it in instance state. The parsed
data is accessible through accessor methods.

## History

I originally wrote this as part of my Senior Design Project at Trine University.
I was dissatisfied with the main NMEA parser I could find for Arduino and
chose to implement my own. Hopefully, this will be well received by the
community. I am happy to include improvements and extensions as well.
