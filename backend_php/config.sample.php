<?php
// Spotify API Credentials
// Get these from https://developer.spotify.com/dashboard/
define('SPOTIFY_CLIENT_ID', 'YOUR_CLIENT_ID_HERE');
define('SPOTIFY_CLIENT_SECRET', 'YOUR_CLIENT_SECRET_HERE');
define('REDIRECT_URI', 'http://YOUR_SERVER_IP_OR_DOMAIN/callback.php');

// Helper Functions
function getAccessToken()
{
    $token_file = __DIR__ . '/tokens/access_token.txt';
    if (file_exists($token_file)) {
        return file_get_contents($token_file);
    }
    return null;
}

function getRefreshToken()
{
    $token_file = __DIR__ . '/tokens/refresh_token.txt';
    if (file_exists($token_file)) {
        return file_get_contents($token_file);
    }
    return null;
}

function saveTokens($access_token, $refresh_token, $expires_in)
{
    if (!is_dir(__DIR__ . '/tokens')) {
        mkdir(__DIR__ . '/tokens', 0777, true);
    }
    file_put_contents(__DIR__ . '/tokens/access_token.txt', $access_token);
    // Only update refresh token if a new one is provided (Rotation)
    if ($refresh_token) {
        file_put_contents(__DIR__ . '/tokens/refresh_token.txt', $refresh_token);
    }
}
?>