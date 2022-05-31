# Schema for the server.
#
# Create the database with:
#
#  sqlite3 chunks.db < chunks-schema.sql

CREATE TABLE WorkQueue(
    phase INTEGER NOT NULL,
    chunk INTEGER NOT NULL,

    solver TEXT,
    user TEXT,
    machine TEXT,
    
    assigned INTEGER,
    completed INTEGER,
    received INTEGER,

    bytesize INTEGER,
    sha256sum BLOB,

    PRIMARY KEY(phase, chunk)
);
