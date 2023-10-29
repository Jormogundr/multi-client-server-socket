/*
 * server.c
 */
#define _GLIBCXX_USE_CXX11_ABI 0

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <cassert> 

using namespace std;

#define SERVER_PORT 2233
#define MAX_PENDING 5
#define MAX_LINE 256

map<string, string> user_credentials;

class Server {
  public:
    // store indices for message of the day 
    int num_messages;
    int current_message_idx;
    int current_message_size; 
    // store user state
    bool authenticated; 
    bool isRoot; 
    vector<string> messages {
      "You create your own opportunities.\n", 
      "Never break your promises.\n", 
      "You are never as stuck as you think you are.\n",
      "Happiness is a choice.\n",
      "Habits develop into character.\n"
      };

    // Retrive the current message of the day and set index
    string getMessageOfTheDay() {

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
    Server() {
      current_message_idx = 0;
      num_messages = messages.size();
      authenticated = false;
      isRoot = false;

      // initialize the user_credentials map
      vector<string> users  = {"root", "john", "david", "mary"};
      vector<string> passwords = {"root01", "john01", "david01", "mary01"};
      for (int i = 0; i < 3; i++) {
        user_credentials.insert(make_pair(users[i], passwords[i]));
      }
    }

    // helper to allow user to MSGSTORE
    void addMotd(char* buf) {
      string message(buf);
      messages.push_back(message);
      num_messages++;
    }

    // main function for MSGSTORE
    void storeMessage(char* buf) {
      addMotd(buf);
      string confirmed = "200, message stored\n";
      char* response = const_cast<char*>(confirmed.c_str());
      memcpy(buf, response, MAX_LINE);
    }

    // Check if user has authenticated with LOGIN, return false is not
    bool validateUser(char* buf) {      
      if (getAuthenticated() == false) {
          string not_authorized = "401, Permission denied\n";
          char* failure = const_cast<char*>(not_authorized.c_str());
          memcpy(buf, failure, MAX_LINE);
          return false;
      }
      string authorized = "200, User credentials valid\n";
      char* success = const_cast<char*>(authorized.c_str());
      memcpy(buf, success, MAX_LINE);
      return true;
    }

    // helper for MSGSTORE
    void buildMessage(char* buf) {
      string response = "200, OK\n";
      string motd = getMessageOfTheDay();
      string res = response + motd;
      memcpy(buf, const_cast<char*>(res.c_str()), MAX_LINE);
      cout << buf << endl;
      setMessageSize(buf);
    }

    // main function for SHUTDOWN
    void shutdown(char* buf, int socket) {
      cout << "Sever: Shutting down..." << endl;
      string authorized = "200, Shutting server down\n";
      char* success = const_cast<char*>(authorized.c_str());
      memcpy(buf, success, MAX_LINE);
      send (socket, buf, MAX_LINE, 0);
      close(socket);
      exit(0);
    }

    // LOGIN
    void login(char* args, char* buf) {
      char* pch;
      pch = strtok(args," ");
      int counter = 0;
      vector<string> credentials;
      while (pch != NULL) {
          if (counter == 0) {
            counter++;
            pch = strtok(NULL, " ");
            continue;
          }
          else if (counter == 1) {
            // add user to beginning of credentials (len 2)
            string user(pch);
            credentials.insert(credentials.begin(), user);
          }
          else if (counter == 2) {
            // add password to end of credentials (len 2)
            string password(pch);
            credentials.push_back(password);
          }
          pch = strtok(NULL, " ");
          counter++;
      }
      
      bool credential_size = (credentials.size() == 2);
      if (!credential_size) {
          string success_str = "200, Number of credentials (user, password) not satisfied. Try again";
          char* success = const_cast<char*>(success_str.c_str());
          memcpy(buf, success, MAX_LINE);
          return;
      }
      
      // Check if user is root and has correct password, and set isRoot accordingly
      if (authenicateUser(credentials[0], credentials[1])) {
        if (credentials[0] == "root") {isRoot = true;}
          string success_str = "200, Authentication success\n";
          char* success = const_cast<char*>(success_str.c_str());
          memcpy(buf, success, MAX_LINE);
          setAuthenticated(true);
      }
      else {
          string fail_str = "401, Wrong UserID or Password\nAuthentication failed\n";
          char* failure = const_cast<char*>(fail_str.c_str());
          memcpy(buf, failure, MAX_LINE);
        }
    }

    // helper for various credentials related functions
    bool authenicateUser(string user, string password) {
      if (user_credentials[user] == password) {
        return true;
      }
      else {
        return false;
      }
    }

    // LOGOUT
    void logout(char* buf) {
      string res_code = "200, OK";
      char* response = const_cast<char*>(res_code.c_str());
      memcpy(buf, response, MAX_LINE);
      setAuthenticated(false);
      isRoot = false;
    }

    void quit(char* buf) {
      string res_code = "200, Closing connection\n";
      strcpy(buf, res_code.c_str());
    }

    void setMessageSize(char* msg) {
      current_message_size = (sizeof(msg)/sizeof(msg[0]));
    }

    void setAuthenticated(bool flag) {
      authenticated = flag;
    }

    bool getAuthenticated() {
      return authenticated;
    }

    bool getIsRoot() {
      return isRoot;
    }
};



int main(int argc, char **argv) {

    struct sockaddr_in sin;
    socklen_t addrlen;
    char buf[MAX_LINE];
    int len;
    int s;
    int new_s;

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (SERVER_PORT);

    /* setup passive open */
    if (( s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
    }

    // set re-usable address option so that the address is not busy when re-running the server app
    int set_reusable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &set_reusable, sizeof(int)) < 0 ) {
        perror("setsockopt");
        exit(1);
    }

    if ((bind(s, (struct sockaddr *) &sin, sizeof(sin))) < 0) {
		perror("bind");
		exit(1);
    }

    listen (s, MAX_PENDING);

    addrlen = sizeof(sin);
    cout << "The server is up, waiting for connection" << endl;

    /* wait for connection, then receive and print text */
    while (1) {
		if ((new_s = accept(s, (struct sockaddr *)&sin, &addrlen)) < 0) {
			perror("accept");
			exit(1);
		}
		cout << "new connection from " << inet_ntoa(sin.sin_addr) << endl;

     
    Server motd;
		while (len = recv(new_s, buf, sizeof(buf), 0)) {
      char response[MAX_LINE] = "";
    
      // strtok modifies buf by inserting terminating chars \000 so copy buf into new buffer for commands w/ args
      char unmodified_buf[MAX_LINE];
      memcpy(unmodified_buf, buf, MAX_LINE);
      char* token = strtok(buf, " ");
      string command(token);

      if (command == "MSGGET\n") {
        if (motd.getAuthenticated()) {
          motd.buildMessage(buf);
        }
        else {
          string fail_str = "401, User not authenticated\n";
          char* failure = const_cast<char*>(fail_str.c_str());
          memcpy(buf, failure, MAX_LINE);
        }
        send (new_s, buf, MAX_LINE, 0);
      }
      else if (command == "QUIT\n") {
        motd.quit(response);
        send (new_s, response, MAX_LINE, 0);
        exit(0);
      }
      // a new line is not expected at the end of this command since additional arguments must be provided
      else if (command == "LOGIN") {
        // newline char delimits the end of the command provided by user
        char* args = strtok(unmodified_buf, "\n");
        motd.login(args, buf);
        send (new_s, buf, MAX_LINE, 0);
      }
      else if (command == "LOGOUT\n") {
        motd.logout(response);
        send (new_s, response, MAX_LINE, 0);
      }
      else if (command == "MSGSTORE\n") {
        // newline char delimits the end of the command provided by user
        char* args = strtok(unmodified_buf, "\n");
        bool authorized;
        motd.validateUser(buf) ? authorized = true : authorized = false;
        send (new_s, buf, MAX_LINE, 0);
        if (authorized) {
          cout << "Server: waiting for user input..." << endl;
          recv (new_s, buf, MAX_LINE, 0);
          cout << "Server: received message: " << buf << endl;
          motd.storeMessage(buf);
          send (new_s, buf, MAX_LINE, 0);
        }
      }
      else if (command == "SHUTDOWN\n") {
        bool authorized;
        motd.validateUser(buf) ? authorized = true : authorized = false;
        cout << buf << motd.getIsRoot() << endl;
        if (authorized == true && motd.getIsRoot()) {
          motd.shutdown(buf, new_s);
        }
        send (new_s, buf, MAX_LINE, 0);
        
      }
      else {
        cout << "Server: Command not recognized" << endl;
      }
      //flush the buffer
      memset(buf, 0, MAX_LINE);
		}

		close(new_s);
    }
} 
 
