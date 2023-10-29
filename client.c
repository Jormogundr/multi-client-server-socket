/*
 * client.c
 */
#define _GLIBCXX_USE_CXX11_ABI 0

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <cstring>

using namespace std;

#define SERVER_PORT 2233
#define MAX_LINE 256

int main(int argc, char * argv[]) {

    struct sockaddr_in sin;
    char buf[MAX_LINE];
    char rbuf[MAX_LINE];
    int len;
    int s;

    if (argc < 2) {
		cout << "Usage: client <Server IP Address>" << endl;
		exit(1);
    }

    /* active open */
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr  = inet_addr(argv[1]);
    sin.sin_port = htons (SERVER_PORT);

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		close(s);
		exit(1);
    }

    /* main loop; get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin)) {
		buf[MAX_LINE - 1] = '\0';
		len = strlen(buf) + 1;

    // store all command line args for possible use
    char args[MAX_LINE];
    strcpy(args, buf);
    
    // extract the command from user input
    char* token = strtok(buf, " ");
    string command(token);

    if (command == "MSGGET\n") {
      send (s, buf, MAX_LINE, 0);
      recv (s, rbuf, MAX_LINE, 0);
      cout << "ECHO: " << rbuf << endl;
    }
    else if (command == "QUIT\n") {
      send (s, buf, MAX_LINE, 0);
      recv (s, rbuf, MAX_LINE, 0);
      // TODO: cleanup memory before quitting?
      cout << "ECHO: " << rbuf << endl;
      exit(0);
    }
    else if (command == "LOGIN") {
      send (s, args, MAX_LINE, 0);
      recv (s, rbuf, MAX_LINE, 0);
      cout << "ECHO: " << rbuf << endl;
    }
    else if (command == "LOGIN\n") {
      cout << "ERROR: Username and password must be passed as arguments.\n" << endl;
    }
    else if (command == "LOGOUT\n") {
      send (s, args, MAX_LINE, 0);
      recv (s, rbuf, MAX_LINE, 0);
      cout << "ECHO: " << rbuf << endl;
    }
    else if (command == "MSGSTORE\n") {
      send (s, args, MAX_LINE, 0);
      recv (s, rbuf, MAX_LINE, 0);
      if (strncmp(rbuf, "200", 3) == 0) {
        cout << rbuf << endl << "Client: Authorized, enter message" << endl;
        fgets(buf, MAX_LINE, stdin);
        send (s, buf, MAX_LINE, 0);
        recv (s, rbuf, MAX_LINE, 0);
        cout << "ECHO: " << rbuf << endl;
      } 
      else {cout << "ECHO: " << rbuf << endl;}
      }
    else if (command == "SHUTDOWN\n") {
      send (s, buf, MAX_LINE, 0);
      recv (s, rbuf, MAX_LINE, 0);
      cout << "ECHO: " << rbuf << endl;
      if (strncmp(rbuf, "200", 3) == 0) {
        cout << "Closing client." << endl;
        exit(0);
      }
     }
    else {
      cout << "Client: Command not recognized" << endl;
    }
    // flush buffers
    memset(buf, 0, MAX_LINE);
    memset(rbuf, 0, MAX_LINE);
    memset(args, 0, MAX_LINE);
    }

    close(s);
} 
 
