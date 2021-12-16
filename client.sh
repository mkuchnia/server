#!/bin/bash
#{ cat alice.json; cat; } | netcat localhost 8888
#netcat localhost 8888 < alice.json
{ cat alice.json; } | telnet localhost 8888