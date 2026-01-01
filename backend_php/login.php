<?php
require_once 'config.php';

// 1. Get Device ID from URL (ESP32 will generate this)
$deviceId = isset($_GET['device_id']) ? $_GET['device_id'] : '';

if (empty($deviceId)) {
    die("Error: No device_id provided. Scan the QR code again.");
}

// 2. Store Device ID in session or pass it via 'state'
// We use 'state' parameter to pass the device_id through Spotify Auth flow
$state = $deviceId;

// 3. Scopes (Permissions)
$scopes = [
    'user-read-playback-state',
    'user-read-currently-playing',
    'user-read-playback-position',
    'user-modify-playback-state'
];

// 4. Build URL
$authUrl = "https://accounts.spotify.com/authorize?" . http_build_query([
    'response_type' => 'code',
    'client_id' => SPOTIFY_CLIENT_ID,
    'scope' => implode(' ', $scopes),
    'redirect_uri' => REDIRECT_URI,
    'state' => $state,
    'show_dialog' => 'true'
]);

// 5. Redirect User
header("Location: $authUrl");
exit;
?>