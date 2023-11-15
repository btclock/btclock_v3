#!/bin/bash

data_directory="data/build"

# Use find to list all files in the directory (including hidden files), sort them, and then calculate the hash
#hash=$(find "$data_directory" -type f \( ! -iname ".*" \) | LC_ALL=C  sort | xargs cat  | shasum -a 256 | cut -d ' ' -f 1)
hash=$(find "$data_directory" -type f \( ! -iname ".*" \) | LC_ALL=C sort | xargs -I {} cat {} | shasum -a 256 | cut -d ' ' -f 1)

echo "Hash of files in $data_directory: $hash"