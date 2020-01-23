# ping
A simple Ping program written in C.

# Build

```sh
$ gcc ping.c -o ping
```

# Usage
Because this program uses raw sockets it must be run as super user or with elevated rights.
```sh
$ sudo ./ping 8.8.8.8
Start pinging 8.8.8.8:
36 bytes from 8.8.8.8: icmp_seq=1 ttl=50 time=73.657990 ms
36 bytes from 8.8.8.8: icmp_seq=1 ttl=50 time=35.037041 ms
36 bytes from 8.8.8.8: icmp_seq=1 ttl=50 time=30.657053 ms
36 bytes from 8.8.8.8: icmp_seq=1 ttl=50 time=35.594940 ms
36 bytes from 8.8.8.8: icmp_seq=1 ttl=50 time=33.379078 ms
36 bytes from 8.8.8.8: icmp_seq=1 ttl=50 time=33.810854 ms
```
