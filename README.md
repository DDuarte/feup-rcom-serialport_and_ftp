rcom-2013
=========

FEUP: Computer Networks - EIC0032 - 2013/2014

Serial port "emulator":

```bash
sudo socat PTY,link=/dev/ttyS0 PTY,link=/dev/ttyS4
```

Usage:

```bash
proj1 send|recv <terminal_filename> <input_file|output_file> [options]
Configuration:
 -t 	 Set timeout value
 -r 	 Set number of retries
 -m 	 Set maximum information frame size
 -b 	 Set baudrate value
 -bcc1e 	 Set bcc1 error simulation probability (0 - 100)
 -bcc2e 	 Set bcc2 error simulation probability (0 - 100)
 -bccseed 	 Set bcc error simulation seed
 -v 	 Set verbose output
 -csi 	 Print CSI (connection statistical information)
```

Example:

```
sudo ./proj1 /dev/ttyS0 fileout
sudo ./proj1 /dev/ttyS4 filein
```

*Whatever*
