#!/bin/bash
for i in {1..5}
do
   cmd.exe /c start wsl ./write.sh
done
