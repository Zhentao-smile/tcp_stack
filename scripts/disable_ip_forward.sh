#!/bin/bash

iptables -A INPUT -p ipv4 -j DROP
echo 0 > /proc/sys/net/ipv4/ip_forward

