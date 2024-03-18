#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#include <limits.h>
#include <iostream>

using namespace std;

extern int errno;
int port;

int main(int argc, char *argv[])
{
    int sd;         //socket descriptor

    struct sockaddr_in server;

    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket()\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons (port);

    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("Eroare la connect()\n");
        return errno;
    }

    printf("Conectat!\nFoloseste comanda [show_commands] pentru a vizualiza toate comenzile.\n");

    while (1)
    {
        char comanda[PATH_MAX];
        fgets(comanda, PATH_MAX, stdin);
        int lgComanda = strlen(comanda);
        write(sd, &lgComanda, sizeof(int));
        write(sd, comanda, lgComanda);

        int nrMsg = 0;
        read(sd, &nrMsg, sizeof(int));
        for (int i = 0; i < nrMsg; i++)
        {
            int lgMesaj = 0;
            read(sd, &lgMesaj, sizeof(int));
            char* mesaj;
            mesaj = (char*)malloc(lgMesaj + 1);
            read(sd, mesaj, lgMesaj);
            mesaj[lgMesaj] = '\0';
            printf("%s\n", mesaj);
        }
    }
}