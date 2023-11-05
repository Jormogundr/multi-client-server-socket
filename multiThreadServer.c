/*
 * multiThreadServer.c -- a multithreaded server
 */

// make a table
// each time a client connects, get an ip and socket fd number, and user id

// socket #	| 	IP 	|	 user ID
// 4		127.0.0.1		John
// 5		141.215.202.69		David

// depending on the command we receive from the client, the server can simply send a single string back to client containing e.g. "200 OK \n List of active users are:\n ...."

// SHUTDOWN command
// use exit() to temrinate all threads on machine during shutdown, otherwise use pthread_exit to shutdown specific machines

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <cassert>
#include <vector>

using namespace std;

#define PORT 2233 // port we're listening on
#define MAX_LINE 256

fd_set master; // master file descriptor list
int listener;  // listening socket descriptor
int fdmax;

map<string, string> user_credentials;
map<string, string> name_ip_map;
map<string, int> name_fd_map;

class Server
{
public:
    // store indices for message of the day
    int num_messages;
    int current_message_idx;
    int current_message_size;
    // store user state
    vector<string> messages{
        "You create your own opportunities.\n",
        "Never break your promises.\n",
        "You are never as stuck as you think you are.\n",
        "Happiness is a choice.\n",
        "Habits develop into character.\n"};

    // Retrive the current message of the day and set index
    string getMessageOfTheDay()
    {

        // check if we are at the end of the 0-indexed vector of messages, and wrap back to 0 as needed
        if (current_message_idx < num_messages - 1)
            current_message_idx++;
        else
            current_message_idx = 0;

        string msg = messages[current_message_idx];
        current_message_size = msg.size();
        return msg;
    }

    // constructor
    Server()
    {
        current_message_idx = 0;
        num_messages = messages.size();

        // initialize the user_credentials map
        vector<string> users = {"root", "john", "david", "mary"};
        vector<string> passwords = {"root01", "john01", "david01", "mary01"};
        for (int i = 0; i < 3; i++)
        {
            user_credentials.insert(make_pair(users[i], passwords[i]));
        }
    }

    void addActiveUserIP(string currentUser, char *ip_addr)
    {
        name_ip_map.insert(make_pair(currentUser, ip_addr));
    }

    void addActiveUserFD(string currentUser, int fd)
    {
        name_fd_map.insert(make_pair(currentUser, fd));
    }

    // helper to allow user to MSGSTORE
    void addMotd(char *buf)
    {
        string message(buf);
        messages.push_back(message);
        num_messages++;
    }

    // main function for MSGSTORE
    void storeMessage(char *buf)
    {
        addMotd(buf);
        string confirmed = "200, message stored\n";
        char *response = const_cast<char *>(confirmed.c_str());
        memcpy(buf, response, MAX_LINE);
    }

    // Check if user has authenticated with LOGIN, return false is not
    bool validateUser(char *buf, bool *isAuthenticated)
    {
        if (*isAuthenticated == false)
        {
            string not_authorized = "401, Permission denied\n";
            char *failure = const_cast<char *>(not_authorized.c_str());
            memcpy(buf, failure, MAX_LINE);
            return false;
        }
        string authorized = "200, User credentials valid\n";
        char *success = const_cast<char *>(authorized.c_str());
        memcpy(buf, success, MAX_LINE);
        return true;
    }

    // helper for MSGSTORE
    void buildMessage(char *buf)
    {
        string response = "200, OK\n";
        string motd = getMessageOfTheDay();
        string res = response + motd;
        memcpy(buf, const_cast<char *>(res.c_str()), MAX_LINE);
        cout << buf << endl;
        setMessageSize(buf);
    }

    // LOGIN
    void login(char *args, char *buf, char *ip_addr, int fd, char *user, bool *isAuthenticated, bool *isRoot)
    {
        // check first if we are already logged in
        if (*isAuthenticated)
        {
            string success_str = "200, You are already logged in! Nothing to do. \n";
            char *success = const_cast<char *>(success_str.c_str());
            memcpy(buf, success, MAX_LINE);
            return;
        }
        char *pch;
        pch = strtok(args, " ");
        int counter = 0;
        vector<string> credentials;
        while (pch != NULL)
        {
            if (counter == 0)
            {
                counter++;
                pch = strtok(NULL, " ");
                continue;
            }
            else if (counter == 1)
            {
                // add user to beginning of credentials (len 2)
                string user(pch);
                credentials.insert(credentials.begin(), user);
            }
            else if (counter == 2)
            {
                // add password to end of credentials (len 2)
                string password(pch);
                credentials.push_back(password);
            }
            pch = strtok(NULL, " ");
            counter++;
        }

        bool credential_size = (credentials.size() == 2);
        if (!credential_size)
        {
            string success_str = "200, Number of credentials (user, password) not satisfied. Try again";
            char *success = const_cast<char *>(success_str.c_str());
            memcpy(buf, success, MAX_LINE);
            return;
        }

        // Check if user is root and has correct password, and set isRoot accordingly
        if (authenicateUser(credentials[0], credentials[1]))
        {
            if (credentials[0] == "root")
            {
                *isRoot = true;
            }
            string success_str = "200, Authentication success\n";
            char *success = const_cast<char *>(success_str.c_str());
            memcpy(buf, success, MAX_LINE);
            *isAuthenticated = true;
        }
        else
        {
            string fail_str = "401, Wrong UserID or Password\nAuthentication failed\n";
            char *failure = const_cast<char *>(fail_str.c_str());
            memcpy(buf, failure, MAX_LINE);
        }
        string currentUser;
        currentUser = credentials[0];
        cout << currentUser << " has logged in successfully." << endl;

        // add user:ip_addr mapping
        addActiveUserIP(currentUser, ip_addr);
        memcpy(user, const_cast<char *>(currentUser.c_str()), MAX_LINE);

        // add user:socket_fd mapping
        addActiveUserFD(currentUser, fd);
    }

    // helper for various credentials related functions
    bool authenicateUser(string user, string password)
    {
        if (user_credentials[user] == password)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // LOGOUT
    void logout(char *buf, char *user)
    {
        string res_code = "200, OK";
        char *response = const_cast<char *>(res_code.c_str());
        memcpy(buf, response, MAX_LINE);

        // delete user from the map of active users
        string k(user);
        name_ip_map.erase(k);
        name_fd_map.erase(k);
    }

    void quit(char *buf)
    {
        string res_code = "200, Closing connection\n";
        strcpy(buf, res_code.c_str());
    }

    void setMessageSize(char *msg)
    {
        current_message_size = (sizeof(msg) / sizeof(msg[0]));
    }
};

// struct for passing arguments to ChildThread
// TODO: prevent race conditions when different ChildThreads change these different vars
struct readThreadParams
{
    int newfd;
    Server *server;
    struct sockaddr_in *remote_sock;
} args;

// the child thread, used to handle interactions with the client
void *ChildThread(void *context)
{
    struct readThreadParams *readParams = (struct readThreadParams *)context;
    char buf[MAX_LINE];
    int nbytes;
    int i, j;
    int childSocket = (long)readParams->newfd;

    printf("(Server) New client connected\n");
    printf("(Server) IP address is: %s\n", inet_ntoa(readParams->remote_sock->sin_addr));
    printf("(Server) port is: %d\n", (int)ntohs(readParams->remote_sock->sin_port));
    printf("(Server) Socket FD: %d\n", childSocket);
    cout << endl;

    // user state stored in each thread on a per session basis
    bool isAuthenticated = false;
    bool isRoot = false;
    char *ip_addr = inet_ntoa(readParams->remote_sock->sin_addr);

    while (1)
    {
        // handle data from a client
        // read nbytes from buffer, this is a cancellation point if no bytes read
        if ((nbytes = recv(childSocket, buf, sizeof(buf), 0)) <= 0)
        {
            // got error or connection closed by client
            if (nbytes == 0)
            {
                // connection closed
                cout << "multiThreadServer: socket " << childSocket << " hung up" << endl;
            }
            else
            {
                perror("recv");
            }
            close(childSocket);           // bye!
            FD_CLR(childSocket, &master); // remove from master set
            pthread_exit(0);
        }
        else
        { // we read some bytes (a command) from the buffer sent by client, so do something with it
            // we got some data from a client
            cout << buf;

            char response[MAX_LINE] = "";

            // commands where we need to be sending a response back to a specific client
            // strtok modifies buf by inserting terminating chars \000 so copy buf into new buffer for commands w/ args
            char unmodified_buf[MAX_LINE];
            memcpy(unmodified_buf, buf, MAX_LINE);
            char *token = strtok(buf, " ");
            string command(token);

            // session state (per command basis)
            char instance_user[MAX_LINE]; // store the name of the current user in the thread, not the server object

            if (command == "MSGGET\n")
            {
                if (isAuthenticated)
                {
                    readParams->server->buildMessage(buf);
                }
                else
                {
                    string fail_str = "401, User not authenticated\n";
                    char *failure = const_cast<char *>(fail_str.c_str());
                    memcpy(buf, failure, MAX_LINE);
                }
                send(childSocket, buf, MAX_LINE, 0);
            }
            else if (command == "QUIT\n")
            {
                readParams->server->quit(response);
                isAuthenticated = false;
                isRoot = false;
                send(childSocket, response, MAX_LINE, 0);
                exit(0);
            }
            // a new line is not expected at the end of this command since additional arguments must be provided
            else if (command == "LOGIN")
            {
                // newline char delimits the end of the command provided by user
                char *args = strtok(unmodified_buf, "\n");
                readParams->server->login(args, buf, ip_addr, childSocket, instance_user, &isAuthenticated, &isRoot);
                send(childSocket, buf, MAX_LINE, 0);
            }
            else if (command == "LOGOUT\n")
            {
                readParams->server->logout(response, instance_user);
                send(childSocket, response, MAX_LINE, 0);
                isAuthenticated = false;
                isRoot = false;
            }
            else if (command == "SEND")
            {
                if (name_fd_map.find(string(instance_user)) == name_fd_map.end())
                {
                    send(childSocket, "420 - You are not signed in.\n", MAX_LINE, 0);
                    continue;
                }
                char *pch;
                char *args = strtok(unmodified_buf, "\n");
                pch = strtok(args, " ");
                int counter = 0;
                string user;
                int dst_fd;
                while (pch != NULL)
                {
                    if (counter == 0)
                    {
                        counter++;
                        pch = strtok(NULL, " ");
                        continue;
                    }
                    else if (counter == 1)
                    {
                        // user is the string key needed for name_fd_map lookup, to resolve the relevant fd for destination
                        string u(pch);
                        user = u;
                        if (name_fd_map.find(user) == name_fd_map.end())
                        {
                            send(childSocket, "420 - Either the user does not exist or is not logged in\n", MAX_LINE, 0);
                            break;
                        }
                        else
                        {
                            dst_fd = name_fd_map.find(user)->second;
                            pch = strtok(NULL, "\n");
                            string msg = string(pch);
                            string res = "200 OK - You have a new message from ";
                            string src_user = string(instance_user);
                            res.append(src_user);
                            res.append(": ");
                            res.append(msg);
                            res.append("\n");
                            char *response = const_cast<char *>(res.c_str());
                            memcpy(buf, response, MAX_LINE);
                            send(dst_fd, response, MAX_LINE, 0);
                            break;
                        }
                    }
                    break;
                }
            }
            else if (command == "WHO\n")
            {
                if (!name_ip_map.empty())
                {
                    string who = "200 OK \n Current active users are: \n";
                    for (auto const &x : name_ip_map)
                    {
                        who.append(x.first);
                        who.append(":");
                        who.append(x.second);
                        who.append("\n");
                    }
                    char *response = const_cast<char *>(who.c_str());
                    memcpy(buf, response, MAX_LINE);
                    send(childSocket, response, MAX_LINE, 0);

                    // TODO: Handle case where who.length() > MAX_LINE
                }
                else
                {
                    // inform client nobody is logged in
                    send(childSocket, "204, OK - Nobody is logged in.\n", MAX_LINE, 0);
                }
            }
            else if (command == "MSGSTORE\n")
            {
                // newline char delimits the end of the command provided by user
                char *args = strtok(unmodified_buf, "\n");
                bool authorized;
                readParams->server->validateUser(buf, &isAuthenticated) ? authorized = true : authorized = false;
                send(childSocket, buf, MAX_LINE, 0);
                if (authorized)
                {
                    cout << "Server: waiting for user input..." << endl;
                    recv(childSocket, buf, MAX_LINE, 0);
                    cout << "Server: received message: " << buf << endl;
                    readParams->server->storeMessage(buf);
                    send(childSocket, buf, MAX_LINE, 0);
                }
            }
            else if (command == "SHUTDOWN\n")
            {
                if (isRoot)
                {
                    // broadcast shutdown to all connected clients
                    for (j = 0; j <= fdmax; j++)
                    {
                        if (FD_ISSET(j, &master))
                        {
                            // except the listener and ourselves
                            if (j != listener && j != childSocket)
                            {
                                if (send(j, buf, nbytes, 0) == -1)
                                {
                                    cout << "(Server) No bytes sent during shutdown broadcast \n";
                                    perror("send");
                                }
                                else
                                {
                                    send(j, "200 - Closing connection to clients.\n", MAX_LINE, 0);
                                    close(j);
                                }
                            }
                        }
                    }
                    // finish cleanup - close the connection to client that sent shutdown, then close server
                    send(childSocket, "200 - Closing connection to clients.\n", MAX_LINE, 0);
                    close(childSocket);
                    close(listener);
                    exit(0);
                }

                else
                {
                    send(childSocket, "403 - Only root can perform this operation.\n", MAX_LINE, 0);
                }
            }
            else
            {
                cout << "Server: Command not recognized" << endl;
            }
            // flush the buffer
            memset(buf, 0, MAX_LINE);
        }
    }
}

int main(void)
{
    struct sockaddr_in myaddr;     // server address
    struct sockaddr_in remoteaddr; // client address
    int newfd;                     // newly accept()ed socket descriptor
    int yes = 1;                   // for setsockopt() SO_REUSEADDR, below
    socklen_t addrlen;

    pthread_t cThread;
    Server motd;

    FD_ZERO(&master); // clear the master and temp sets

    // get the listener
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // lose the pesky "address already in use" error message
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    // bind
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(PORT);
    memset(&(myaddr.sin_zero), '\0', 8);
    if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // listen
    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(1);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    addrlen = sizeof(remoteaddr);

    // main loop
    for (;;)
    {
        // handle new connections

        // if new connection unsuccessful
        if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1)
        {
            perror("accept");
            exit(1);
        }
        else
        {                           // connection is successful, socket file descriptor to master file descriptor set
            FD_SET(newfd, &master); // add to master set
            cout << "multiThreadServer: new connection from "
                 << inet_ntoa(remoteaddr.sin_addr)
                 << " socket " << newfd << endl;

            // initialize args struct
            args.newfd = newfd;
            args.server = &motd;
            args.remote_sock = &remoteaddr;

            if (newfd > fdmax)
            { // keep track of the maximum
                fdmax = newfd;
            }
            // for successful connection, create child process to run client/server interactions handled by ChildThread, which runs indefinitely
            if (pthread_create(&cThread, NULL, ChildThread, &args) < 0)
            {
                perror("pthread_create");
                exit(1);
            }
        }
    }
    return 0;
}
