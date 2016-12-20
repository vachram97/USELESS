# USELESS
USELESS {aka cron} - launch commands at hosts on schedule

This project consists of client part and server part.

## Server part

Files: my_server.cpp  
Compilation string: `g++ -W -Wall my_server.cpp -o useless_server -lcrypt`  
Launch command: `useless_server port`   
Stop command: `useless_server -s`  

## Client part

Files:  
- mycrons.cpp & includes.h — main function
- reader.cpp & reader.h — functions for file reading
- execution.cpp & execution.h — function for execute commands

Compilation:
```
g++ -c reader.cpp
g++ -c execution.cpp
g++ mycrons.cpp reader.o execution.o -o useless_client -lcrypt
```

Launch command: `useless_client mycronfile hostfile`  
Stop command `useless_client -s`  
Receiving logs command: `useless_client -r`

Mycronfile should contain following type of strings (each contains at maximum `MAX_LINE_LENGTH_ALLOWED = 500` chars:
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

Host file should contain strings(each contains at maximum `MAX_LINE_LENGTH_ALLOWED = 500` chars) like:
```
hostname ip port passwd
```  
where hostname couldn't be `localhost` or `all`.

## Log files

Program  creates log files called `host.log`, `error.log` on client computer and `localhost.log` on servers.

