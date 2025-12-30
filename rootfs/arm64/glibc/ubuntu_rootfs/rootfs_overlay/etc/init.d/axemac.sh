#!/bin/sh

# enable RFS&RPS
echo 3 > /sys/class/net/eth0/queues/rx-0/rps_cpus
echo 32768 > /proc/sys/net/core/rps_sock_flow_entries
echo 32768 > /sys/class/net/eth0/queues/rx-0/rps_flow_cnt
echo 200000 > /sys/class/net/eth0/queues/tx-0/byte_queue_limits/limit_min

# enable rx pause frame
ethtool -A eth0 rx on
