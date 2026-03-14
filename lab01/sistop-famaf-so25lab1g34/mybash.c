#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

#define GREEN_BOLD "\033[1;32m" // cambia a verde y negrita
#define RED_BOLD "\033[1;31m" // cambia a rojo y negrita
#define NORMAL_STYLE "\033[0m" // vuelve a color y estilo normales

static void show_prompt(void) {
    char *current_working_directory = getcwd(NULL, 0);

    if (current_working_directory != NULL) {
        printf(GREEN_BOLD "%s" NORMAL_STYLE, current_working_directory);
        free(current_working_directory);
    }

    printf(RED_BOLD " mybash> " NORMAL_STYLE);

    fflush(stdout); // asegura que el prompt se vea de inmediato
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        show_prompt();
        pipe = parse_pipeline(input);
        if (pipe != NULL) execute_pipeline(pipe);
        
        /* Hay que salir luego de ejecutar? */
        quit = parser_at_eof(input);
        if (pipe != NULL) pipeline_destroy(pipe);
    }
    input = parser_destroy(input);
    return EXIT_SUCCESS;
}

