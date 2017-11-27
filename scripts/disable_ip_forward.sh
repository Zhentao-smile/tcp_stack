#!/bin/bash

# Archlinux
iptables -A INPUT -p ipv4 -j DROP
# Ubuntu
#iptables -A INPUT -p ip -j DROP
echo 0 > /proc/sys/net/ipv4/ip_forward

