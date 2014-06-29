#!/bin/sh

for dir in case-*; do
  printf "testing $dir ... "
  cd $dir
  make -s
  ./trie_search_test
  if [ $? -eq 0 ]; then
    echo "ok"
  else
    echo "failed"
    exit 1
  fi
  cd ..
done

echo "all tests passed"
