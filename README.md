# client-server-socket

## Building

Because I make use of the unordered_map data structure in this program, this program should be compiled with g++ 9.4.0. During development of this program I was able to successfully use the Makefile template provided by the professor to build the binaries.

## Running

Run the `server` binary first in your terminal, then run the `client` binary. The client requires that a host address be provided as an argument. During development I used the localhost address `127.0.0.1` for testing.

## Description

I completed this project on my own.

This program demonstrates using web sockets in the client-server model. It implements a "yet another message of the day" program, where users can sign in and get authenticated and perform various functions.

In my implementation, I used the professor's server.c and client.c samples. Most of my changes are done on server.c, but there is some command validation and output that occurs in the main loop of the client. 

To store users, I used the unordered_map class to quickly look up users by their user name (key) and passwords. 

The bulk of my code is in the Server class. There are various functions and members of the class. Default values are initialzed in the constructor, such as the `isRoot` and `authenticated` boolean flags which are used for the CLI functions that require specific permissions, such as `SHUTDOWN`.

On the server, the main loop is similar to the client main loop in that the bulk of the code is just doing command validation and contains the bulk of the socket transactions (`send` and `recv`). Some logic is performed here to check that various conditions exist.

The messages of the day are stored in the Server object as a vector of strings. The `storeMessage` and `buildMessage` class members read and write to this vector. The logic for wrapping through the vector of messages is in `getMessageOfTheDay`.

Several of the command line functions use the `validateUser` function to check that the user issuing the commands have the appropriate permissions, and that the user has signed in to begin with. There are several helper functions within the class that make it simple to get and set different class attributes from within the main loop, such as `getIsRoot` and `setAuthenticated` which return the value of the isRoot flag and sets the user as authenticated, respectively.

Some of the output on the client are not received through the server socket connection but instead are within the client itself. Some of the validations made more sense to run on the client side, such as checking if the number of arguments passed with the `LOGIN` function were appropriate.

The char buffer `buf` is used primarily to pass information between the server, client, and functions within both programs. The buffer is flushed with each iteration of the while loop and re-written to, both on the client and server.

## Sample Outputs

Here is some output from the client program:

```
nate@nate-Z790-UD-AC ~/client-server-socket (main) $ ./client 127.0.0.1
MSGGET
ECHO: 401, User not authenticated

MSGSTORE
ECHO: 401, Permission denied

LOGIN root wrongpassword
ECHO: 401, Wrong UserID or Password
Authentication failed

LOGIN
ERROR: Username and password must be passed as arguments.

LOGIN john john01
ECHO: 200, Authentication success

MSGGET
ECHO: 200, OK
Never break your promises.

LOGOUT
ECHO: 200, OK

MSGSTORE
ECHO: 401, Permission denied

LOGIN root root01
ECHO: 200, Authentication success

MSGSTORE
200, User credentials valid

Client: Authorized, enter message
This is a really cool message of the day
ECHO: 200, message stored

MSGGET
ECHO: 200, OK
You are never as stuck as you think you are.

MSGGET
ECHO: 200, OK
Happiness is a choice.

MSGGET
ECHO: 200, OK
Habits develop into character.

MSGGET
ECHO: 200, OK
This is a really cool message of the day

LOGOUT
ECHO: 200, OK

SHUTDOWN
ECHO: 401, Permission denied

LOGIN root root01
ECHO: 200, Authentication success

SHUTDOWN
ECHO: 200, Shutting server down

Closing client.

```

And the corresponding server output

```
nate@nate-Z790-UD-AC ~/client-server-socket (main) $ ./server 
The server is up, waiting for connection
new connection from 127.0.0.1
200, OK
Never break your promises.

Server: waiting for user input...
Server: received message: This is a really cool message of the day

200, OK
You are never as stuck as you think you are.

200, OK
Happiness is a choice.

200, OK
Habits develop into character.

200, OK
This is a really cool message of the day

401, Permission denied
0

200, User credentials valid
Sever: Shutting down...

```

## TODOS and Bugs

Future work would include: 
- Adding a function for construction the response code and concatenating the `buf` message.
