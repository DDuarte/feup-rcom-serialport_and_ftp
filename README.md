rcom-2013
=========

FEUP: Computer Networks - EIC0032 - 2013/2014

### Project 1: Send/receive file through serial port

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

```bash
sudo socat PTY,link=/dev/ttyS0 PTY,link=/dev/ttyS4 # serial port "emulator"
sudo ./proj1 recv /dev/ttyS0 fileout
sudo ./proj1 send /dev/ttyS4 filein
```

### Project 2: Simple FTP client (downloads a file)

Usage:

```bash
./download ftp://[<user>:<password>@]<host>[:port]/<url-path>
```

Example:

```bash
./download ftp://ftp.fe.up.pt/welcome.msg # file is saved to current working dir with filename welcome.msg
```
