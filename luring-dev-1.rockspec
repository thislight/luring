package = "luring"
version = "dev-1"
source = {
   url = "git+https://github.com/thislight/luring.git"
}
description = {
   homepage = "https://github.com/thislight/luring",
   license = "GPL v3 or later"
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
