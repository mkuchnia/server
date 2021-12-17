#!/bin/bash
#{ cat alice.json; cat; } | netcat localhost 8888 -v
#netcat localhost 8888 < alice.json
#{ cat alice.json; } | telnet localhost 8888
socat -dd - TCP4:localhost:8888