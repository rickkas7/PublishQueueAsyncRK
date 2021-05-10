# Automated Test - PublishQueuePosixRK

This is the automated test tool for the library. It's kind of difficult to use, but it has a 
number of useful features you might want to adapt for your own projects:

- The Particle device runs special firmware that can run tests, controlled by USB serial
- The node.js test tool monitors the USB serial output of the Particle device to look for logging messages
- The test tools sends commands over USB serial to control the on-device behavior
- It connects to the Particle cloud server-sent-events stream for the device to monitor published events
- It also acts as a device service, monitoring cloud data transmission and causing data loss as needed for some tests



### Config File

#### dsAddr

```
dig 1.udp.particle.io
```

### Setting up the device


The first thing you need to do is change your device to point at the server. 

- Note the IP address of the computer you're running the node.js server on.
- Put the device in DFU mode (blinking yellow)
- Run the command:

```
particle keys server --protocol udp --host 24.92.248.215 --port 5684 ec.pub.der
```

The ec.pub.der file is the public server key of the real Particle cloud server. It's included in the automated-test directory so you don't need to download it separately.

Replace 65.19.178.42 with the IP address of your server. Note that this must be a public IP address so the device can see it; it can't be an internal 192.168.x.x or 10.x.x.x address! You can use UDP port forwarding from your router to an internal address for port 5684.



## Restore your device

To put the device back to normal cloud mode run the command with no additional options.

```
particle keys server
```





