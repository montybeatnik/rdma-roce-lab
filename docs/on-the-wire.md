# Get Captures

## Change from nano to vim
If you're like me and prefer vim to nano, run the following in the VM: 

```bash
sudo update-alternatives --config editor
```

You'll get the following:
```bash
There are 4 choices for the alternative editor (providing /usr/bin/editor).

  Selection    Path                Priority   Status
------------------------------------------------------------
* 0            /bin/nano            40        auto mode
  1            /bin/ed             -100       manual mode
  2            /bin/nano            40        manual mode
  3            /usr/bin/vim.basic   30        manual mode
  4            /usr/bin/vim.tiny    15        manual mode

Press <enter> to keep the current choice[*], or type selection number: 3
update-alternatives: using /usr/bin/vim.basic to provide /usr/bin/editor (editor) in manual mode
```

## No password for sudo 
This is ***NOT*** something you would ever do in prod, but this is a lab. 

```
sudo visudo 
# add this line to the bottom
# ${USER}   ALL=(ALL) NOPASSWD: ALL
## where ${USER} is your username; for example:
chern    ALL=(ALL) NOPASSWD: ALL
```

## Try to get a capture
```bash
ssh ubuntu@192.168.2.38 'sudo tcpdump -U -nni enp0s1 -w - not port 22' | wireshark -k -i -
# or if you're using name resolution (the script should have updated the /etc/hosts file)
```

## Wireshark not found
```bash
➜  rdma-roce-lab git:(refactor-for-blog) ✗ ssh ubuntu@192.168.2.38 'sudo tcpdump -U -nni enp0s1 -w - not port 22' | wireshark -k -i -
zsh: command not found: wireshark
```

If the following opens wireshark
```bash
open /Applications/Wireshark.app  
```

Run this:
```bash
export PATH="/Applications/Wireshark.app/Contents/MacOS:$PATH"
```

## Interesting Capture filters
```
# RoCE v2 uses UDP port 4791, so to capture all RoCE packets:
udp.port == 4791
# CM MAD packets for setup/teardown 
infiniband.mad.method == 0x03
```
