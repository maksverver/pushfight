CREATE TABLE WorkQueue(
    phase INTEGER NOT NULL,
    chunk INTEGER NOT NULL,

    solver TEXT,
    user TEXT,
    machine TEXT,
    
    assigned INTEGER,
    completed INTEGER,

    PRIMARY KEY(phase, chunk)
);

CREATE TABLE WorkResults(
    phase INTEGER NOT NULL,
    chunk INTEGER NOT NULL,

    received INTEGER,

    solver TEXT,
    user TEXT,
    machine TEXT,

    bytesize INTEGER,
    sha256sum TEXT,

    PRIMARY KEY(phase, chunk)
);
