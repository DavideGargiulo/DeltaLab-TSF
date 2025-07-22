#ifndef AUTH_H
#define AUTH_H

void authRegister(int client_socket, const char *request);
void authLogin(int client_socket, const char *request);

#endif
