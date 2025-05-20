
public('public');
views('views');

database('sqlite://user.db');

set('Site', {
	domain: 'www.sipme.io',
	name: 'Demo Server'
});

get('/', { ejs: 'index.ejs', cache: 120 });

get('/users', { 
    db: { 
        alias: 'users', 
        table: 'user_info', 
        limit: 3,
        fts: { table: 'users_fts', index: 'name', rank: [10.0, 1.0] }
    },
    ejs: 'users.ejs', cache: 600 
});

get('/user', { db: { table: 'users', op: 'schema' }, ejs: 'add-user.ejs', cache: 600  });
get('/user/:id', { db: { table: 'users'  }, ejs: 'user.ejs' });

post('/user', { db: { table: 'users'}, redirect: '/user/:id', role: 'su' });
delete('/user/:id', { db: { table: 'users'  } });
get('/api/users', { 
    db: { 
        table: 'user_info', 
        limit: 10,
        fts: { table: 'users_fts', index: 'name', rank: [10.0, 1.0] }
    }, 
    cache: 60 });

post('/api/user', { db: { table: 'users'} });
