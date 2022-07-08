#!/bin/bash

curl "ftp://$1:1337/ux0:/data/"$(curl "ftp://$1:1337/ux0:/data/" 2>/dev/null | grep psp2core | tail -1 | awk '{print $(NF)}') --output coredump
