# USELESS
USELESS {aka cron} - launch commands at hosts on schedule

At all servers launch: 'my_server port'

At client launch: 'mycrons mycronfile hostfile'

To stop daemon 'prog_name stop'

At mycronfile write strings like:
min hour day month host command

Avaiable time combinations are:
min hour day month 
* \* \* \* \*
* \* \* \* n
* \* \* m n
* \* k m n
* a b c d

where \* means all allowed combinations for this measure

Host can be 'localhost' or 'all' or defined in hostfile

At host file should be written known hosts:
hostname ip port passwd

Files:
- mycrons.cpp & includes.h — main function
- reader.cpp & reader.h — functions for file reading
- execution.cpp & execution.h — function for execute commands
- my_server.cpp — server part
