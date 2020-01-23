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
```
