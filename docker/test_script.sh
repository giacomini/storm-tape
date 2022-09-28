#!/usr/bin/env bash
ls 
storm-tape &
job_id=$!
sleep 5
response_code=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/.well-known/wlcg-tape-rest-api)
kill $job_id
wait
[ $response_code -eq 200 ]