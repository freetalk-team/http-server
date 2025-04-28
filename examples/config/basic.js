
public('public');
views('views');

database('sqlite://user.db');

set('Site', {
	domain: 'www.sipme.io',
	name: 'Demo Server'
});

get('/', { ejs: 'index.ejs', cache: 120 });

get('/users', { db: { table: 'users', limit: 3 }, ejs: 'users.ejs', cache: 600 });
get('/user/:id', { db: { table: 'users'  }, ejs: 'user.ejs' });
