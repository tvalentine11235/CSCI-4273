DISTRIBUTED FILE SYSTEM
splits file into 4 pieces and puts 2 pieces on each server for rudundancy. the 2 pieces on each server are determined via a hashing function. pieces will be retrievable as long as only 1 server is down and possibly if 2 servers are down. All put requests are authenticated by the server against a username and password. if username and password do not match authorized user database no request can be performed.

TO RUN:
open 5 terminals. 
terminal 1 - navigate to client folder, make and run with ./dfc dfc.config
terminal 2-5 - navigate to server folder, make and run with ./dfs ./DFS[server number] [port number]

