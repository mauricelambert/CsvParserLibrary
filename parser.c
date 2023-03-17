// Quote = "
// Value = (&Quote ([^"] / "")* &Quote / [^,\nEOF]*)
// Line = (Value ,)* Value [\nEOF]

// gcc shared.c -o Windows_CSV_Parser.exe -shared -O5
// gcc -O5 parser.c -shared -o Linux_CSV_Parser

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

void remove_double_quote(Value* value, unsigned int counter) {
	unsigned int value_length = value->length - counter;
	char* value_string = malloc(value_length);
	char* position_string = value_string;
	for (int index = 0; index < value->length; index += 1) {
		if (value->start[index] == '"') {
			counter -= 1;
			index += 1;
		}
		position_string[0] = value->start[index];
		position_string += 1;
	}

	value->start = value_string;
}

int is_character (SizedBuffer* buffer, char character) {
	if (buffer->position >= buffer->length) {
		return 1;
	}

	if (buffer->buffer[0] == character) {
		buffer->position += 1;
		buffer->buffer += 1;
		return 0;
	}
	return 1;
}

int get_simple_value(SizedBuffer* buffer, Value* value) {
	value->start = buffer->buffer;
	int return_code = 0;

	while (buffer->position < buffer->length && buffer->buffer[0] != ',' && buffer->buffer[0] != '\n') { // [^EOF,\n]*
		buffer->position += 1;
		buffer->buffer += 1;
		value->length += 1;
	}

	if (buffer->position < buffer->length) {                                                             // [,\n]
		if (buffer->buffer[0] == ',') {
			return_code = 1;
		} else if (buffer->buffer[0] == '\n') {
			return_code = 2;
		}

		buffer->position += 1;
		buffer->buffer += 1;
	}

	return return_code;
}

int get_quoted_value(SizedBuffer* buffer, Value* value) {
	value->start = buffer->buffer;
	unsigned int double_quote_number = 0;

	while (buffer->position < buffer->length) {                                        // [^EOF]*

		if (buffer->buffer[0] == '"') {                                                // [^"]
			if (buffer->position + 1 < buffer->length && buffer->buffer[1] == '"') {   // OR ""
				double_quote_number += 1;
				buffer->position += 2;
				buffer->buffer += 2;
				value->length += 2;
				continue;
			}
			break;
		}

		buffer->position += 1;
		buffer->buffer += 1;
		value->length += 1;
	}

	if (buffer->position < buffer->length) {
		buffer->position += 1;
		buffer->buffer += 1;

		if (double_quote_number) {
			remove_double_quote(value, double_quote_number);
		}
		return 0;
	}

	value->start = NULL;
	value->length = 0;
	return 1;
}

Line* new_line(Line* pointer_line) {
	Value* value = malloc(sizeof(Value));
	value->length = 0;
	value->next = NULL;
	Line* line = malloc(sizeof(Line));
	line->value = value;
	line->next = NULL;
	pointer_line->next = line;
	return line;
}

Value* new_value(Value* pointer_value) {
	Value* value = malloc(sizeof(Value));
	value->length = 0;
	value->next = NULL;
	pointer_value->next = value;
	return value;
}

extern Line* process(SizedBuffer* buffer) {
	Line* first_line = malloc(sizeof(Line));
	Value* pointer_value = malloc(sizeof(Value));
	pointer_value->length = 0;
	pointer_value->next = NULL;
	first_line->value = pointer_value;
	Line* pointer_line = first_line;

	while (buffer->position < buffer->length) {
		int position = buffer->position;

		if (
			is_character(buffer, '"') == 0 &&                // &Quote
			get_quoted_value(buffer, pointer_value) == 0     // ([^"EOF] / "")* &Quote
		) {
			if (is_character(buffer, ',') == 0) {            // ,
				pointer_value = new_value(pointer_value);
				continue;
			} else if (is_character(buffer, '\n') == 0) {    // OR \n
				pointer_line = new_line(pointer_line);
				pointer_value = pointer_line->value;
				continue;
			} else if (buffer->position >= buffer->length) { // OR EOF
				break;
			}
		}

		buffer->buffer -= (buffer->position - position);
		buffer->position = position;
		int stop_type = get_simple_value(buffer, pointer_value);

		if (stop_type == 2) {
			pointer_line = new_line(pointer_line);
			pointer_value = pointer_line->value;
			continue;
		} else if (stop_type == 1) {
			pointer_value = new_value(pointer_value);
			continue;
		} else {
			break;
		}

		return NULL;                                         // Error
	}

	return first_line;
}

int main () {
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
    Line* pointer_line = process(&size_buffer);

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

    free(buffer);
	fclose(csvfile);
	return 0;
}
