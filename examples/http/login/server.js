const express = require('express');
const jwt = require('jsonwebtoken');  // JWT library
const cookieParser = require('cookie-parser');

const PORT = process.env.PORT || 3000;
const SECRET_KEY = process.env.SECRET_KEY || 'your_secret_key';  // Secret key for signing the JWT
const API_SERVER = process.env.API_SERVER || 'http://127.0.0.1:5000';
const API_KEY = process.env.API_KEY || 'mykey';

const app = express();

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(cookieParser());



async function authenticateUser(email, password) {
	try {
		// const user = await User.findOne({ where: { email } });

		//const url = API_SERVER + `?email=${email}`;
		const url = API_SERVER + '/' + email;

		const res = await fetch(url, {
			method: 'GET',
			headers: {
				Authorization: API_KEY
			}
		});

		const user = await res.json();

		if (!user) {
			console.error('User not found:', email);
			return { success: false, message: 'User not found' };
		}

		console.log(user);

		// const isPasswordValid = await bcrypt.compare(password, user.password);
		// if (!isPasswordValid) {
		// 	return { success: false, message: 'Invalid password' };
		// }

		return { success: true, user };
	} catch (error) {
		console.error('Authentication error:', error);
		return { success: false, message: 'An error occurred' };
	}
}


app.post('/login', async (req, res) => {

	const { username, password } = req.body;

	const r = await authenticateUser(username, password);

	if (!r.success) {
		res.redirect('/login-failed');
		return;
	}

	let role = 'user';
	let uid = r.user.id;

	if (typeof uid == 'number') {
		if (uid< 5)
			role = 'su';

		uid = uid.toString();
	}

	// Create a payload with custom data
	const payload = {
		r: role,
		u: uid,
		e: r.user.email,
		n: r.user.name,
		x: Math.floor(Date.now() / 1000) + (10 * 60)  // Token expiration (1 hour)
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


app.listen(PORT, async () => {
	console.log('Login server running on:', PORT);
});
