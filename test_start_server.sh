#!/bin/bash

destination_file=dst_file.txt

if [ -f $destination_file ]; then
    rm $destination_file
fi

sudo ./build/ftp_server