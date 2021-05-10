
const { networkInterfaces } = require('os');

const dgram = require('dgram');
const net = require('net');




/*

vorpal
	.command('data [action]', 'Turns data transmissions, both upload and download. Action = [on|off] or omit to toggle.')
	.action(function(args, callback) {
		handleAction(args.action, 'data');
		this.log('data ' + (modes.data ? 'on' : 'off'));
		
		callback();			
	});

vorpal
	.command('disconnect', 'Disconnect cloud connections.')
	.action(function(args, callback) {
		if (connections.length > 0) {
			this.log('disconnecting ' + connections.length + ' connections');
			while(connections.length > 0) {
				var conn = connections.pop();
				conn.client.destroy();
				conn.conn.destroy();
			}
		}
		else {
			this.log('disconnect - no connections');			
		}
		callback();			
	});

vorpal
	.command('latency [ms]', 'Simulate a high-latency network like satellite.')
	.action(function(args, callback) {
		if (args.ms) {
			modes.latency = parseInt(args.ms);
		}
		else {
			modes.latency = 0;
		}
		this.log('latency ' + modes.latency + ' ms');
		callback();			
	});

vorpal
	.command('loss [pct]', 'Randomly lose pct percent of packets (0 <= pct <= 100)')
	.action(function(args, callback) {
		if (args.pct) {
			modes.loss = parseInt(args.pct);
			if (modes.loss < 0) {
				modes.loss = 0;
			}
			if (modes.loss > 100) {
				modes.loss = 100;
			}
		}
		else {
			modes.loss = 0;
		}
		this.log('loss ' + modes.loss + '%');
		callback();			
	});


vorpal
	.delimiter('$')
	.show();

class Device {
	constructor(addr, port) {
		this.addr = addr;
		this.port = port;
		
		console.log(`new device ${addr}:${port}`);
		
		this.socket = dgram.createSocket('udp4');
		
		this.socket.on('error', (err) => {
			console.log(`from cloud error:\n${err.stack}`);
			server.close();
		});

		this.socket.on('message', (msg, rinfo) => {
			// console.log(`from cloud: ${msg} from ${rinfo.address}:${rinfo.port}`);
			
			if (modes.data && !losePacket()) {
				if (modes.latency == 0) {
					console.log('< ' + msg.length);
					server.send(msg, this.port, this.addr);
				}
				else {
					var port = this.port;
					var addr = this.addr;
					
					console.log('< ' + msg.length + ' queued');
					setTimeout(function() {
						console.log('< ' + msg.length);
						server.send(msg, port, addr);					
					}, modes.latency)
				}
			}
			else {
				console.log('< ' + msg.length + ' discarded');				
			}
		});

		this.socket.on('listening', () => {
			const address = this.socket.address();
			console.log(`device specific cloud port listening ${address.address}:${address.port}`);
		});
		this.socket.bind();
	}
	
	send(msg) {
		// Send to the real UDP device service from our client-specific socket
		this.socket.send(msg, dsPort, dsAddr);
	}
}
var devices = [];

function getDeviceForDeviceAddrPort(addr, port) {
	for(var d of devices) {
		if (d.addr == addr && d.port == port) {
			return d;
		}
	}
	var d = new Device(addr, port);
	devices.push(d);
	return d;
}

function losePacket() {
	if (modes.loss > 0) {
		return (Math.random() * 100) < modes.loss;
	}
	else {
		return false;
	}
}

server.on('error', (err) => {
	console.log(`server error:\n${err.stack}`);
	server.close();
});

server.on('message', (msg, rinfo) => {
	// console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);

	var d = getDeviceForDeviceAddrPort(rinfo.address, rinfo.port);
	if (modes.data && !losePacket()) {
		if (modes.latency == 0) {
			console.log('> ' + msg.length);
			d.send(msg);
		}
		else {
			console.log('> ' + msg.length + ' queued');
			setTimeout(function() {
				console.log('> ' + msg.length);
				d.send(msg);					
			}, modes.latency);	
		}
	}
	else {
		console.log('> ' + msg.length + ' discarded');
	}
});

server.on('listening', () => {
	const address = server.address();
	console.log(`server listening ${address.address}:${address.port}`);
});

server.bind(dsPort);

function handleAction(arg, sel) {
	if (arg === 'on') {
		modes[sel] = true;
	}
	else
	if (arg === 'off') {
		modes[sel] = false;		
	}
	else {
		modes[sel] = !modes[sel];
	}
}
*/

(function(cloudManipulator) {

    cloudManipulator.modes = {
        data:true,
        latency:0,
        loss:0,
        loseFromDevice:0,
        loseToDevice:0
    };

    cloudManipulator.reset = function() {
        cloudManipulator.modes.data = true;
        cloudManipulator.modes.latency = 0;
        cloudManipulator.modes.loss = 0;
        cloudManipulator.modes.loseFromDevice = 0;
        cloudManipulator.modes.loseToDevice = 0;
    };

    cloudManipulator.setData = function(on) {
        cloudManipulator.modes.data = on;
        console.log('cloud manipulator setData=' + on);
    };

    cloudManipulator.setLoseFromDevice = function(num) {
        cloudManipulator.modes.loseFromDevice = num;
        console.log('cloud manipulator loseFromDevice=' + num);
    };

    cloudManipulator.setLoseToDevice = function(num) {
        cloudManipulator.modes.loseToDevice = num;
        console.log('cloud manipulator loseToDevice=' + num);
    };

    cloudManipulator.device = function(addr, port) {
        let device = {};
        device.addr = addr;
        device.port = port;

        device.send = function(msg) {
            // Send to the real UDP device service from our client-specific socket
            device.socket.send(msg, cloudManipulator.config.dsPort, cloudManipulator.config.dsAddr);
        }

        device.init = function() {
            console.log(`new device ${addr}:${port}`);
        
            device.socket = dgram.createSocket('udp4');
            
            device.socket.on('error', (err) => {
                console.log(`from cloud error:\n${err.stack}`);
                cloudManipulator.server.close();
            });
    
            device.socket.on('message', (msg, rinfo) => {
                // console.log(`from cloud: ${msg} from ${rinfo.address}:${rinfo.port}`);
                
                if (cloudManipulator.modes.data && !cloudManipulator.losePacket() && !cloudManipulator.loseToDevice()) {
                    if (cloudManipulator.modes.latency == 0) {
                        console.log('< ' + msg.length);
                        cloudManipulator.server.send(msg, device.port, device.addr);
                    }
                    else {
                        var port = device.port;
                        var addr = device.addr;
                        
                        console.log('< ' + msg.length + ' queued');
                        setTimeout(function() {
                            console.log('< ' + msg.length);
                            cloudManipulator.server.send(msg, port, addr);					
                        }, cloudManipulator.modes.latency)
                    }
                }
                else {
                    console.log('< ' + msg.length + ' discarded');				
                }
            });
    
            device.socket.on('listening', () => {
                const address = device.socket.address();
                console.log(`device specific cloud port listening ${address.address}:${address.port}`);
            });
            device.socket.bind();    
        }

        device.init();

        return device;
    };

    cloudManipulator.devices = [];

    cloudManipulator.getDevice = function(addr, port) {
        for(let d of cloudManipulator.devices) {
            if (d.addr == addr && d.port == port) {
                return d;
            }
        }
        let d = cloudManipulator.device(addr, port);
        cloudManipulator.devices.push(d);
        return d;
    };

    cloudManipulator.losePacket = function() {
        if (cloudManipulator.modes.loss > 0) {
            return (Math.random() * 100) < cloudManipulator.modes.loss;
        }
        else {
            return false;
        }
    };

    cloudManipulator.loseFromDevice = function() {
        if (cloudManipulator.modes.loseFromDevice > 0) {
            cloudManipulator.modes.loseFromDevice--;
            return true;
        }
        else {
            return false;
        }
    };
    
    cloudManipulator.loseToDevice = function() {
        if (cloudManipulator.modes.loseToDevice > 0) {
            cloudManipulator.modes.loseToDevice--;
            return true;
        }
        else {
            return false;
        }
    };

    cloudManipulator.run = function(config) {

        cloudManipulator.config = config;

        if (!cloudManipulator.config.dsAddr) {
            console.log('config.dsAddr not set to device service address');
            return false;
        }
        if (!cloudManipulator.config.dsPort) {
            cloudManipulator.config.dsPort = 5684;
        }

        cloudManipulator.server = dgram.createSocket('udp4');
    
        cloudManipulator.localAddr = null;
        {
            const intf = networkInterfaces();
            const intfResults = {};
        
            for (const intfName in intf) {
                for (const net of intf[intfName]) {
                    if (net.family === 'IPv4' && !net.internal) {
                        if (!intfResults[intfName]) {
                            intfResults[intfName] = [];
                        }
                        intfResults[intfName].push(net.address);
                    }
                }
            }
        
            let intfName = cloudManipulator.config.intf;
        
            if (intfName && intfResults[intfName]) {
                // Use config-specified interface
            }
            else {
                const numIntf = Object.keys(intfResults).length;
                if (numIntf == 0) {
                    console.log('no viable interfaces found', intf);
                    return false;
                }
                else
                if (numIntf > 1) {
                    console.log('more than one viable interface', intfResults);
                    return false;
                }
            
                intfName = Object.keys(intfResults)[0];
            }
        
            localAddr = intfResults[intfName][0];
            console.log('using ' + intfName + ' ' + localAddr);

            cloudManipulator.server.on('error', (err) => {
                console.log(`server error:\n${err.stack}`);
                cloudManipulator.server.close();
            });
    
            cloudManipulator.server.on('message', (msg, rinfo) => {
                // console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);
    
                var d = cloudManipulator.getDevice(rinfo.address, rinfo.port);
                if (cloudManipulator.modes.data && !cloudManipulator.losePacket() && !cloudManipulator.loseFromDevice()) {
                    if (cloudManipulator.modes.latency == 0) {
                        console.log('> ' + msg.length);
                        d.send(msg);
                    }
                    else {
                        console.log('> ' + msg.length + ' queued');
                        setTimeout(function() {
                            console.log('> ' + msg.length);
                            d.send(msg);					
                        }, cloudManipulator.modes.latency);	
                    }
                }
                else {
                    console.log('> ' + msg.length + ' discarded');
                }
            });
    
            cloudManipulator.server.on('listening', () => {
                const address = cloudManipulator.server.address();
                console.log(`server listening ${address.address}:${address.port}`);
                cloudManipulator.readyResolve();
            });
    
            cloudManipulator.server.bind(cloudManipulator.config.dsPort, localAddr);    
        }

        return new Promise((resolve, reject) => {
            cloudManipulator.readyResolve = resolve;
        });
    };

    
    


}(module.exports));

