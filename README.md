Chunkly is a program written in C that use a L7 protocol to transfer files and folder between client and server.
The structure of the protocol:
                                              
| Field                  | Size (bytes) | Description                  |
|------------------------|--------------|----------------------------  |
| filename length        | 4            | filename length (big endian) |
| filename               | N            | filename                     |
| compressed             | 4            | can be 0 or 1                |
| file size              | 8            | file size (big endian)       |
| payload size           | 8            | payload size (big endian)    |
| payload                | M            | payload                      |

The tool's main function is to segment the file into 5 chunks by default, or one chunk if the file size exceeds 2GB, thus preserving RAM usage. 
After a segmented entry, the file is sent to the destination server and then reassembled. 
The tool's unique feature is that it summarizes the upload from the last chunk sent in case of interruption. 
Currently, the connection between client and server is not encrypted.                                     
