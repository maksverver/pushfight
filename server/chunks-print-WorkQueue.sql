# Prints the content of the WorkQueue table in a human-readable format.
#
# Note that SHA256 sums are truncated!

.mode column
.headers on
.separator ROW "\n"
.nullvalue NULL

SELECT 
    phase, chunk, solver, user, machine,
    datetime(assigned, 'unixepoch') AS assigned,
    datetime(completed, 'unixepoch') AS completed,
    datetime(received, 'unixepoch') AS received,
    bytesize,
    lower(substr(hex(sha256sum), 0, 16)) AS sha256sum
FROM WorkQueue
