#!/bin/sh
cur_path=$(cd "$(dirname $0)";pwd)

process=opaldemo

pid=$(pidof ${process})
if [ $pid ]; then
  echo "${process} is already running, please check the process(pid: $pid) first."
  exit 1;
fi

pid=$(pidof ${process}_s)
if [ $pid ]; then
  echo "${process}_s is already running, please check the process(pid: $pid) first."
  exit 1;
fi

if [ -e /var/log/core* ]; then
  echo "exist coredump file under path: /var/log, please deal with coredump file first."
  exit 1;
fi

if [ $# -lt 2 ]; then
  echo "USAGE: $0 -s <sensor type> [-n <scenario>] [-l <log level>] [-t <log target>] [-h]"
  echo " e.g.: $0 -s 0                  -- Start with single os04a10"
  echo " e.g.: $0 -s 0 -n 1             -- Start with single os04a10"
  echo " e.g.: $0 -s 0 -n 0             -- Start with dual os04a10"
  echo " ---------------------------------------------------------------------------------------------------------------------------"
  echo "   s: sensor type."
  echo "      0: OS04A10 (DEFAULT)"
  echo "      1: SC200AI"
  echo "      2: SC450AI"
  echo "      3: SC500AI"
  echo "      4: SC850SL"
  echo "   n: scenario type."
  echo "      0: Dual sensor"
  echo "      1: Single sensor (DEFAULT)"
  echo "   l: log level, indicates the log level."
  echo "      ALERT = 1, CRITICAL = 2, ERROR = 3 (DEFAULT), WARN = 4, NOTICE = 5, INFO = 6, DEBUG = 7, DATA = 8"
  echo "   t: log target, indicates the log output targets."
  echo "      SYSLOG = 1, APPLOG = 2, STDOUT = 4 (DEFAULT) (Calculate the sum if multiple targets is required)"
  echo "   c: config path, indicates configure files top path."
  echo "      ./config (Default)"
  echo "   h: usage help"
  exit 1;
fi

#set -e
cd $cur_path

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$cur_path/lib

# Enable core dump
EnableCoreDump=0
# Run in background
RunInBackground=0

# Check whether start with gdb debug mode
if [[ $(expr match "$*" ".*-d.*") != 0 ]]
then
  debug="gdb --args"
else
  debug=""
fi

# Check whether start process linked by static lib
if [[ $(expr match "$*" ".*-a.*") != 0 ]]
then
  static_lib="_s"
else
  static_lib=""
fi

# Open core dump
if [ $EnableCoreDump == 1 ] ; then
  ulimit -c unlimited
  echo /opt/data/core-%e-%p-%t > /proc/sys/kernel/core_pattern
fi

md5=`md5sum ${process}${static_lib} | awk '{ print $1 }'`
echo "launching ${process}${static_lib}, md5: ${md5} ..."

# launch
if [ $RunInBackground == 1 ] ; then
  nohup $debug ./${process}${static_lib} "$*" &
else
  $debug ./${process}${static_lib} $*
fi
