# `NMEA_Parser`

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

## Overview

This library aims to provide a simple, stateful parser of NMEA sentences. Each
instance of the parser holds a suite of variables that can be determined from
NMEA sentence payloads. The parser functions update instance variables that are
found in a sentence. This means that, for instance, asking the same parser to
process an RMC sentence generated in one location, and then a GLL sentence from
another, will cause *inconsistent state*. GLL sentences only carry location and
time, while RMC carry much more information. After this occurs, the stored data
will be internally inconsistent.

The reason for this choice is that the library is designed for use on embedded
systems which lack memory management. As such, it is infeasible for the parsers
to return structures holding only the data retrieved from the given sentence.

Instances of `NMEA_Parser` store all the data that can be retrieved from known
parsers, and each parse will only update the fields to which it applies.

### Features

#### Simple API

Any NMEA sentence that the library knows is accepted as an input to `parse()`.

```cpp
NMEA_Parser parser;
parser.parse("$GPRMC...");
parser.parse("$GPGGA...");
parser.parse("$GPGLL...");
```

The library will handle validation, detection, and processing internally.

The parsed data is accessible via value-returning accessors.

```cpp
nmea_coord_t location = parser.coordinates();
```

The library also supports bulk copy in and out of instances via `store()` and
`load()`. This can be used for persistent storage or giving the data to another
system, such as a communication node.

```cpp
NMEA_Parser parser;
nmea_storage_t backup;
parser.store(&backup);
//  backup now contains a complete copy of parser's data.
NMEA_Parser clone;
clone.load(&backup);
//  clone is now a duplicate of parser
```

#### Semantic Types

Custom types are used to package and differentiate information from the NMEA
sentences. The strong type information permits accessors to be used without
casting in any function, from this library or external, that knows to accept
them. All the custom types are documented for public use.

#### Re-Entrant Functions

All functions have fully self-contained state and are suitable for use in a
multithreaded environment. This promise only holds when operating on separate
instances, however. The parsers store information as they process it, and are
neither atomic nor guaranteed to always reach every field. This library can be
used in multiple threads or in interrupts, so long as the user takes care that
there is not competition to use the same instance.

#### Modularity

Specific field parsers are in their own functions, so sentence parsers only have
to walk the string and call the appropriate parsers or do some simple checks on
their own.

All component functions, including specific and sentence parsers, are marked
virtual so that subclasses can only modify the functions they choose, and any
functions that call them will use the correct version. The most useful instance
of this is the `delegate_parse()` function, which is called by the main
`parse()` interface. To add new parse types in a subclass, simply override
`delegate_parse()` to call the new parsers and pass all other sentences to the
base `delegate_parse()`. The `parse()` interface will call the appropriate
delegate, which will either match the sentence to a parser or give up, with a
minimum of effort and verbosity in the subclass.

This also allows subclasses to alter the behavior of any sentence or field
parser to fit specific environments and use cases without having to rewrite
significant parts of the library. As long as the interface is matched, the
individual components are easily replaceable.
