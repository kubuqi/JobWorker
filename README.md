# Job Worker
Running jobs on a remote linux server.

# Build:
Assuming that gRPC is been installed in "~/local" path, and "~/local/bin" has been added into $PATH, then
```bash
cd cmake/build
cmake -DCMAKE_PREFIX_PATH="~/local" ../..
make -j 4
cd ../..
```

# Run:
First you need to generate the certificates and keys for the client and server. To do this:
```
cd cert
./gen.sh
cd ..
```

Then change directory to where the generated binaries are, which is under cmake/build.
```
cd cmake/build
```

Start the server: 
```./server```

Start the client with any command: 
```./client ping google.com```

Stop the client: press return

