# Prints a summary of the WorkQueue table in a human-readable format.
#
#  sqlite3 chunks.db < chunks-print-WorkQueue-summary.sql

.mode column
.headers on
.separator ROW "\n"
.nullvalue NULL

SELECT
    phase,
    SUM(difficulty) AS total,
    SUM(difficulty * (completed IS NOT NULL)) AS completed,
    SUM(difficulty * (completed IS NULL)) AS remaining,
    SUM(difficulty * (completed IS NULL AND assigned IS NOT NULL)) AS assigned
FROM WorkQueue
GROUP BY phase
ORDER BY phase;
