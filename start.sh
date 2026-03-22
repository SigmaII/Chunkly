#!/bin/bash

set -e
set -o errtrace

function errtrap {
   echo $?
   RED='\033[0;31m'
   echo -e "\n\n\n${RED} ---- ERROR: unexpected termination! ----\n"
   exit 1
}

trap errtrap ERR

display_help() {
echo "
USAGE: start.sh 

description     This script will install chunkly system 
                
flags
                --only-server:   install chunkly-server
                --only-client:   install chunkly client  
                --upgrade        upgrade to latest version from github (client and server)
                --uninstall      uninstall chunkly
"
}
#shift $((OPTIND -1))

function install_chunkly() {

    mkdir -p /etc/chunkly-server/bin
    mkdir .chunkly
    git clone https://github.com/SigmaII/Chunkly.git .chunkly
    cd .chunkly/server
    make
    cp chunkly-server /etc/chunkly-server/bin
    cd ../client
    make
    cp chunkly /usr/local/bin
    cd ../systemd-service
    cp chunkly-server.service  /etc/systemd/system/
    systemctl daemon-reload
    systemctl start chunkly-server

    rm -r ../../.chunkly
}

function install_server() {

    mkdir -p /etc/chunkly-server/bin
    mkdir .chunkly
    git clone https://github.com/SigmaII/Chunkly.git .chunkly
    cd .chunkly/server
    make
    cp chunkly-server /etc/chunkly-server/bin
    cd ../systemd-service
    cp chunkly-server.service  /etc/systemd/system/
    systemctl daemon-reload
    systemctl start chunkly-server

    rm -r ../../.chunkly
}

function install_client() {

    mkdir .chunkly
    git clone https://github.com/SigmaII/Chunkly.git .chunkly
    cd .chunkly/client
    make
    cp chunkly /usr/local/bin

    rm -r ../../.chunkly
}

function install_upgrade() {

    rm -f /etc/chunkly-server/bin/chunkly-server
    rm -f /usr/local/bin/chunkly
    mkdir .chunkly
    git clone https://github.com/SigmaII/Chunkly.git .chunkly
    cd .chunkly/server
    make
    cp chunkly-server /etc/chunkly-server/bin
    cd ../client
    make
    cp chunkly /usr/local/bin
    cd ../systemd-service
    cp chunkly-server.service  /etc/systemd/system/
    systemctl daemon-reload
    systemctl start chunkly-server

    rm -r ../../.chunkly
}

function uninstall_chunkly() {

  rm -r /etc/chunkly-server || true
  rm -f /usr/local/bin/chunkly || true
  rm -f /etc/systemd/system/chunkly-server.service || true
}

if [ $# -eq 0 ]; then
  install_chunkly
  exit 0
fi

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help)
      display_help
      exit 0
      ;;
    --only-server)
      install_server
      shift
      ;;
    --only-client)
      install_client
      shift
      ;;
    --upgrade)
      install_upgrade
      shift
      ;;
    --uninstall)
      uninstall_chunkly
      shift
      ;;
  esac
done

#install_chunkly
