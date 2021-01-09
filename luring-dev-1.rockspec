package = "luring"
version = "dev-1"
source = {
   url = "git+https://github.com/thislight/luring.git",
}
description = {
   homepage = "https://github.com/thislight/luring",
   license = "Apache-2.0",
   summary = "Luring is a callback-style interface to io_uring.",
   detailed = [[Luring is a callback-style interface to io_uring. Io_uring is a new I/O framework introduced in Linux 5.1. You should have liburing to install it.]],
}
build = {
   type = "builtin",
   modules = {
      luring = {
         sources = {"luring/luring.c"},
         incdirs = {"luring/"},
         libraries = {"uring"},
      }
   }
}
