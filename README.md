# CSV Parser Library

## Description

This repository is a multi-platform library (Shared Object (.so) for Linux and Dynamic-Link Library (.dll) for Windows) to parse CSV data. I add an example to parse a CSV file for Windows and Linux. 

## Requirements

 - No requirements

## Download

[Download binaries (Linux and Windows Libraries and examples) from latest github release](https://github.com/mauricelambert/CsvParserLibrary/releases/latest).

## Compile

### Download sources

```bash
git clone https://github.com/mauricelambert/CsvParserLibrary.git
cd CsvParserLibrary
```

#### Library

##### Windows

```bash
gcc parser_library.c -o Windows_CSV_Parser.dll -shared -O5
```

##### Linux

```bash
gcc parser_library.c -shared -o Linux_CSV_Parser.so -O5
```

#### Examples

##### Windows

```bash
gcc WinCsvParser.c -o WindowsParserCSV.exe -O5
WindowsParserCSV.exe
```

##### Linux

```bash
gcc LinuxCsvParser.c -o LinuxParserCSV -O5
./LinuxParserCSV
```

## Usages

### Python

```python
from ctypes import Structure, c_uint, c_char_p, CDLL, POINTER, addressof
from os import name as os_name

class Value(Structure):
    pass

Value._fields_ = [("length", c_uint), ("start", c_char_p), ("next", POINTER(Value))]

class Line(Structure):
    pass

Line._fields_ = [("value", POINTER(Value)), ("next", POINTER(Line))]

class SizedBuffer(Structure):
    _fields_ = [("position", c_uint), ("length", c_uint), ("buffer", c_char_p)]

csv_data = """abc,def,ghi
123,456,789
"foo","bar","f,"",r
"
""".encode('latin1')

if os_name == 'nt':
    parserCSV = CDLL('./Windows_CSV_Parser.dll')
else:
    parserCSV = CDLL('./Linux_CSV_Parser.so')

parserCSV.process.argtypes = (POINTER(SizedBuffer),)
parserCSV.process.restype = POINTER(Line)
line = parserCSV.process(SizedBuffer(0, len(csv_data), c_char_p(csv_data)))

while bool(line):
    value = line.contents.value
    while bool(value):
        print(value.contents.start[:value.contents.length].decode('latin1') + ',', end='')
        value = value.contents.next
    print()
    line = line.contents.next
```

### C

#### Windows

```c
// gcc Windows_example.c -o Windows_example.exe -O5

#include <windows.h>
#include <winbase.h>
#include <windef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

typedef struct Value {
    unsigned int length;
    char* start;
    struct Value* next;
    unsigned int to_free;
} Value;

typedef struct Line {
    Value* value;
    struct Line* next;
} Line;

typedef struct SizedBuffer {
    unsigned int position;
    unsigned int length;
    char* buffer;
} SizedBuffer;

typedef Line* (*FunctionProcessCSV)(SizedBuffer*);

int main () {
    HINSTANCE ParserCSV = LoadLibrary("Windows_CSV_Parser.dll");

    if (NULL == ParserCSV)
    {
        fputs("DLL not found: Windows_CSV_Parser.dll", stderr);
        return EXIT_FAILURE;
    }

    FunctionProcessCSV processCSV = (FunctionProcessCSV)GetProcAddress(ParserCSV, "process");
    if (NULL == processCSV) {
        fputs("DLL Function not found: process", stderr);
        return EXIT_FAILURE;
    }

    SizedBuffer size_buffer = {0, 44, "abc,def,ghi\n123,456,789\n\"foo\",\"bar\",\"\n,\"\",\n\""};
    Line* pointer_line = processCSV(&size_buffer);

    while (pointer_line != NULL) {
        Value* value = pointer_line->value;
        while (value != NULL) {
            write(1, value->start, value->length);
            putchar(',');
            Value* old_value = value;
            value = value->next;
            if (old_value->to_free) {
                free(old_value->start);
            }
            free(old_value);
        }
        puts("");
        Line* old_line = pointer_line;
        pointer_line = pointer_line->next;
        free(old_line);
    }

    FreeLibrary(ParserCSV);
    return 0;
}
```

#### Linux

```c
// gcc Linux_example.c -o Linux_example -O5

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

typedef struct Value {
    unsigned int length;
    char* start;
    struct Value* next;
    unsigned int to_free;
} Value;

typedef struct Line {
    Value* value;
    struct Line* next;
} Line;

typedef struct SizedBuffer {
    unsigned int position;
    unsigned int length;
    char* buffer;
} SizedBuffer;

int main() {
    void *ParserCSV = dlopen("./Linux_CSV_Parser.so", RTLD_LAZY);
    if (NULL == ParserCSV) {
        fputs("Shared library not found: ./Linux_CSV_Parser.so", stderr);
        return EXIT_FAILURE;
    }
    dlerror();

    Line* (*processCSV)(SizedBuffer*) = (Line* (*)(SizedBuffer*)) dlsym(ParserCSV, "process");
    char* error = dlerror();

    if (NULL != error) {
        fputs("DLL Function not found: process", stderr);
        return EXIT_FAILURE;
    }

    SizedBuffer size_buffer = {0, 44, "abc,def,ghi\n123,456,789\n\"foo\",\"bar\",\"\n,\"\",\n\""};
    Line* pointer_line = processCSV(&size_buffer);

    while (pointer_line != NULL) {
        Value* value = pointer_line->value;
        while (value != NULL) {
            write(1, value->start, value->length);
            putchar(',');
            fflush(stdout);
            Value* old_value = value;
            value = value->next;
            if (old_value->to_free) {
                free(old_value->start);
            }
            free(old_value);
        }
        puts("");
        Line* old_line = pointer_line;
        pointer_line = pointer_line->next;
        free(old_line);
    }
        
    dlclose(ParserCSV);
    return 0;
}
```

## Licence

Licensed under the [GPL, version 3](https://www.gnu.org/licenses/).
