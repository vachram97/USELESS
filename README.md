# USELESS
USELESS {aka cron} - launch commands at hosts on schedule

This project consists of client part and server part.

## Server part

Files: my_server.cpp  
Compilation string: `g++ -W -Wall my_server.cpp -o useless_server`  
Launch command: `useless_server port`  
In directory with program should be located txt file "hash.txt" with SHA-512 hash of passwd to verify clients trying to connect with server  
Stop command: `useless_server stop`  

## Client part

Files:  
- mycrons.cpp & includes.h — main function
- reader.cpp & reader.h — functions for file reading
- execution.cpp & execution.h — function for execute commands

Compilation strings:  
```
g++ -o execution.cpp  
g++ -o reader.cpp  
g++ -c mycrons.cpp execution.o reader.o -o useless_client
```  

Launch command: `useless_client mycronfile hostfile`

Stop command `useless_client stop`

Mycronfile should contain following type of strings:
```
min hour day month host command
```
Avaiable time combinations are:
```
* * * *
* * * n
* * m n
* k m n
a b c d
```   
where \* means all allowed combinations for this measure

Host can be `localhost` or `all` or defined in hostfile

Host file should contain strings like:
```
hostname ip port passwd
```  
where hostname couldn't be `localhost` or `all`.

## Log files

Program  creates log files called `host.log`, `error.log` on client computer and `host-full.log` on servers.

