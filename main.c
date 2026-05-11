#include <stdio.h>
#include <string.h>
int main() {
    char input[1024];


    while(1){
        printf( "mini-shell> " );

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = '\0';

        printf("input: %s\n", input);
    }

    return 0;
}