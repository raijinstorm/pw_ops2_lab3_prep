L7: Literary Journal#

“Chronicle”, an established literary journal, is currently at a critical breaking point. Editors are overworked while handling submissions from just a single author. The owner has concluded that the problem lies in the submission and distribution system. The journal still relies on archaic infrastructure to handle its core business: receiving poetry and prose from authors and distributing it to readers.

For a start, you are tasked to create a system which will help a single editor (program user) work with a single author (netcat TCP server) and multiple readers (netcat TCP clients). The owner wants to overwork each editor with multiple authors, so make sure the system is scalable (by using a multiplexing mechanism).

The program takes 2 arguments: \(X\) (author port to connect to) and \(Y\) (port the readers should connect to).

Stages:#

Stage 1 (6 pts)#

Get in contact with the author and start receiving chapters. The author (a netcat server) will be listening on port \(X\). The program must connect to them and print all received characters to STDOUT. Use epoll, poll, or select to listen for incoming data. To reduce the number of system calls, always attempt to read at least READ_BUFF_SIZE bytes from the author, but do not block if less is available. The program only stops when the author closes the connection.

Stage 2 (7 pts)#

Time to share those brilliant pieces of literature with the readers, in a multicast fashion. Start listening for connections on port \(Y\); you can serve up to MAX_CLIENTS readers at a time (possibly more over the program’s runtime if some readers disconnect). Use the same multiplexing mechanism as in stage 1. The first reader is always the terminal — still print all received data.

Once a connection is established, the reader will receive all the data transmitted by the author, as if they were connected directly. Readers may disconnect at any time. A disconnection doesn’t interrupt receiving data from the author or sending to other readers. Don’t send data to disconnected readers.

Stage 3 (7 pts)#

Modern social media algorithms have made readers used to highly filtered and curated content. To stay relevant, implement an on-demand filtering system. Receive incoming data from the readers (from stdin too), attempt to do this in READ_BUFF_SIZE chunks too. The last letter (you can use the isalpha() function) received from the reader becomes its filter. Please notice that this means the filter may be updated at some point.

Readers with a configured filter will not receive the author’s data in its entirety. Instead, they will only receive non-overlapping substrings of length GREP_LEN (5), starting with the filter character. For example, if the author sends "aaabacccccahhhjioao\ncalhjkhj", a reader with filter 'a' shall receive "aaabaahhhjao\nca". If the substring is shorter than GREP_LEN, it must be completed and sent only when new data arrives.

Stage 4 (4 pts)#

The Rector announced rector’s hours. After receiving SIGINT, processing shall be stopped, but cleanup still must be performed. Print out “I cannot handle this anymore!!!” before exiting. Be mindful of possible race conditions!
