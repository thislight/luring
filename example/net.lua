local sys_socket = require "posix.sys.socket" -- You need 'luaposix' for this example
local luring = require "luring"

local ring = luring.queue_init(64, 0)

local serverfd =
    sys_socket.socket(sys_socket.AF_INET, sys_socket.SOCK_STREAM, 0)
local clientfd =
    sys_socket.socket(sys_socket.AF_INET, sys_socket.SOCK_STREAM, 0)

local server_address = {
    family = sys_socket.AF_INET,
    port = 6789,
    addr = "127.0.0.1"
}

sys_socket.bind(serverfd, server_address)
if sys_socket.listen(serverfd, 64) ~= 0 then error("listen fails") end

local server_addr = sys_socket.getsockname(serverfd)
print(string.format("served on %s:%d", server_addr.addr, server_addr.port))

local ok, err, e = sys_socket.connect(clientfd, server_address)
if err then error(err) end

local program_stop = false

luring.accept(ring, serverfd, 0, function(cqe, toclientfd)
    if toclientfd < 0 then
        print(string.format("server accept error: %s", luring.strerror(-toclientfd)))
    else
        luring.recv(ring, toclientfd, 4096, 0, function(cqe, res, result)
            if res < 0 then
                print(string.format("server recv error: %s",
                                    luring.strerror(-res)))
                program_stop = true
            else
                print(string.format("server recv: %s", result))
                luring.send(ring, toclientfd,
                            string.format("Hello %s!", result), 0)
                print("server sent")
            end
            luring.cqe_seen(ring, cqe)
        end)
        print("server attempt to recv")
    end
    luring.cqe_seen(ring, cqe)
end)
print("server attept to accept")

luring.send(ring, clientfd, "luring", 0, function(cqe, res)
    if res < 0 then
        print(string.format("client send error: %s", luring.strerror(-res)))
        program_stop = true
    else
        luring.recv(ring, clientfd, 4096, 0, function(cqe, res, result)
            if res < 0 then
                print(string.format("client recv error: %s",
                                    luring.strerror(-res)))
            else
                print(string.format("client recv: %s", result))
            end
            program_stop = true
            luring.cqe_seen(ring, cqe)
        end)
        print("client attempt to recv")
    end
    luring.cqe_seen(ring, cqe)
end)
print("client attempt to send")

while not program_stop do
    luring.submit_and_wait(ring, 1)
    luring.do_cqes(ring)
end
