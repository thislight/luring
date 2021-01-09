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

## References
````lua
local luring = require "luring"
````
### `luring.open_file(file_path: string, mode: string?) : integer`
Open file and return the file descriptor. If `mode` is not presented, it's `w`.

`mode` accepts one of these options:
- `w`: open the file in read/write mode, create the file if it does not exists
- `w+`: same as `w`, but it will clear original content of the file
- `a`: open the file in write only mode, create the file if it does not exists
- `a+`: same as `a`, but it will clear original content of the file
- `r`: open the file in read only mode

The created file's permission will be `-rw-rw-r--.`.

It will return `-errno` when fails.

### `luring.strerror(errno: integer): string`
Return the explain of the `errno`.

### `luring.queue_init(entries: integer, flags: integer): userdata`
Create a new io_uring instance and return the pointer. It is always refered as `ring` later.
`entries` is the size of the ring, `flags` should be `0` for now. The userdata is set to automatically call `queue_exit` when is being collected as garbage.

### `luring.queue_exit(ring: userdata)`
Free the `ring`.

### `luring.submit(ring: userdata)`
Submit the SQEs you just changed. Note that it will make a system call.

### `luring.submit_and_wait(ring: userdata, wait_nr: integer)`
Submit the SQEs and wait until `wait_nr` CQEs filled in queue (after these SQEs' work done). Note that it will make a system call.

### `luring.do_cqes(ring: userdata)`
Do callbacks of finished works.

### `luring.cqe_seen(ring: userdata, cqe: userdata)`
This function MUST be called in the end of your callbacks. You can treat this function as that it marks the `cqe` is usable for next result.

Beside `io_uring_cqe_seen`, this function also unrefer the callback and free the memory of the buffer. The `result` you got will not be freed, because it's a copy of the result (Lua always do copy when using `lua_pushlstring`).

### I/O Actions
These actions' effect is closed to system call with same name. They are accept a function as a callback, the first argument of the callback is always the CQE (userdata), the second is always the result code. Genernally the negative result code means error, and you may get the exact error number from their negative (`-result_code`).

If the io_uring queue is full, `nil` will be returned, otherwise a userdata of the SQE will be returned.

Luring will do `malloc` for every needed actions (`read`, `recv` and `accept`. They need buffer to store data.). It might costs. You could use custom allocator to override the standard manual memory management functions, for example "mimalloc".

#### `luring.write(ring: userdata, fd: integer, content: string, offest: integer, callback: (function(cqe: userdata, res: integer): nil)?): userdata`
Write `content` to `fd` depends on `offest`.

#### `luring.read(ring: userdata, fd: integer, size: integer, offest: integer, callback: (function(cqe: userdata, res: integer, result: string): nil)?): userdata`
Read a chunk with `size` max size from `fd` depends on `offest`. `result` is the result.

#### `luring.recv(ring: userdata, sockfd: integer, size: integer, flags: integer, callback: (function(cqe: userdata, res: integer, result: string): nil)?): userdata`
Recvive data from `sockfd` with `size` max size. `result` is the result.

#### `luring.send(ring: userdata, sockfd: integer, content: string, flags: integer, callback: (function(cqe: userdata, res: integer): nil)?): userdata`

#### `luring.accept(ring: userdata, sockfd: integer, flags: integer, callback: (function(cqe: userdata, res: integer): nil)?): userdata`
The `res` is the file descriptor of the socket of the accepted connection if it does not fail.
