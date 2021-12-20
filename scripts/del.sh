#!/bin/bash
{ cat del.json; cat; } |  socat -dd - TCP4:localhost:8888
