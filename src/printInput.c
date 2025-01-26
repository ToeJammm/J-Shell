#include <stdio.h>

#define MAX_LENGTH 1000

int main() {
    char text[MAX_LENGTH];

    // Read text from stdin and print to stdout
    while (fgets(text, MAX_LENGTH, stdin) != NULL) {
        printf("printing input: %s", text);
    }

    return 0;
}
