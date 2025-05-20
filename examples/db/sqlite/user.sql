DROP TRIGGER IF EXISTS user_ai;
DROP TRIGGER IF EXISTS user_ad;
DROP TRIGGER IF EXISTS user_au;

DROP TABLE IF EXISTS users_fts;
DROP VIEW IF EXISTS users;
DROP TABLE IF EXISTS user;

CREATE TABLE user (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	name text NOT NULL,
	email text UNIQUE,
	state text CHECK(state IN ('initial', 'complete', 'gmail')) DEFAULT 'initial',
	info text DEFAULT '{}',
	created DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Sample data
INSERT INTO user(name, email) VALUES
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


CREATE VIEW users AS SELECT id,name,email,created FROM user ORDER BY created DESC;


CREATE VIRTUAL TABLE users_fts USING fts5(
	name,
	email,
	content='users',
	content_rowid='id'
);

-- populate users_fts with the current data
INSERT INTO users_fts (rowid, name, email)
SELECT id, name, email FROM users;

-- After INSERT
CREATE TRIGGER user_ai AFTER INSERT ON user BEGIN
	INSERT INTO users_fts(rowid, name, email)
	VALUES (new.id, new.name, new.email);
END;

-- After DELETE
CREATE TRIGGER user_ad AFTER DELETE ON user BEGIN
	DELETE FROM users_fts WHERE rowid = old.id;
END;

-- After UPDATE
CREATE TRIGGER user_au AFTER UPDATE ON user BEGIN
	UPDATE users_fts
	SET name = new.name, email = new.email
	WHERE rowid = old.id;
END;


/*

SELECT v.*, bm25(users_fts, 10.0, 1.0) AS rank
FROM user_info v
JOIN users_fts fts ON fts.rowid = v.id
WHERE users_fts MATCH 'Pavel'
ORDER BY rank;

*/

/*

CREATE VIEW user_search AS
SELECT
	user_info.*,
	user_info.id AS fts_rowid  -- expose this to join later
FROM user_info
JOIN users_fts ON user_info.id = users_fts.rowid;


SELECT v.*, bm25(users_fts, 10.0, 1.0) AS rank
FROM user_search v
JOIN users_fts fts ON fts.rowid = v.fts_rowid
WHERE users_fts MATCH 'Pavel'
ORDER BY rank;

*/


