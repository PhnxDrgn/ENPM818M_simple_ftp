#!/bin/bash

source_file=$PWD/src_file.txt
destination_file=dst_file.txt

if [ -f $destination_file ]; then
    rm $destination_file
fi

./build/ftp_client 127.0.0.1 $source_file $destination_file