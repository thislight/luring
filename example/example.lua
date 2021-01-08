local luring = require "luring"

local ring = luring.queue_init(4, 0)

local fd = luring.open_file("./example.txt", "w+")
if fd < 0 then
    print(string.format("could not open example.txt: %s", luring.strerror(-fd)))
    return
end

luring.write(ring, fd, "Hello World!", 0)

luring.submit_and_wait(ring, 1)

luring.do_cqes(ring)

luring.read(ring, fd, 4096, 0, function(cqe, res, result)
    if res < 0 then
        print(luring.strerror(-res))
    else
        print(res, result)
    end
    luring.cqe_seen(ring, cqe)
end)

luring.submit_and_wait(ring, 1)

luring.do_cqes(ring)
