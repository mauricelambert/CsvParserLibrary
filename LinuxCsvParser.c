/*
    Copyright (C) 2023  Maurice Lambert
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// gcc LinuxCsvParser.c -o LinuxParserCSV -O5


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

    FILE* csvfile = fopen("test.csv", "r");

    if (NULL == csvfile) {
        fputs("File not found: test.csv", stderr);
        return EXIT_FAILURE;
    }

    fseek(csvfile, 0, SEEK_END);
    unsigned int length = ftell(csvfile);
    fseek(csvfile, 0, SEEK_SET);
    char* buffer = malloc(length);

    if (NULL == buffer) {
        fclose(csvfile);
        fputs("Memory error.", stderr);
        return EXIT_FAILURE;
    }

    fread(buffer, 1, length, csvfile);
    fclose(csvfile);

    SizedBuffer size_buffer = {0, length, buffer};
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
    free(buffer);
    return 0;
}
