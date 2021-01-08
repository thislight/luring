#include "luring.h"

#define Lcheckstack(L, n) luaL_checkstack(L, n, "in luring")

#define Geturing(var, L, n) struct io_uring *var = luaL_checkudata(L, n, LURING)

#define Getsqe(var, ring) struct io_uring_sqe *var = io_uring_get_sqe(ring)

#define Getrequest(var, callback_id, buffer_size, type) struct luring_request *var = luring_request_new(callback_id, buffer_size, type)

static unsigned int luaL_checkuint(lua_State *L, int arg) {
    lua_Integer value = luaL_checkinteger(L, arg);
    if ((value < 0) || (value > UINT_MAX)) {
        luaL_argerror(L, arg,
                      "the value must be not greater than UINT_MAX and be "
                      "greater than 0");
    }
    return value;
}

int luring_queue_init(lua_State *L) {
    unsigned int entries = luaL_checkuint(L, 1);
    unsigned int flags = luaL_checkuint(L, 2);
    lua_newuserdata(L, sizeof(struct io_uring));
    struct io_uring *ring = lua_touserdata(L, -1);
    io_uring_queue_init(entries, ring, flags);
    if (luaL_newmetatable(L, LURING)) {
        lua_pushcfunction(L, luring_queue_exit);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
    luaL_setmetatable(L, LURING);
    return 1;
}

int luring_submit(lua_State *L) {
    Geturing(ring, L, 1);
    int r = io_uring_submit(ring);
    lua_pushinteger(L, r);
    return 1;
}

int luring_submit_and_wait(lua_State *L) {
    Geturing(ring, L, 1);
    unsigned int wait_nr = (unsigned int)luaL_checkinteger(L, 2);
    int r = io_uring_submit_and_wait(ring, wait_nr);
    lua_pushinteger(L, r);
    return 1;
}

int luring_queue_exit(lua_State *L) {
    Geturing(ring, L, 1);
    io_uring_queue_exit(ring);
    return 0;
}

enum luring_request_type {
    REQ_TREAD,
    REQ_TWRTIE,
    REQ_TACCEPT,
};

struct luring_request {
    int callback_id;
    void *buffer; /* it will be store address in TACCEPT */
    size_t buffer_size;
    socklen_t sockaddrlen;
    enum luring_request_type type;
};

struct luring_request *luring_request_new(int callback_id, size_t buffer_size, enum luring_request_type type) {
    struct luring_request *request = malloc(sizeof(struct luring_request));
    request->callback_id = callback_id;
    if (buffer_size > 0) {
        request->buffer = malloc(buffer_size);
        request->buffer_size = buffer_size;
    } else {
        request->buffer = NULL;
        request->buffer_size = 0;
    }
    return request;
}

void luring_request_free(struct luring_request *req) {
    if (req->buffer_size > 0) {
        free(req->buffer);
    }
    free(req);
}

int luring_send(lua_State *L) {
    Geturing(ring, L, 1);
    int sockfd = luaL_checkinteger(L, 2);
    int buffer_len = luaL_len(L, 3);
    const char *buffer = luaL_checkstring(L, 3);
    int flags = luaL_checkinteger(L, 4);
    int callback_id = LUA_NOREF;
    if (!lua_isnoneornil(L, 5)){
        luaL_checkany(L, 5); /* it's a callback */
        lua_settop(L, 5);
        int callback_id = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    Getsqe(sqe, ring);
    io_uring_prep_send(sqe, sockfd, buffer, buffer_len, flags);
    Getrequest(req, callback_id, 0, REQ_TWRTIE);
    io_uring_sqe_set_data(sqe, req);
    Lcheckstack(L, 1);
    lua_pushlightuserdata(L, sqe);
    return 1;
}

int luring_recv(lua_State *L) {
    Geturing(ring, L, 1);
    int sockfd = luaL_checkinteger(L, 2);
    size_t buffer_size = luaL_checkinteger(L, 3);
    int flags = luaL_checkinteger(L, 4);
    int callback_id = LUA_NOREF;
    if (!lua_isnoneornil(L, 5)){
        luaL_checkany(L, 5); /* it's a callback */
        lua_settop(L, 5);
        callback_id = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    Getrequest(req, callback_id, buffer_size, REQ_TREAD);
    Getsqe(sqe, ring);
    io_uring_prep_recv(sqe, sockfd, req->buffer, req->buffer_size, flags);
    io_uring_sqe_set_data(sqe, req);
    Lcheckstack(L, 1);
    lua_pushlightuserdata(L, sqe);
    return 1;
}

int luring_write(lua_State *L){
    Geturing(ring, L, 1);
    int fd = luaL_checkinteger(L, 2);
    lua_Integer len = luaL_len(L, 3);
    if (len > UINT_MAX){
        luaL_error(L, "luring_write does not support writing string which the length is greater than %u", UINT_MAX);
    }
    const char *content = luaL_checkstring(L, 3);
    lua_Integer offest = luaL_checkinteger(L, 4);
    if (offest > LONG_MAX){
        luaL_error(L, "offest should <= %ld", LONG_MAX);
    }
    int callback_id = LUA_NOREF;
    if (!lua_isnoneornil(L, 5)){
        luaL_checkany(L ,5);
        lua_settop(L, 5);
        callback_id = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    Getrequest(req, callback_id, 0, REQ_TWRTIE);
    Getsqe(sqe, ring);
    io_uring_prep_write(sqe, fd, content, (unsigned int)len, (off_t)offest);
    io_uring_sqe_set_data(sqe, req);
    Lcheckstack(L, 1);
    lua_pushlightuserdata(L, sqe);
    return 1;
}

int luring_accept(lua_State *L){
    Geturing(ring, L, 1);
    int sockfd = luaL_checkinteger(L, 2);
    int flags = luaL_checkinteger(L, 3);
    int callback_id = LUA_NOREF;
    if (!lua_isnoneornil(L, 4)){
        luaL_checkany(L, 4);
        lua_settop(L, 4);
        callback_id = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    Getrequest(req, callback_id, sizeof(struct sockaddr), REQ_TACCEPT);
    Getsqe(sqe, ring);
    io_uring_prep_accept(sqe, sockfd, req->buffer, &(req->sockaddrlen), flags);
    io_uring_sqe_set_data(sqe, req);
    Lcheckstack(L, 1);
    lua_pushlightuserdata(L, sqe);
    return 1;
}

int luring_read(lua_State *L){
    Geturing(ring, L, 1);
    int fd = luaL_checkinteger(L, 2);
    lua_Integer buffer_size = luaL_checkinteger(L, 3);
    if (buffer_size < 0 || buffer_size > UINT_MAX){
        luaL_argerror(L, 3, "buffer size must be between 0 and UINT_MAX");
    }
    lua_Integer offest = luaL_checkinteger(L, 4);
    if (offest > LONG_MAX){
        luaL_argerror(L, 4, "offest must be not greater than LONG_MAX");
    }
    int callback_id = LUA_NOREF;
    if (!lua_isnoneornil(L, 5)){
        luaL_checkany(L, 5);
        lua_settop(L, 5);
        callback_id = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    Getrequest(req, callback_id, buffer_size, REQ_TREAD);
    Getsqe(sqe, ring);
    io_uring_prep_read(sqe, fd, req->buffer, (unsigned int)(req->buffer_size), (off_t)offest);
    io_uring_sqe_set_data(sqe, req);
    Lcheckstack(L, 1);
    lua_pushlightuserdata(L, sqe);
    return 1;
}

int luring_cqe_seen(lua_State *L) {
    Geturing(ring, L, 1);
    struct io_uring_cqe *cqe = lua_touserdata(L, 2);
    struct luring_request *req = (struct luring_request*)cqe->user_data;
    if (req->callback_id != LUA_NOREF){
        luaL_unref(L, LUA_REGISTRYINDEX, req->callback_id);
    }
    luring_request_free((struct luring_request*)cqe->user_data);
    io_uring_cqe_seen(ring, cqe);
    return 0;
}

int luring_do_cqes(lua_State *L) {
    lua_Integer count = 0;
    Geturing(ring, L, 1);
    int ret = 0;
    while (ret == 0) {
        struct io_uring_cqe *cqe;
        ret = io_uring_peek_cqe(ring, &cqe);
        if (ret == 0) {
            Lcheckstack(L, 2);
            struct luring_request *req = (struct luring_request*)cqe->user_data;
            if (req->callback_id != LUA_NOREF) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, req->callback_id);
                switch(req->type){
                    case REQ_TREAD:
                    Lcheckstack(L, 2);
                    lua_pushlightuserdata(L, cqe);
                    lua_pushlstring(L, req->buffer, req->buffer_size);
                    lua_call(L, 2, 0);
                    break;
                    case REQ_TWRTIE:
                    Lcheckstack(L, 1);
                    lua_pushlightuserdata(L, cqe);
                    lua_call(L, 1, 0);
                    break;
                    case REQ_TACCEPT: /* callback: cqe, sockfd (to client)*/
                    Lcheckstack(L, 2);
                    lua_pushlightuserdata(L, cqe);
                    lua_pushinteger(L, cqe->res);
                    /* TODO: callback with addr */
                    lua_call(L, 2, 0);
                    break;
                }
            } else {
                Lcheckstack(L, 3);
                lua_pushcfunction(L, &luring_cqe_seen);
                lua_pushnil(L);
                lua_copy(L, 1, -1);
                lua_pushlightuserdata(L, cqe);
                lua_call(L, 2, 0);
            }
        }
    }
    lua_pushinteger(L, ret);
    return 1;
}

static bool strcompare(const char* s1, const char* s2){
    return !strcmp(s1, s2);
}

int luring_open_file(lua_State *L){
    const char *name = luaL_checkstring(L, 1);
    const char *mode = lua_type(L, 2) != LUA_TNONE ? luaL_checkstring(L, 2): "w";
    int flags = 0;
    const int CREAT_S = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    if (strcompare(mode, "w")){
        flags = (O_RDWR | O_CREAT | O_APPEND);
    } else if (strcompare(mode, "w+")){
        flags = (O_RDWR | O_CREAT | O_TRUNC | O_APPEND);
    } else if (strcompare(mode, "a")){
        flags = (O_WRONLY | O_CREAT | O_APPEND);
    } else if (strcompare(mode, "a+")){
        flags = (O_WRONLY | O_CREAT | O_TRUNC | O_APPEND);
    } else if (strcompare(mode, "r")){
        flags = O_RDONLY;
    } else {
        luaL_argerror(L, 2, "mode must be one of: w, w+, a, a+, r");
    }
    int fd = open(name, flags, CREAT_S);
    if (fd > 0){
        Lcheckstack(L, 1);
        lua_pushinteger(L, fd);
        return 1;
    } else {
        luaL_error(L, "open %s: %s", name, strerror(errno));
    }
}
