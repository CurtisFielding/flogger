Causes kernel panics at the moment; do not use. 

Short term plans include replacing the read syscall with a version of my own: one that writes all passed buffers to a log file somewhere. 

Possible near future plans include sending all of this data to a remote machine, perhaps via UDP or other.
