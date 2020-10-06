# Job Worker
Running jobs on a remote linux server.

# Build:
Assuming that gRPC is been installed in "~/local" path, and "~/local/bin" has been added into $PATH, then

mkdir -p cmake/build
cd cmake/build
cmake -DCMAKE_PREFIX_PATH="~/local" ../..
make -j 4

# Run:
Change directory int cmake/build.

Start the server: ./server

Start the client with any command: ./client ping google.com
Stop the client: press return

