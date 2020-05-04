let s:clangvers = systemlist("clang --version | head -1 | awk '{print $NF}'")[0]
let &path=".,include,/usr/include,/usr/lib/clang/".s:clangvers."/include,,"
set makeprg=./build.sh
