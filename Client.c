/*
 *
 * Client file
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


//Variabler som brukes i void metodene
int sock;
unsigned char type;
unsigned int lengde;
char tekst[1024];

void usage();
void getJob();
void isCorrupt();
void sig_handler(int);

/*
 * Main metoden
 *
 * 1. Starter to barneprossesser
 * 2. Klient og barneprossesser kommunisere med pipes
 * 3. Klient kobler seg til server
 * 4. Gir bruker 4 valgmuligheter
 */
int main(int argc, char* argv[]) {

  //Argumenter fra terminalen
  char* ip = argv[1];
  char* port = argv[2];
  //mode on/off ?


  pid_t child1, child2;
  int fd1[4], save_read1;
  char tekst_child1[1024];

  child1 = fork();

  /*
    if (pipe(fd1) < 0) {
    perror("pipe");
    exit(1);
    }
  */

  if (child1 < 0) {
    printf("\nError");
    exit(1);

  } else if (child1 == 0) {
    printf("\nHello I am the child1 process.\n");
    save_read1 = read(fd1[0], tekst_child1, sizeof(tekst_child1));
    tekst_child1[save_read1] = 0;
    /*
      close(fd1[READ]);
      write(fd1[WRITE], tekst_child1, sizeof(tekst_child1));
      close(fd1[WRITE]);
    */
  } else {
    printf("\nHello I am the parent process.\n");

    child2 = fork();

    if (child2 < 0) {
      printf("\nError");
      exit(1);

    } else if (child2 == 0) {
      printf("\nHello I am the child2 process.\n");
      printf("Her er stderr.\n");

    } else {
      printf("\nHello I am the parent process.\n");

      if (argc != 3) {
	fprintf(stderr, "Usage: %s <IP> <port>\n", argv[0]);
	return 0;
      }

      sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (sock == -1) {
	perror("socket()");
	exit(EXIT_FAILURE);
      }

      int port_num = atoi(port);

      //Print test
      printf("IP Adressen er: %s\n", ip);
      printf("Port nummeret er: %d\n", port_num);

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
        exit(EXIT_FAILURE);
      }

      printf("Connecting to %s:%d\n", ip, port_num);
      if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
	       perror("connect()");
	        close(sock);
	         exit(EXIT_FAILURE);
      }

      printf("Yay, connected!\n");

      //Utforer oppgaver i forhold til valgmulighetene
      while(1) {

	//Meny
	usage(argc, argv);

	char cmd[4];
	scanf("%s", cmd);

	//Brukes for Ctrl+C
	if(signal (SIGINT, sig_handler) == SIG_ERR) {
	  continue;
	}

	if(strcmp(cmd, "1") == 0) {

	  getJob();
	  isCorrupt();

	  //skjekker om tom
	  if (lengde == 0) {
	    printf("Jobs done.\n");
	    exit(EXIT_SUCCESS);
	  }

	  //skal printes til stdout hvis 'O'
	  if (type == 'O') {
	  printf("skal sendes til barn1\n");
	  write (fd1[1], tekst, (int)lengde);
	  }
	  //skal printes til stderr hvis 'E'
	  if (type == 'E') {
	  printf("Skal sendes til barn2\n");
	  fprintf(stderr, "%s", tekst);
	  }


	} else if (strcmp(cmd, "2") == 0) {
	  /*Hent X antall jobber fra serveren*/
	  printf("Antall?\n");
	  int antall;
	  scanf("%d", &antall);
	  printf("Antall er: %d\n", antall);

	  int i;
	  for (i = 0; i < antall; i++) {
	    getJob();
	    isCorrupt();

	    //skjekker om tom
	    if (lengde == 0) {
	      printf("Jobs done.\n");
	      exit(EXIT_SUCCESS);
	    }

	    //skal printes til stdout hvis 'O'
	    if (type == 'O') {
	    printf("skal sendes til barn1\n");
	    write (fd1[1], tekst, (int)lengde);
	    }
	    //skal printes til stderr hvis 'E'
	    if (type == 'E') {
	    printf("Skal sendes til barn2\n");
	    fprintf(stderr, "%s", tekst);
	    }
	  }

	} else if(strcmp(cmd, "3") == 0) {
	  //Hent alle jobber fra serveren
	  while(1) {

	    getJob();
	    isCorrupt();

	    //skjekker om tom
	    if (lengde == 0) {
	      printf("Jobs done.\n");
	      exit(EXIT_SUCCESS);
	    }
	    //skal printes til stdout hvis 'O'
	    if (type == 'O') {
	      printf("Skal sendes til barn1\n");
	      write (fd1[1], tekst, (int)lengde);
	    }
	    //skal printes til stderr hvis 'E'
	    if (type == 'E') {
	      printf("Skal sendes til barn2\n");
	      fprintf(stderr, "%s", tekst);
	    }
	  }

	} else if (strcmp(cmd, "4") == 0) {
	  //Avslutter programmet
	  printf("Programmet avsluttes...\n");
	  send(sock, "T", sizeof(char), 0);
	  close(sock);
	  exit(0);

	} else {
	  printf("Invalid command.\n");
	  usage();
	}
      }
      close(sock);
      return 0;
    }
  }
  exit(1);
  return 0;
}


/*
 * En meny som vises hver gang brukeren skal taste en kommando.
 */
void usage() {

  printf("\n1\tHent en jobb fra serveren.\n");
  printf("2\tHent X antall jobber fra serveren (antall?).\n");
  printf("3\tHent alle jobber fra serveren.\n");
  printf("4\tAvslutte programmet.\n");
}

/*
 * Henter en job fra serveren.
 */
void getJob() {
  send(sock, "G", sizeof(char), 0);
  recv(sock, &type, sizeof(unsigned char), 0);
  recv(sock, &lengde, sizeof(unsigned int), 0);
  recv(sock, tekst, (int)lengde, 0);

  //print test
  printf("\nType: %c\n", type);
  printf("Lengde: %d\n", lengde);
  tekst[lengde] = 0;
  printf("Tekst: %s\n", tekst);
}

/*
 * Skjekker om korrupt type.
 * Hvis det er tilfellen, sendes til stdout.
 */
void isCorrupt() {
  if (type != 'O' && type != 'E' && type != 'Q') {
    printf("Error: Har ingen relevant type.");
    send(sock, "E", sizeof(char), 0);
    close(sock);
    exit(EXIT_FAILURE);
  }
}

/*
 * Handterer Ctrl+C
 */
void sig_handler(int signo) {
  if(signo == SIGINT) {
    printf("\nCtrl+C signal caught!\n");
    printf("Avslutter pga Ctrl+C.\n");
    send(sock, "T", sizeof(char), 0);
    close(sock);
    exit(0);
  }
}
