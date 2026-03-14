#include <assert.h>
#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "strextra.h"

struct scommand_s {
    GQueue *words;
    char *redir_out;
    char *redir_in;
};

struct pipeline_s {
    GQueue *scommands;
    bool wait;
};

static char *scommand_set_redir(char *redir, char *filename);
static char *str_append(char *s1, char *s2);

/* COMANDOS SIMPLES */

scommand scommand_new(void) {
    scommand result = malloc(sizeof(struct scommand_s));

    assert(result != NULL);

    result->words = g_queue_new();
    result->redir_out = NULL;
    result->redir_in = NULL;

    assert(scommand_is_empty(result));
    assert(scommand_get_redir_in(result) == NULL);
    assert(scommand_get_redir_out(result) == NULL);

    return result;
}

scommand scommand_destroy(scommand self) {
    assert(self != NULL);

    /* Eliminar cada palabra del comando simple */
    while (!scommand_is_empty(self)) {
        scommand_pop_front(self);
    }

    /* Liberar cola */
    g_queue_free(self->words);

    /* Liberar redirecciones */
    free(self->redir_out);
    free(self->redir_in);

    /* Liberar estructura */
    free(self);

    return NULL;
}

/* Modificadores */

void scommand_push_back(scommand self, char *argument) {
    assert(self != NULL);
    assert(argument != NULL);

    g_queue_push_tail(self->words, (gpointer) argument);

    assert(!scommand_is_empty(self));
}

void scommand_pop_front(scommand self) {
    assert(self != NULL);
    assert(!scommand_is_empty(self));

    char *head = (char *) g_queue_pop_head(self->words);
    free(head);
}

/*
 * Función auxiliar interna para asignar un redirector.
 *   redir: puntero al redirector actual (puede ser NULL)
 *   filename: nueva cadena a asignar (puede ser NULL)
 * Retorna: el puntero a la nueva cadena (filename), después de liberar
 *   la memoria del redirector previo si existía.
 * Nota: el TAD se apropia de la referencia filename.
 */
static char *scommand_set_redir(char *redir, char *filename) {
    if (redir != NULL) {
        free(redir);
    }
    return filename;
}

void scommand_set_redir_in(scommand self, char *filename) {
    assert(self != NULL);

    self->redir_in = scommand_set_redir(self->redir_in, filename);
}

void scommand_set_redir_out(scommand self, char *filename) {
    assert(self != NULL);

    self->redir_out = scommand_set_redir(self->redir_out, filename);
}

/* Proyectores */

bool scommand_is_empty(const scommand self) {
    assert(self != NULL);

    return g_queue_is_empty(self->words) != FALSE;
}

unsigned int scommand_length(const scommand self) {
    assert(self != NULL);

    unsigned int length = g_queue_get_length(self->words);

    assert((length == 0) == scommand_is_empty(self));

    return length;
}

char *scommand_front(const scommand self) {
    assert(self != NULL);
    assert(!scommand_is_empty(self));

    char *result = (char *) g_queue_peek_head(self->words);

    assert(result != NULL);

    return result;
}

char *scommand_get_redir_in(const scommand self) {
    assert(self != NULL);

    return self->redir_in;
}

char *scommand_get_redir_out(const scommand self) {
    assert(self != NULL);

    return self->redir_out;
}

/*
 * str_append:
 *   Concatena la cadena `s2` al final de `s1`, liberando `s1` original.
 *   - s1: cadena acumulada previamente (debe ser no NULL).
 *   - s2: cadena a agregar al final (debe ser no NULL).
 * 
 *   Devuelve un puntero a la nueva cadena concatenada si la operación
 *   tiene éxito. Si falla la asignación de memoria, devuelve `s1`
 *   sin modificar.
 */
static char *str_append(char *s1, char *s2) {
    assert(s1 != NULL);
    assert(s2 != NULL);

    char *temp = strmerge(s1, s2);

    if (temp != NULL) {
        free(s1);
        s1 = temp;
    }

    return s1;
}

char *scommand_to_string(const scommand self) {
    assert(self != NULL);

    char *result = strdup(""); // string acumulado

    /* Palabras */
    unsigned int length = scommand_length(self);
    for (unsigned int i = 0; i < length; i++) {
        if (i > 0) {
            result = str_append(result, " ");
        }
        char *word = (char *) g_queue_peek_nth(self->words, i);
        result = str_append(result, word);
    }

    /* Redirector de salida */
    if (scommand_get_redir_out(self) != NULL) {
        result = str_append(result, " > ");
        result = str_append(result, scommand_get_redir_out(self));
    }

    /* Redirector de entrada */
    if (scommand_get_redir_in(self) != NULL) {
        result = str_append(result, " < ");
        result = str_append(result, scommand_get_redir_in(self));
    }

    assert(scommand_is_empty(self) ||
           scommand_get_redir_in(self) == NULL ||
           scommand_get_redir_out(self) == NULL ||
           strlen(result) > 0);

    return result;
}

/* TUBERÍAS */

pipeline pipeline_new(void) {
    pipeline result = malloc(sizeof(struct pipeline_s));

    assert(result != NULL);

    result->scommands = g_queue_new();
    result->wait = true;

    assert(pipeline_is_empty(result));
    assert(pipeline_get_wait(result));

    return result;
}

pipeline pipeline_destroy(pipeline self) {
    assert(self != NULL);

    /* Eliminar cada comando simple de la tubería */
    while (!pipeline_is_empty(self)) {
        pipeline_pop_front(self);
    }

    /* Liberar cola */
    g_queue_free(self->scommands);

    /* Liberar estructura */
    free(self);

    return NULL;
}

/* Modificadores */

void pipeline_push_back(pipeline self, scommand sc) {
    assert(self != NULL);
    assert(sc != NULL);

    g_queue_push_tail(self->scommands, (gpointer) sc);

    assert(!pipeline_is_empty(self));
}

void pipeline_pop_front(pipeline self) {
    assert(self != NULL);
    assert(!pipeline_is_empty(self));

    scommand head = (scommand) g_queue_pop_head(self->scommands);
    scommand_destroy(head);
}

void pipeline_set_wait(pipeline self, const bool w) {
    assert(self != NULL);

    self->wait = w;
}

/* Proyectores */

bool pipeline_is_empty(const pipeline self) {
    assert(self != NULL);

    return g_queue_is_empty(self->scommands) != FALSE;
}

unsigned int pipeline_length(const pipeline self) {
    assert(self != NULL);

    unsigned int length = g_queue_get_length(self->scommands);

    assert((length == 0) == pipeline_is_empty(self));

    return length;
}

scommand pipeline_front(const pipeline self) {
    assert(self != NULL);
    assert(!pipeline_is_empty(self));

    scommand result = (scommand) g_queue_peek_head(self->scommands);

    assert(result != NULL);

    return result;
}

bool pipeline_get_wait(const pipeline self) {
    assert(self != NULL);

    return self->wait;
}

scommand pipeline_get(const pipeline self, unsigned int index) {
    assert(self != NULL);
    assert(index < pipeline_length(self));
    return (scommand) g_queue_peek_nth(self->scommands, index);
}

char *pipeline_to_string(const pipeline self) {
    assert(self != NULL);

    char *result = strdup(""); // string acumulado

    /* Comandos simples */
    unsigned int length = pipeline_length(self);
    for (unsigned int i = 0; i < length; i++) {
        if (i > 0) {
            result = str_append(result, " | ");
        }
        char *scmd = scommand_to_string(
                         (scommand) g_queue_peek_nth(self->scommands, i)
                     );
        result = str_append(result, scmd);
        free(scmd);
    }

    /* Terminador que indica si no debe esperar */
    if (!pipeline_get_wait(self)) {
        result = str_append(result, " &");
    }

    assert(pipeline_is_empty(self) ||
           pipeline_get_wait(self) ||
           strlen(result) > 0);

    return result;
}

