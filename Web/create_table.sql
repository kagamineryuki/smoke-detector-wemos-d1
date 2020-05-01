CREATE TABLE encrypted(
    id SERIAL PRIMARY KEY,
    encrypted TEXT,
    cycle BIGINT,
    time BIGINT,
    length BIGINT,
    machine_id BIGINT,
    encryption_type VARCHAR(50)
);

CREATE TABLE current(
    id SERIAL PRIMARY KEY,
    current NUMERIC,
    machine_id BIGINT,
    encryption_type VARCHAR(50)
);

CREATE TABLE decrypted(
    id SERIAL PRIMARY KEY,
    decrypted TEXT,
    cycle BIGINT,
    time BIGINT,
    length BIGINT,
    machine_id BIGINT,
    encryption_type VARCHAR(50)
);

SELECT * FROM encrypted;
SELECT * FROM current;
SELECT * FROM decrypted;