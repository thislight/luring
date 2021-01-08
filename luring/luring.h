#include <stdio.h>
#include <liburing.h>
#include <lua.h>
#include <lauxlib.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#define LURING "luring"

int luring_queue_init(lua_State *L);

int luring_submit(lua_State *L);

int luring_submit_and_wait(lua_State *L);

int luring_queue_exit(lua_State *L);

int luring_send(lua_State *L);

int luring_recv(lua_State *L);

int luring_write(lua_State *L);

int luring_read(lua_State *L);

int luring_cqe_seen(lua_State *L);

int luring_do_cqes(lua_State *L);

int luring_open_file(lua_State *L);

LUALIB_API int luaopen_luring(lua_State *L){
    luaL_Reg reg[] = {
        {"queue_init", &luring_queue_init},
        {"queue_exit", &luring_queue_exit},
        {"submit", &luring_submit},
        {"submit_and_wait", &luring_submit_and_wait},
        {"send", &luring_send},
        {"recv", &luring_recv},
        {"write", &luring_write},
        {"read", &luring_read},
        {"do_cqes", &luring_do_cqes},
        {"cqe_seen", &luring_cqe_seen},
        {"open_file", &luring_open_file},
        {NULL, NULL},
    };
    luaL_newlib(L, reg);
    return 1;
}
