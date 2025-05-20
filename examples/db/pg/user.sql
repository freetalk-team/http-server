-- Drop in correct order
DROP TRIGGER IF EXISTS tsvectorupdate ON user_;
DROP FUNCTION IF EXISTS user_fts_trigger;
DROP INDEX IF EXISTS user_fts_idx;
DROP VIEW IF EXISTS users;
DROP TABLE IF EXISTS user_;
DROP TYPE IF EXISTS user_state;

-- Recreate type
CREATE TYPE user_state AS ENUM ('initial', 'complete', 'gmail');

-- Recreate table
CREATE TABLE user_ (
	id SERIAL PRIMARY KEY,
	name text NOT NULL,
	email text,
	state user_state DEFAULT 'initial',
	info JSONB DEFAULT '{}',
	created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);


-- Sample data
INSERT INTO user_(name, email) VALUES
	('Pavel P.', 'pavel@example.com'),
	('John Smith', 'john@example.com'),
	('Alice Freeman', 'alice@example.com'),
	('Pavel Patarinski', 'pavelp.work@gmail.com'),
	('Srebrina Zlateva', 'srebrina.zlateva@gmail.com'),
	('Alice F.', 'alice@gmail.com'),
	('Bob Smith', 'bob@example.com'),
	('Alice Carter', 'alice.carter@example.com'),
	('Benjamin Lopez', 'ben.lopez@example.com'),
	('Chloe Rivera', 'chloe.r@example.com'),
	('Daniel Hughes', 'dan.hughes@example.com'),
	('Ella Morgan', 'ella.m@example.com'),
	('Finn Cooper', 'f.cooper@example.com'),
	('Grace Kelly', 'gracek@example.com'),
	('Henry Adams', 'h.adams@example.com'),
	('Isla Brooks', 'isla.b@example.com'),
	('Jack Ward', 'jack.ward@example.com'),
	('Katie Simmons', 'katie.s@example.com'),
	('Liam Reed', 'liam.r@example.com'),
	('Mia Russell', 'mia.russell@example.com'),
	('Noah Dixon', 'noah.d@example.com'),
	('Olivia Hart', 'olivia.hart@example.com'),
	('Peter Walsh', 'p.walsh@example.com'),
	('Quinn Barrett', 'quinn.b@example.com'),
	('Ruby Fleming', 'ruby.f@example.com'),
	('Samuel Bishop', 'sam.b@example.com'),
	('Tessa Holt', 't.holt@example.com'),
	('Umar Lowe', 'umar.l@example.com'),
	('Violet Fox', 'violet.fox@example.com'),
	('Wyatt Neal', 'wyatt.neal@example.com'),
	('Xavier Lamb', 'x.lamb@example.com'),
	('Yasmin Blake', 'yas.blake@example.com'),
	('Zane Sharp', 'z.sharp@example.com'),
	('Harper Mills', 'harper.mills@example.com'),
	('Leo Doyle', 'leo.doyle@example.com'),
	('Nina Pratt', 'n.pratt@example.com'),
	('Oscar Kane', 'oscar.k@example.com');

-- Add FTS column
ALTER TABLE user_ ADD COLUMN fts tsvector;

-- Optional view
CREATE VIEW users AS
SELECT id,name,email,info,created,updated,fts FROM user_ ORDER BY created DESC;

-- Populate FTS column
-- UPDATE users SET fts = to_tsvector('english', coalesce(name, '') || ' ' || coalesce(email, ''));

UPDATE user_
SET fts = 
	setweight(to_tsvector('english', coalesce(name, '')), 'A') ||
	setweight(to_tsvector('english', coalesce(email, '')), 'B');


-- Index on FTS
CREATE INDEX user_fts_idx ON user_ USING GIN(fts);

-- Create trigger function
CREATE FUNCTION user_fts_trigger() RETURNS trigger AS $$
BEGIN
  NEW.fts := to_tsvector('english', coalesce(NEW.name, '') || ' ' || coalesce(NEW.email, ''));
  RETURN NEW;
END
$$ LANGUAGE plpgsql;

-- Create trigger
CREATE TRIGGER tsvectorupdate
BEFORE INSERT OR UPDATE ON user_
FOR EACH ROW
EXECUTE FUNCTION user_fts_trigger();


/*

SELECT id, name, email
FROM users
WHERE fts @@ to_tsquery('english', 'pavel')
ORDER BY ts_rank(fts, to_tsquery('english', 'pavel')) DESC;

*/