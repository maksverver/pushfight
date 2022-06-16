# Prints the content of the WorkQueue table in a human-readable format.
#
# Note that SHA256 sums are truncated!
#
# Run with:
#
#  sqlite3 chunks.db < chunks-print-WorkQueue.sql

.mode column
.headers on
.separator ROW "\n"
.nullvalue NULL

SELECT 
    phase, chunk, difficulty,
    solver, user, machine,
    datetime(assigned, 'unixepoch') AS assigned,
    datetime(completed, 'unixepoch') AS completed,
    datetime(received, 'unixepoch') AS received,
    bytesize,
    lower(substr(hex(sha256sum), 1, 16)) AS sha256sum
FROM WorkQueue
ORDER BY phase DESC, chunk ASC;
