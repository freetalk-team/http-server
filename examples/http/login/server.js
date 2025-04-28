const express = require('express');
const jwt = require('jsonwebtoken');  // JWT library
const cookieParser = require('cookie-parser');

const PORT = process.env.PORT || 3000;
const SECRET_KEY = process.env.SECRET_KEY || 'your_secret_key';  // Secret key for signing the JWT

const app = express();

app.use(express.json());
app.use(cookieParser());


app.post('/login', (req, res) => {

	// Create a payload with custom data
	const payload = {
		role: 'su',
		username: 'test_user',
		exp: Math.floor(Date.now() / 1000) + (60 * 60)  // Token expiration (1 hour)
	};

	// Sign the JWT
	const token = jwt.sign(payload, SECRET_KEY);

	// Send the token as a cookie to the client
	res.cookie('AuthToken', token, {
		httpOnly: true,  // Token should not be accessible via JS
		secure: false,   // Set to true if using HTTPS
		sameSite: 'Strict',
		maxAge: 600000   // 10 minutes expiration time
	});

	// res.json({ message: 'Login successful', token });
	res.redirect('/app');
});

app.post('/logout', (req, res) => {
	res.cookie('AuthToken', '', {
		httpOnly: true,
		secure: false,    // true if HTTPS
		sameSite: 'Strict',
		expires: new Date(0)  // Set expiration time in the past
	});

	//res.json({ message: 'Logged out' });
	res.redirect('/');
});


app.listen(PORT, () => {
	console.log('Login server running on:', PORT);
});
