DROP TRIGGER IF EXISTS post_ai;
DROP TRIGGER IF EXISTS post_ad;
DROP TRIGGER IF EXISTS post_au;

DROP TABLE IF EXISTS post_fts;
DROP VIEW IF EXISTS posts;
DROP VIEW IF EXISTS post_user;
DROP TABLE IF EXISTS post;

CREATE TABLE post (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	title text NOT NULL,
	summary text DEFAULT '',
	user INTEGER NOT NULL,
	md blob,
	created DATETIME DEFAULT CURRENT_TIMESTAMP,
	updated DATETIME DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO post(title, summary, user) VALUES
	('HTTP server', 'HTTP EJS router introduction', 1),
	('EJS script', 'Simple EJS parser', 2),
	('Heroku deployment', 'Nginx, EJS Router deployment on Heroku', 3),
	('Build EJS Router', 'Build EJS Router on Ubuntu & Alpine', 4);


CREATE VIEW post_user AS SELECT
	post.id,
	post.title,
	post.summary,
	post.md,
	post.created,
	post.updated,
	post.user,
	user.name AS username,
	user.email
FROM post
JOIN user ON post.user = user.id;

CREATE VIEW posts AS SELECT
	post.id,
	post.title,
	post.summary,
	post.created,
	post.user,
	user.name AS username,
	user.email
FROM post
JOIN user ON post.user = user.id ORDER BY post.created DESC;

CREATE VIRTUAL TABLE posts_fts USING fts5(
	title,
	summary,
	content='posts',
	content_rowid='id'
);

-- populate users_fts with the current data
INSERT INTO posts_fts (rowid, title, summary)
SELECT id, title, summary FROM posts;

-- After INSERT
CREATE TRIGGER post_ai AFTER INSERT ON post BEGIN
	INSERT INTO posts_fts(rowid, title, summary)
	VALUES (new.id, new.title, new.summary);
END;

-- After DELETE
CREATE TRIGGER post_ad AFTER DELETE ON post BEGIN
	DELETE FROM posts_fts WHERE rowid = old.id;
END;

-- After UPDATE
CREATE TRIGGER post_au AFTER UPDATE ON post BEGIN
	UPDATE posts_fts
	SET title = new.title, email = new.summary
	WHERE rowid = old.id;
END;
