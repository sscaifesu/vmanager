#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    
    if (argc > 0) {
        const char *command = argv[0];
        printf("command = '%s'\n", command);
        
        if (strcmp(command, "list") == 0) {
            printf("Matched 'list' command\n");
            bool verbose = false;
            
            for (int i = 1; i < argc; i++) {
                printf("Checking argv[%d] = '%s'\n", i, argv[i]);
                if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
                    verbose = true;
                    printf("Set verbose = true\n");
                    break;
                }
            }
            
            printf("Final verbose = %s\n", verbose ? "true" : "false");
        }
    }
    
    return 0;
}
