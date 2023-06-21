#!/usr/bin/env bash

cat > storm-tape.conf <<EOF
storage-areas:
- name: sa1
  root: /tmp
  access-point: /tmp
EOF

storm_tape=$(command -v storm-tape)
if [ $? -ne 0 ]; then
  storm_tape=$(command -v storm-tape-debug)
  if [ $? -ne 0 ]; then
    echo "no storm-tape executable available" >&2
    exit $?
  fi
fi

${storm_tape} &
job_id=$!
sleep 5
response_code=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/favicon.ico)
kill $job_id
wait
[ $response_code -eq 204 ]
