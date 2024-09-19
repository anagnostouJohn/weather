#!/bin/bash

IP="192.168.1.114"
SUBJECT_CA="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=CA/CN=$IP"
SUBJECT_SERVER="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=Server/CN=$IP"
SUBJECT_CLIENT="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=Client/CN=$IP"

function generate_CA () {
   echo "$SUBJECT_CA"
   openssl req -x509 -nodes -sha256 -newkey rsa:2048 -subj "$SUBJECT_CA"  -days 365 -keyout ca.key -out ca.crt
}

function generate_server () {
   echo "$SUBJECT_SERVER"
   openssl req -nodes -sha256 -new -subj "$SUBJECT_SERVER" -keyout servermain.key -out servermain.csr
   openssl x509 -req -sha256 -in servermain.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out servermain.crt -days 365
}

function generate_client () {
   echo "$SUBJECT_CLIENT"
   openssl req -new -nodes -sha256 -subj "$SUBJECT_CLIENT" -out clientsse.csr -keyout clientsse.key 
   openssl x509 -req -sha256 -in clientsse.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out clientsse.crt -days 365
}

# generate_CA
generate_server
# generate_client
