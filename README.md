# Lightweight C++ HTTP Server

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Boost Libraries](https://img.shields.io/badge/Boost-Asio%20%26%20Beast-ff69b4.svg)](https://www.boost.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A **tiny, high-performance HTTP server** built with **Boost.Asio** and **Boost.Beast**.  
Easily configurable through a simple `config.js` file, supporting **EJS templating** and **built-in database connectivity**.

---

## ✨ Features

- ⚡ **Fast and lightweight** — Ideal for small web services or embedded applications.
- ⚙️ **Easy configuration** — Define routes, static files, views, and database connections with JavaScript-like syntax.
- 📝 **EJS templating engine** — Render dynamic HTML pages seamlessly.
- 🛢️ **Database integration** — Native support for connecting to databases like SQLite.
- 📦 **Static file serving** — Serve public assets effortlessly.
- 🧠 **Route caching** — Control caching behavior individually per route.
- 🛠️ **Minimal dependencies** — Requires only C++, Boost, and EJS templates.

---

## 📜 Example `config.js`

```javascript
// Serve static files from 'public' directory
public('public');

// Set views directory for EJS templates
views('views');

// Connect to a SQLite database
database('sqlite://test.db');

// Define routes
get('/', { ejs: 'index.ejs', cache: 120 });

get('/users', { db: { table: 'users', limit: 3 }, ejs: 'users.ejs', cache: 600 });

get('/user', { db: { table: 'users', op: 'schema' }, ejs: 'add-user.ejs', cache: 600 });

get('/user/:id', { db: { table: 'users' }, ejs: 'user.ejs' });
```


## BUILD



### Requirements

* g++ 14 (std=c++23)
* cmake
* sqlite3
* postgresql
* nlohmann-json
* boost 1.83

```
apt install g++14 \
    cmake make \
    libboost-all-dev \
    nlohmann-json3-dev \
    libsqlite3-dev \
    libpq-dev
```

### Ubuntu
```
make
```

or

```
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Alpine

Build **build** image

```
cd build/alpine
make build
```

## RUN

```
./server 0.0.0.0 8080 ../examples/www ../examples/config/basic.js 4
```

## Donation

We hope you've found our software useful. As a non-profit organization, we rely on the generosity of people like you to continue our mission of creating free/OS software

If you've found our work valuable and would like to support us, please consider making a donation. Your contribution, no matter the size, will make a meaningful difference in the lives of those we serve

Thank you for considering supporting us. Together, we can make a positive impact on our community/world

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=XUSKMVK55P35G)

