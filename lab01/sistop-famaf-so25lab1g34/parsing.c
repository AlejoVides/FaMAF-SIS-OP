#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

typedef enum {
    DESTROY_PIPELINE_IF_GARBAGE,
    ALWAYS_DESTROY_PIPELINE
} destroy_policy_t;

static void handle_garbage(Parser p, pipeline *result, destroy_policy_t policy);
static void handle_op_background(Parser p, pipeline result);

/*
 * Parsea un comando simple desde el Parser `p`.
 *   p: parser que provee los argumentos y redirecciones.
 *
 * Returns:
 *   - Un comando simple con sus argumentos y redirecciones.
 *   - NULL si hubo error de sintaxis:
 *       * aparece una redirección sin nombre de archivo,
 *       * aparece un comando simple vacío (sin argumentos).
 *
 * Requires:
 *   p != NULL
 *
 * Ensures:
 *   - Si no hubo error, se consumieron tokens hasta encontrar alguno de los
 *     siguientes delimitadores: fin de línea ('\n'), operador de pipe ('|'),
 *     operador de background ('&') o fin de archivo (EOF).
 *   - Si hubo error, el resultado es NULL y no quedan recursos sin liberar.
 */
static scommand parse_scommand(Parser p) {
    assert(p != NULL);

    scommand res = scommand_new();
    bool parsing_active = true;
    char *arg = NULL;
    arg_kind_t type = ARG_NORMAL;
    while (parsing_active && !parser_at_eof(p)) {
        parser_skip_blanks(p);
        arg = parser_next_argument(p, &type);
        if (arg != NULL) {
            if (type == ARG_NORMAL) {
                scommand_push_back(res, arg);
            } else if (type == ARG_OUTPUT) {
                scommand_set_redir_out(res, arg);
            } else {
                scommand_set_redir_in(res, arg);
            }
        } else {
            parsing_active = false;
            if (type == ARG_OUTPUT || type == ARG_INPUT) {
                res = scommand_destroy(res);
            }
        }
    }

    if (res != NULL && scommand_is_empty(res)) {
        res = scommand_destroy(res);
    }

    return res;
}

pipeline parse_pipeline(Parser p) {
    assert(p != NULL);
    assert(!parser_at_eof(p));

    pipeline result = pipeline_new();
    scommand cmd = NULL;

    /* Parsear y agregar todos los comandos simples a la tubería */
    bool syntax_error = false;
    bool was_op_pipe = true;
    cmd = parse_scommand(p);
    syntax_error = (cmd == NULL);
    while (was_op_pipe && !syntax_error) {
        pipeline_push_back(result, cmd);
        if (!parser_at_eof(p)) {
            parser_op_pipe(p, &was_op_pipe);
        } else {
            was_op_pipe = false;  /* Salir del while. EOF no es error. */
        }
        if (was_op_pipe) {
            cmd = parse_scommand(p);
            syntax_error = (cmd == NULL);
        }
    }

    /* Manejar error de sintaxis o buscar un operador de background y basura */
    if (syntax_error) {
        handle_garbage(p, &result, ALWAYS_DESTROY_PIPELINE);
    } else {
        handle_op_background(p, result);
        handle_garbage(p, &result, DESTROY_PIPELINE_IF_GARBAGE);
    }

    return result;
}

static void handle_garbage(Parser p,
                           pipeline *result,
                           destroy_policy_t policy) {
    bool was_garbage = false;
    if (!parser_at_eof(p)) {
        parser_garbage(p, &was_garbage);  /* Consume todo hasta un '\n' */
    }
    if (parser_at_eof(p)) {
        printf("\n");  /* Para mantener prolija la terminal */
    }
    if (policy == ALWAYS_DESTROY_PIPELINE || was_garbage) {
        *result = pipeline_destroy(*result);  /* Hacer cleanup */
        fprintf(stderr, "mybash: syntax error\n");
    }
}

static void handle_op_background(Parser p, pipeline result) {
    bool was_op_background;
    if (!parser_at_eof(p)) {
        parser_op_background(p, &was_op_background);
        if (was_op_background) {
            pipeline_set_wait(result, false);
        }
    }
}

