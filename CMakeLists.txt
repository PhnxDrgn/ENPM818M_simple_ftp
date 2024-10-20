# set project name
PROJECT(ftp)

# SERVER
# add executables
add_executable(ftp_server src/simple_ftp_server.c)
# add includes
target_include_directories(ftp_server PUBLIC inc)

# CLIENT
# add executables
add_executable(ftp_client src/simple_ftp_client.c)
# add includes
target_include_directories(ftp_client PUBLIC inc)