#!/bin/bash

usage() {
    echo "usage: $0 <configs file> <key>"
    echo "notes: the configuration file uses the format key1=value key2=value, with each key-value pair on a separate line."
    exit 1
}

if [ $# -ne 2 ]; then
    usage
fi

CONFIG_FILE="$1"
KEY="$2"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "error: config file '$CONFIG_FILE' is  not exist"
    exit 2
fi

grep -E "^\s*${KEY}=" "$CONFIG_FILE" \
    | sed -E 's/^\s*'"$KEY"'=//; s/\s*$//' \
    | sed 's/\s*\/\/.*$//' \
    | sed 's/\s*\/\*.*\*\/\s*//' \
    | head -n 1
