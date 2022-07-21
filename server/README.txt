This directory contains source files for the Push Fight work server,
which is accessed by the network-enabled solvers like solve2.

server.py serves on two ports:

 - port 7429: work assignment using a custom RPC protocol.
 - port 7430: plain text status page served over HTTP.
