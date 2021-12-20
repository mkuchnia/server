#!/bin/bash
{ cat sleep.json; cat; } |  socat -dd - TCP4:localhost:8888
