# Summary
This example sends customized packets from one process to another: [TX Process] ------> [RX Process]

* The TX Process sends on port #0 and the RX Process receives on port #1. 
* The packets are sent with a proper ethernet header configured with the mac address of port #1 interface.
* Each process runs on a different core.

# Compilation
Both binaries should be compiled in the */examples* directory of the DPDK environment.
```sh
$ make
```

# Running

##### TX Process (DPDK_Sender):

```sh
$ sudo ./build/sender -l 0 --proc-type=auto --file-prefix lalal --socket-mem 400
```

##### RX Process (DPDK_Receiver):

```sh
$ sudo ./build/receiver -l 1 -c 2 --proc-type=auto --file-prefix lalal
```

