#!/bin/bash
{ cat get.json; cat; } |  socat -dd - TCP4:localhost:8888
