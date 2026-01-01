<?php
require_once 'config.php';

// 1. Handle Error
if (isset($_GET['error'])) {
    die("Spotify Auth Error: " . $_GET['error']);
}

// 2. Get Code and State (Device ID)
$code = $_GET['code'];
$deviceId = $_GET['state'];

if (empty($code) || empty($deviceId)) {
    die("Error: Invalid response from Spotify.");
}

// 3. Exchange Code for Tokens
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, 'https://accounts.spotify.com/api/token');
curl_setopt($ch, CURLOPT_POST, 1);
curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query([
    'grant_type' => 'authorization_code',
    'code' => $code,
    'redirect_uri' => REDIRECT_URI,
]));
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'Authorization: Basic ' . base64_encode(SPOTIFY_CLIENT_ID . ':' . SPOTIFY_CLIENT_SECRET)
]);

$response = curl_exec($ch);
curl_close($ch);

$json = json_decode($response, true);

if (isset($json['access_token'])) {
    // 4. Save Tokens to File (tokens/DEVICE_ID.json)
    // Make sure 'tokens' folder exists and is writable (chmod 777)
    $file = 'tokens/' . preg_replace('/[^a-zA-Z0-9_-]/', '', $deviceId) . '.json';

    // Check if tokens directory exists
    if (!is_dir('tokens')) {
        mkdir('tokens', 0777, true);
    }

    file_put_contents($file, json_encode($json));

    echo "<h1>Success! 🎉</h1>";
    echo "<p>Device ($deviceId) authorized. Check your ESP32 screen.</p>";
} else {
    echo "<h1>Error 😞</h1>";
    print_r($json);
}
?>