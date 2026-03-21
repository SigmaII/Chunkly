Chunkly is a project born from the desire to write a tool capable of fast, chunk-based, and reliable file transfers.

It uses an L7 protocol written in C for file sharing. The general structure is shown below:

                                              
| Field                  | Size (bytes) | Description                  |
|------------------------|--------------|------------------------------|
| filename_length        | 4            | filename length (big endian) |
| filename               | N            | filename                     |
| compressed             | 4            | 0 or 1 (big endian)          |
| file_size              | 8            | file size (big endian)       |
| payload_size           | 8            | payload size (big endian)    |
| payload                | M            | payload                      |
                                              
- `filename_length` contains the filename's size in bytes, converted to big endian.
- `filename` contains the actual filename.
- `compressed` is a field that can be either 0 or 1. This depends on the type of file being sent during program execution.
Chunkly sends directories by compressing them in tar.gz (and changing the compressed value to 1) and decompressing them upon receipt by the server.
This ensures greater speed than reading and sending files recursively. Obviously, the larger the folder, the greater the advantage in terms of bytes.
- `file_size` contains the total size of the file in big endian. This is more useful to the server, which, upon a first request from the client, will send it the current size of the requested file. This is the core of the resume function, which allows uploading from the last uploaded chunk.
- `payload_size` contains the size of the chunk in big endian. Chunkly splits the file into chunks to speed up file delivery. Each chunk, after being sent, is removed from RAM. By default, any file smaller than 10GB is segmented into 5 chunks, to prevent the RAM used from exceeding 2GB.
- `payload` contains the chunk's data.

# Get started
Install Chunkly:

    ./start.sh

Get help:

    chunkly -h


