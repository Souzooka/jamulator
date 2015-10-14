export CGO_CPPFLAGS="`llvm-config-3.6 --cppflags`"
export CGO_LDFLAGS="`llvm-config-3.6  --ldflags --libs --system-libs all` runtime/runtime.a -lGLEW -lGL -lSDL -lSDL_gfx"
export CGO_CXXFLAGS=-std=c++11
make runtime/runtime.a
cp -r runtime/ ../../../llvm.org/svn/llvm-project/llvm/branches/release_36/bindings/go/llvm.svn/
go build -tags byollvm test.go
