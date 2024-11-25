
// Created by ChatGPT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OUTPUT_HEADER "color_table.h"

void trim(char *str) {

    char *start = str;
    char *end;

    while (isspace((unsigned char)*start)) start++;

    if (*start == 0) {
        *str = '\0';
        return;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';

    memmove(str, start, end - start + 2);
}

void toLowerCase(char *str) {
    for (; *str; ++str) {
        *str = tolower((unsigned char)*str);
    }
}

int containsSpace(const char *str) {
    while (*str) {
        if (isspace((unsigned char)*str)) {
            return 1;
        }
        str++;
    }
    return 0;
}

int main() {

    const char *inputFilePath = "/usr/share/X11/rgb.txt";

    FILE *inputFile = fopen(inputFilePath, "r");
    if (!inputFile) {
        perror("Failed to open rgb.txt");
        return 1;
    }

    FILE *outputFile = fopen(OUTPUT_HEADER, "w");
    if (!outputFile) {
        perror("Failed to open output file");
        fclose(inputFile);
        return 1;
    }

    fprintf(outputFile, "#ifndef COLOR_TABLE_H\n");
    fprintf(outputFile, "#define COLOR_TABLE_H\n\n");
    fprintf(outputFile, "#include <stdint.h>\n\n");
    fprintf(outputFile, "typedef struct {\n");
    fprintf(outputFile, "    const char *name;\n");
    fprintf(outputFile, "    uint32_t value;\n");
    fprintf(outputFile, "} ColorName;\n\n");
    fprintf(outputFile, "static const ColorName colorNames[] = {\n");

    char line[256];
    int entryCount = 0;

    while (fgets(line, sizeof(line), inputFile)) {

        char name[128];
        int r, g, b;

        if (sscanf(line, "%d %d %d %[^\n]", &r, &g, &b, name) == 4) {

            trim(name);
            if (containsSpace(name)) continue;
            toLowerCase(name);

            fprintf(outputFile, "    {\"%s\", 0x%02x%02x%02x},\n", name, r, g, b);
            entryCount++;
        }
    }

    fprintf(outputFile, "    {NULL, 0} // End marker\n");
    fprintf(outputFile, "};\n\n");
    fprintf(outputFile, "#endif // COLOR_TABLE_H\n");

    fclose(inputFile);
    fclose(outputFile);

    printf("Generated header file '%s' with %d entries.\n", OUTPUT_HEADER, entryCount);

    return 0;
}

