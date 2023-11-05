/*
 * sclient.c
 */

#include <iostream>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>

using namespace std;

#define SERVER_PORT 2233
#define MAX_LINE 256
#define STDIN 0

int main(int argc, char *argv[])
{

  fd_set master;   // master file descriptor list
  fd_set read_fds; // temp file descriptor list for select()
  int fdmax;       // maximum file descriptor number

  struct sockaddr_in sin;
  char buf[MAX_LINE];
  char rbuf[MAX_LINE];
  int len;
  int s;

  FD_ZERO(&master); // clear the master and temp sets
  FD_ZERO(&read_fds);

  /* active open */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("select client: socket");
    exit(1);
  }

  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(argv[1]);
  sin.sin_port = htons(SERVER_PORT);

  // client network information
  printf("(Client) New client connected\n");
  printf("(Client) IP address is: %s\n", inet_ntoa(sin.sin_addr));
  printf("(Client) port is: %d\n", (int)ntohs(sin.sin_port));
  printf("(Client) Socket FD: %d\n", s);
  cout << endl;

  // check if we can connect
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("select client: connect");
    close(s);
    exit(1);
  }

  // add the STDIN to the master set
  FD_SET(STDIN, &master);

  // add the listener to the master set
  FD_SET(s, &master);

  // keep track of the biggest file descriptor
  fdmax = s; // so far, it's this one

  /* main loop; get and send lines of text */
  while (1)
  {
    read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("select");
      exit(1);
    }

    // looking for data to read either from the server or the user
    // if STDIN is in the set of file descriptors, then the user has provided input at the terminal
    if (FD_ISSET(STDIN, &read_fds))
    {
      // handle user input and send to the server
      if (fgets(buf, sizeof(buf), stdin))
      {
        buf[MAX_LINE - 1] = '\0';
        len = strlen(buf) + 1;

        // store all command line args for possible use
        char args[MAX_LINE];
        strcpy(args, buf);

        // extract the command from user input
        char *token = strtok(buf, " ");
        string command(token);

        if (command == "MSGGET\n")
        {
          send(s, buf, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << "ECHO: " << rbuf << endl;
        }
        else if (command == "QUIT\n")
        {
          send(s, buf, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << "ECHO: " << rbuf << endl;
          exit(0);
        }
        else if (command == "WHO\n")
        {
          send(s, args, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << "ECHO: " << rbuf << endl;
        }
        else if (command == "SEND\n")
        {
          cout << "ERROR: You must provide a name to send to.\n"
               << endl;
        }
        else if (command == "SEND")
        {
          send(s, args, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << "ECHO: " << rbuf << endl;
        }
        else if (command == "LOGIN")
        {
          send(s, args, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << rbuf << endl;
        }
        else if (command == "LOGIN\n")
        {
          cout << "ERROR: Username and password must be passed as arguments.\n"
               << endl;
        }
        else if (command == "LOGOUT\n")
        {
          send(s, args, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << "ECHO: " << rbuf << endl;
        }
        else if (command == "MSGSTORE\n")
        {
          send(s, args, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          if (strncmp(rbuf, "200", 3) == 0)
          {
            cout << rbuf << endl
                 << "Client: Authorized, enter message" << endl;
            fgets(buf, MAX_LINE, stdin);
            send(s, buf, MAX_LINE, 0);
            recv(s, rbuf, MAX_LINE, 0);
            cout << "ECHO: " << rbuf << endl;
          }
          else
          {
            cout << "ECHO: " << rbuf << endl;
          }
        }
        else if (command == "SHUTDOWN\n")
        {
          send(s, buf, MAX_LINE, 0);
          recv(s, rbuf, MAX_LINE, 0);
          cout << "ECHO: " << rbuf << endl;
          if (strncmp(rbuf, "200", 3) == 0)
          {
            close(s);
            exit(0);
          }
        }
        else
        {
          cout << "Client: Command not recognized \n" << endl;
        }
        // flush buffers
        memset(buf, 0, MAX_LINE);
        memset(rbuf, 0, MAX_LINE);
        memset(args, 0, MAX_LINE);
      }
      else
      {
        break;
      }
    }

    // handle data from the server
    if (FD_ISSET(s, &read_fds))
    {
      if (recv(s, rbuf, sizeof(buf), 0) > 0)
      {
        cout << rbuf;

        // check if we are receiving broadcast to shutdown
        if (strncmp(rbuf, "200", 3) == 0)
        {
          cout << "Closing client." << endl;
          close(s);
          exit(0);
        }
      }
    }
  }

  close(s);
}
