# Luring
Luring is a callback-style interface for Lua to "io_uring" which is the asynchronous I/O framework introduced in Linux Kernel 5.1.

Luring uses liburing internally, you sould have installed liburing to compile it.

## Usage
````
local luring = require "luring"
````
Luring deal with file descriptors. You could use libraries like "luaposix" to get descriptors, which luring could deal with. Besides, luring provide a simple helper `open_file(file_path, mode)` to open regular file.

To use luring, you should have knownledge about how io_uring works, here're some resources:
- [Ringing in a new asynchronous I/O API By Jonathan Corbet - LWN.net](https://lwn.net/Articles/776703/)
- [An Introduction to the io_uring Asynchronous I/O Framework By Bijan Mottahedeh - Oracle Linux Blog](https://blogs.oracle.com/linux/an-introduction-to-the-io_uring-asynchronous-io-framework)

In short, it is just 4 step to correctly handle I/O actions by luring:
1. Do actions by functions provided by luring, such as `write(ring, fd, content, offest, callback)`, `read(ring, fd, size, offest, callback)`, `recv(ring, sockfd, size, flags, callback)`, `send(ring, sockfd, content, flags, callback)`, `accept(ring, sockfd, flags, callback)`
2. Submit. `submit(ring)` or `submit_and_wait(ring, wait_nr)`.
3. Handle CQEs. This step you use `do_cqes(ring)`, which will block the thread and call the callbacks until no CQEs leaves in ring.
4. Free resources. The first argument of callbacks is a userdata, which is a pointer to CQE. You MUST call `cqe_seen(ring, cqe)` on the userdata in the end of your callback, or the callback will be called repeatly and the other CQEs could not be processed.

- [A basic example to read and write regular file](example/example.lua)
- [Network programming example](example/net.lua)

## License
Apache License, version 2.0

https://www.apache.org/licenses/LICENSE-2.0