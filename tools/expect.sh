#!/usr/bin/expect

spawn make qemu
expect "$ " { send "sh < test.sh\n" }
interact
