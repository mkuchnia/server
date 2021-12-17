#!/bin/bash
{ cat stat.json; cat; } |  socat -dd - TCP4:localhost:8888
