#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

#include "builtin.h"
#include "tests/syscall_mock.h"

/* Nombres de los comandos internos; NULL marca el fin de la lista */
static const char *builtins[] = {"cd", "exit", "help", NULL};

/* Builtins implementados */
static void builtin_cd(scommand cmd);
static void builtin_exit(void);
static void builtin_help(void);

/* Funciones auxiliares */
static const char *get_home_directory(void);
static void change_directory(const char *path);



/* INTERFAZ DEL MÓDULO BUILTIN.H */



bool builtin_is_internal(scommand cmd) {
    assert(cmd != NULL);

    bool res = false;
    const char **ptr = builtins;

    if (!scommand_is_empty(cmd)) {
        char *cmd_name = scommand_front(cmd);
        while (*ptr != NULL) {
            res = res || (strcmp(cmd_name, *ptr) == 0);
            ptr++;
        }
    }

    return res;
}

bool builtin_alone(pipeline p) {
    assert(p != NULL);

    return pipeline_length(p) == 1 &&
           builtin_is_internal(pipeline_front(p));
}

void builtin_run(scommand cmd) {
    assert(builtin_is_internal(cmd));

    char *cmd_name = scommand_front(cmd);
    if (strcmp(cmd_name, "cd") == 0) {
        builtin_cd(cmd);
    } else if (strcmp(cmd_name, "exit") == 0) {
        builtin_exit();
    } else if (strcmp(cmd_name, "help") == 0) {
        builtin_help();
    }
}

bool pipeline_contains_builtin(const pipeline self) {
    assert(self != NULL);

    bool res = false;
    for (unsigned int i = 0; i < pipeline_length(self); i++) {
        res = res || builtin_is_internal(pipeline_get(self, i));
    }

    return res;
}



/* BUILTINS IMPLEMENTADOS */



/* Ejecuta el comando interno `cd`.
 * PRE: cmd != NULL && scommand_front(cmd) == "cd"
 * POS: se elimina "cd" de cmd
 */
static void builtin_cd(scommand cmd) {
    assert(cmd != NULL);
    assert(strcmp(scommand_front(cmd), "cd") == 0);

    unsigned int length = scommand_length(cmd);
    if (length > 2) {
        fprintf(stderr, "mybash: cd: too many arguments\n");
    } else if (length == 2) {
        /* "cd dir": cambiar al directorio especificado */
        scommand_pop_front(cmd);
        char *dir = scommand_front(cmd);
        if (dir != NULL) {
            change_directory(dir);
        } else {
            fprintf(stderr, "mybash: cd: could not change directory\n");
        }
    } else {
        /* "cd": cambiar al home del usuario */
        const char *home_directory = get_home_directory();
        if (home_directory != NULL) {
            change_directory(home_directory);
        } else {
            fprintf(stderr, "mybash: could not change to home directory\n");
        }
    }
}

static void builtin_exit(void) {
    exit(EXIT_SUCCESS);
}

static void builtin_help(void) {
    printf("MY-SHELL Version 1.0.0-release (x86_64-pc-linux-2025)\n"
           "echo Gerbaudo Nicolás '&' Ontivero Nahuel Mauricio '&' Vides Alejo Miguel\n"
           "    Los comandos internos son:\n"
           "        cd: Cambia el directorio de trabajo de la shell.\n"
           "        help: Imprime este mismo mensaje!\n"
           "        exit: Sale de la shell.\n");
}



/* FUNCIONES AUXILIARES */



/* Cambia el directorio actual a `path`.
 * PRE: path != NULL
 * Imprime un mensaje de error si falla.
 */
static void change_directory(const char *path) {
    assert(path != NULL);

    int rc = chdir(path);
    if (rc == -1) {  // fallo al cambiar directorio
        perror("mybash: cd"); // imprime motivo del fallo
    }
}

/* Devuelve el directorio home del usuario actual ("/home/user").
 * Retorna NULL si no se pudo obtener la información.
 * La cadena devuelta no debe liberarse.
 */
static const char *get_home_directory(void) {
    const char *res = NULL;
    uid_t user_id = getuid();
    struct passwd *user_info = getpwuid(user_id);  // NULL si falla

    if (user_info != NULL) {
        res = user_info->pw_dir;  // home directory
    }

    return res;
}

