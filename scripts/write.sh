#!/bin/bash
{ cat write.json; cat; } |  socat -dd - TCP4:localhost:8888
