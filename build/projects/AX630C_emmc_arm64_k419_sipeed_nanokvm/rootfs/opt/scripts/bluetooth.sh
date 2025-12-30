#!/bin/bash

# Note: This command will enable hardware flow control on ttyS5, while disabling hardware flow control on ttyS0 ~ ttyS4.
devmem 0x04870038 32 0x00000400

hciattach -n /dev/ttyS5 any 1500000 &
hciconfig hci0 up