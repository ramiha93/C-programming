/*
 *
 * Server file
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <signal.h>


/*
 * Denne metoden bruker ip og port som ble oppgitt i
 * terminalen og bruker disse to til aa opprette en socket.
 * Et par feilhaandteringer ligger der ogsaa.
 */
int create_socket(char* ip, char* port) {
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) {
    perror("socket()");
    return -1;
  }

  int port_num = atoi(port);

  //Oppretter en socket
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_num);

  //Feilsjekking
  int ip_ret = inet_pton(AF_INET, ip, &server_addr.sin_addr.s_addr);
  if (ip_ret != 1) {
    if (ip_ret == 0) {
      fprintf(stderr, "Invalid IP address: %s\n", ip);
    } else {
      perror("inet_pton()");
    }
    close(sock);
    return -1;
  }

  //Gir den et navn med bind()
  if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
    perror("bind()");
    close(sock);
    return -1;
  }

  printf("Bound successfully to %s:%d!.\n", ip, port_num);

  //Lytter paa socket, sier ifra om en feil oppstaar.
  if (listen(sock, SOMAXCONN)) {
    perror("listen()");
    close(sock);
    return -1;
  }
  return sock;
}


/*
 * Main metoden
 * Usage: ./Server <ip> <port> <file_name>
 */
int main(int argc, char* argv[]) {

  //Brukte kode for socket fra plenumstime som utgangspunkt.
  int sock;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <IP> <port> <file_name>\n", argv[0]);
    return 0;
  }

  sock = create_socket(argv[1], argv[2]);
  if(sock == -1) {
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  socklen_t addr_len = sizeof(client_addr);

  int client_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);
  if (client_sock == -1) {
    perror("accept()");
    close(sock);
    exit(EXIT_FAILURE);
  }

  printf("Client connected! :))\n");
  char* client_ip = inet_ntoa(client_addr.sin_addr);
  uint32_t client_ip_bin = ntohl(client_addr.sin_addr.s_addr);
  printf("IP/port: %s/0x%x\n", client_ip, client_ip_bin);

  //Variabler
  unsigned char jobbinfo; //var T, er naa I
  unsigned int tekstlengde;
  char jobbtekst[1080];
  char* file_name = argv[3];

  //Open file
  FILE *f;
  f = fopen(file_name, "r");

  //Utforer oppgavene som ble vist i menyen.
  while(1) {

    char recv_melding;
    recv(client_sock, &recv_melding, 4, 0);

    //Henter job
    if (recv_melding == 'G') {

      fread(&jobbinfo, sizeof(unsigned char), 1, f);
      fread(&tekstlengde, sizeof(unsigned int), 1, f);
      fread(jobbtekst, 1, tekstlengde, f);

      //Det som skjer etter vi har lest hele filen.
      if (feof(f)) {

	printf("Jobs done.\n");

	//Barneprossess termineres og variabler resettes.
	jobbinfo = 'Q';
	tekstlengde = 0;
	strcpy(jobbtekst, "");

	send(client_sock, &jobbinfo, sizeof(unsigned char), 0);
	send(client_sock, &tekstlengde, sizeof(unsigned int), 0);
	send(client_sock, jobbtekst, (int)tekstlengde, 0);
      }

      printf("\nSender jobbinfo: %c\n", jobbinfo);
      printf("Sender tekstlengde: %d\n", tekstlengde);
      printf("Sender jobbtekst: %s\n", jobbtekst);

      send(client_sock, &jobbinfo, sizeof(char), 0);
      send(client_sock, &tekstlengde, sizeof(int), 0);
      send(client_sock, jobbtekst, (int)tekstlengde, 0);

      //TODO: check send verdien

    } else if(recv_melding == 'T') {
      printf("Server will now close.\n");
      close(sock);
      fclose(f);
      exit(EXIT_SUCCESS);

    } else if(recv_melding == 'E') {
      printf("En feil oppsto.\n");
      close(sock);
      fclose(f);
      exit(EXIT_FAILURE);

    } else {
      printf("Ugyldig melding.\n");
      exit(EXIT_FAILURE);
    }
  }
  close(sock);
  close(client_sock);

  return 0;
}
