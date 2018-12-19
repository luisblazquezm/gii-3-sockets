#!/bin/bash
#
# lanzaServidor.sh
# Lanza un servidor y varios clientes TFTP

./servidor
./cliente 127.0.0.1 TCP r fichero1.txt &
./cliente 127.0.0.1 TCP r fichero2.txt &
./cliente 127.0.0.1 TCP w fichero3.txt &
./cliente 127.0.0.1 UDP w fichero4.txt &
./cliente 127.0.0.1 UDP r fichero5.txt &
./cliente 127.0.0.1 UDP w fichero6.txt &
