#!/bin/bash
{ cat inc.json; cat; } |  socat -dd - TCP4:localhost:8888
