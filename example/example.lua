local luring = require "luring"

local ring = luring.queue_init(4, 0)

local fd = luring.open_file("./example.txt", "w+")

luring.write(ring, fd, "Hello World!", 0, function(cqe)
    luring.cqe_seen(ring, cqe)
end)

luring.submit_and_wait(ring, 1)

luring.do_cqes(ring)

luring.read(ring, fd, 4096, 0, function(cqe, result)
    print(result)
    luring.cqe_seen(ring, cqe)
end)

luring.submit_and_wait(ring, 1)

luring.do_cqes(ring)
