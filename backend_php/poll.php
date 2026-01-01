<?php
require_once 'config.php';

$deviceId = isset($_GET['device_id']) ? $_GET['device_id'] : '';

if (empty($deviceId)) {
    jsonResponse(['error' => 'No device_id'], 400);
}

// Security: Sanitize filename
$safeId = preg_replace('/[^a-zA-Z0-9_-]/', '', $deviceId);
$file = 'tokens/' . $safeId . '.json';

if (file_exists($file)) {
    // Token Found!
    $content = file_get_contents($file);
    $json = json_decode($content, true);

    // Send to ESP32
    jsonResponse($json);

    // Optional: Delete file after reading if you want one-time retrieval
    // unlink($file); 
} else {
    // Not ready yet
    jsonResponse(['status' => 'waiting'], 202);
}
?>