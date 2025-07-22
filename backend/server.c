#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "moduli/auth.h"
//#include "moduli/lobby.h"

#define PORT 8080
#define BUFFER_SIZE 4096

void route_request(int client_socket, const char *request) {
    if (strstr(request, "POST /register")) {
        authRegister(client_socket, request);
    } else if (strstr(request, "POST /login")) {
        authLogin(client_socket, request);
    } else {
        char *response = "HTTP/1.1 404 Not Found\r\n\r\nEndpoint non trovato";
        send(client_socket, response, strlen(response), 0);
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // 🛠️ Socket setup
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Errore socket");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Errore bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Errore listen");
        exit(EXIT_FAILURE);
    }

    printf("✅ Backend C in ascolto su porta %d...\n", PORT);

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
            memset(buffer, 0, sizeof(buffer));
            read(client_socket, buffer, BUFFER_SIZE);
            printf("📥 Richiesta:\n%s\n", buffer);

            // 📡 Instrada la richiesta
            route_request(client_socket, buffer);

            close(client_socket);
        }
    }

    return 0;
}
