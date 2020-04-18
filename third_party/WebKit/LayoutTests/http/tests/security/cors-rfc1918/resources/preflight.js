function preflightURL(status, out) {
    return 'http://localhost:8080/security/cors-rfc1918/resources/preflight.php?preflight=' + status + '&out=' + out;
}
