#ifndef CLIENT_AUTH_H
#define CLIENT_AUTH_H

// Authentication functions
int handle_login(const char *username, const char *password);
int handle_register(const char *username, const char *password);

#endif // CLIENT_AUTH_H

