<?php
require_once 'config.php';

// ESP32 sends Refresh Token here safely
$refreshToken = isset($_POST['refresh_token']) ? $_POST['refresh_token'] : '';

if (empty($refreshToken)) {
    // Also support JSON body
    $input = json_decode(file_get_contents('php://input'), true);
    if (isset($input['refresh_token'])) {
        $refreshToken = $input['refresh_token'];
    }
}

if (empty($refreshToken)) {
    jsonResponse(['error' => 'No refresh_token provided'], 400);
}

// Exchange Refresh Token for New Access Token
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, 'https://accounts.spotify.com/api/token');
curl_setopt($ch, CURLOPT_POST, 1);
curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query([
    'grant_type' => 'refresh_token',
    'refresh_token' => $refreshToken,
]));
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'Authorization: Basic ' . base64_encode(SPOTIFY_CLIENT_ID . ':' . SPOTIFY_CLIENT_SECRET)
]);

$response = curl_exec($ch);
$httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

// Relay the Spotify response directly to ESP32
http_response_code($httpCode);
header('Content-Type: application/json');
echo $response;
?>