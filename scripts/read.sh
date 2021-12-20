#!/bin/bash
{ cat read.json; cat; } |  socat -dd - TCP4:localhost:8888
