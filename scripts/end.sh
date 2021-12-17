#!/bin/bash
{ cat end.json; cat; } |  socat -dd - TCP4:localhost:8888

