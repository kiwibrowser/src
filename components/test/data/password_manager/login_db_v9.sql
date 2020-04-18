PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE meta(key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY, value LONGVARCHAR);
INSERT INTO "meta" VALUES('last_compatible_version','1');
INSERT INTO "meta" VALUES('version','9');
CREATE TABLE logins (
origin_url VARCHAR NOT NULL,
action_url VARCHAR,
username_element VARCHAR,
username_value VARCHAR,
password_element VARCHAR,
password_value BLOB,
submit_element VARCHAR,
signon_realm VARCHAR NOT NULL,
ssl_valid INTEGER NOT NULL,
preferred INTEGER NOT NULL,
date_created INTEGER NOT NULL,
blacklisted_by_user INTEGER NOT NULL,
scheme INTEGER NOT NULL,
password_type INTEGER,
possible_usernames BLOB,
times_used INTEGER,
form_data BLOB,
use_additional_auth INTEGER,
date_synced INTEGER,
display_name VARCHAR,
avatar_url VARCHAR,
federation_url VARCHAR,
is_zero_click INTEGER,
UNIQUE (origin_url, username_element, username_value, password_element, submit_element, signon_realm));
INSERT INTO "logins" VALUES(
'https://accounts.google.com/ServiceLogin', /* origin_url */
'https://accounts.google.com/ServiceLoginAuth', /* action_url */
'Email', /* username_element */
'theerikchen', /* username_value */
'Passwd', /* password_element */
X'', /* password_value */
'', /* submit_element */
'https://accounts.google.com/', /* signon_realm */
1, /* ssl_valid */
1, /* preferred */
13047429345000000, /* date_created */
0, /* blacklisted_by_user */
0, /* scheme */
0, /* password_type */
X'00000000', /* possible_usernames */
1, /* times_used */
X'18000000020000000000000000000000000000000000000000000000', /* form_data */
NULL, /* use_additional_auth */
0, /* date_synced */
'', /* display_name */
'', /* avatar_url */
'', /* federation_url */
0  /* is_zero_click */
);
INSERT INTO "logins" VALUES(
'https://accounts.google.com/ServiceLogin', /* origin_url */
'https://accounts.google.com/ServiceLoginAuth', /* action_url */
'Email', /* username_element */
'theerikchen2', /* username_value */
'Passwd', /* password_element */
X'', /* password_value */
'', /* submit_element */
'https://accounts.google.com/', /* signon_realm */
1, /* ssl_valid */
1, /* preferred */
13047423600000000, /* date_created */
0, /* blacklisted_by_user */
0, /* scheme */
0, /* password_type */
X'00000000', /* possible_usernames */
1, /* times_used */
X'18000000020000000000000000000000000000000000000000000000', /* form_data */
NULL, /* use_additional_auth */
0, /* date_synced */
'', /* display_name */
'https://www.google.com/icon', /* avatar_url */
'', /* federation_url */
0  /* is_zero_click */
);
INSERT INTO "logins" VALUES(
'http://example.com', /* origin_url */
'http://example.com/landing', /* action_url */
'', /* username_element */
'user', /* username_value */
'', /* password_element */
X'', /* password_value */
'', /* submit_element */
'http://example.com', /* signon_realm */
1, /* ssl_valid */
1, /* preferred */
13047423600000000, /* date_created */
0, /* blacklisted_by_user */
1, /* scheme */
0, /* password_type */
X'00000000', /* possible_usernames */
1, /* times_used */
X'18000000020000000000000000000000000000000000000000000000', /* form_data */
NULL, /* use_additional_auth */
0, /* date_synced */
'', /* display_name */
'https://www.google.com/icon', /* avatar_url */
'', /* federation_url */
0  /* is_zero_click */
);
CREATE INDEX logins_signon ON logins (signon_realm);
COMMIT;

