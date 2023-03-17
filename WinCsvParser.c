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

// gcc WinCsvParser.c -o ParserCSV.exe -O5

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
        puts("DLL not found: Windows_CSV_Parser.dll");
        return EXIT_FAILURE;
    }

    FunctionProcessCSV processCSV = (FunctionProcessCSV)GetProcAddress(ParserCSV, "process");
    if (NULL == processCSV) {
        puts("DLL Function not found: process");
        return EXIT_FAILURE;
    }

    FILE* csvfile = fopen("test.csv", "r");

    if (NULL == csvfile) {
        puts("File not found: test.csv");
        return EXIT_FAILURE;
    }

    fseek(csvfile, 0, SEEK_END);
    unsigned int length = ftell(csvfile) - 3;
    fseek(csvfile, 0, SEEK_SET);
    char* buffer = malloc(length);

    if (NULL == buffer) {
        fclose(csvfile);
        puts("Memory error.");
        return EXIT_FAILURE;
    }

    fread(buffer, 1, length, csvfile);

    SizedBuffer size_buffer = {0, length, buffer};
    Line* pointer_line = processCSV(&size_buffer);

    while (pointer_line != NULL) {
        Value* value = pointer_line->value;
        while (value != NULL) {
            write(1, value->start, value->length);
            putchar(',');
            Value* old_value = value;
            value = value->next;
            free(old_value);
        }
        printf("\r\n");
        Line* old_line = pointer_line;
        pointer_line = pointer_line->next;
        free(old_line);
    }

    FreeLibrary(ParserCSV);
    free(buffer);
    fclose(csvfile);
    return 0;
}