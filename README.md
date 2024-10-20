# ENPM818M Week 7 Assignment Simple FTP
## Building
CMake is used to build from the source code. You can run `./build_simple_ftp.sh` from the root of the repo to build the project with CMake.

## Server
The server can be started by running `./build/ftp_server` from the root of this repo.

## Client
The client can be started by running `./build/ftp_client <server ip> <src file on server> <destination for client>`.

## Test
Start server with: `./test_server_start.sh`.

The server script should delete `dst_file.txt` then start the server.

In another terminal, run `./test_client.sh`.

The client script should connect to the server to get `src_file.txt` and save it to `dst_file.txt`

The server should stay up until you close it with `ctrl+c`