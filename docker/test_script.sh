#!/usr/bin/env bash

storm-tape-poc/build/storm-tape &
sleep 5
response_code=`curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/.well-known/wlcg-tape-rest-api`
if [$response_code != '200']
then
    exit 1
fi
exit 0