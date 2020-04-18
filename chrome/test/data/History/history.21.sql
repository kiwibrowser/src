BEGIN TRANSACTION;
-- Version reset
CREATE TABLE meta(key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY,value LONGVARCHAR);
INSERT INTO "meta" VALUES('version','21');
INSERT INTO "meta" VALUES('last_compatible_version','16');

-- Create old androi_urls table
CREATE TABLE android_urls(id INTEGER PRIMARY KEY, raw_url LONGVARCHAR, created_time INTEGER NOT NULL, last_visit_time INTEGER NOT NULL, url_id INTEGER NOT NULL,favicon_id INTEGER DEFAULT NULL, bookmark INTEGER DEFAULT 0);
CREATE INDEX android_urls_raw_url_idx ON android_urls(raw_url);
CREATE INDEX android_urls_url_id_idx ON android_urls(url_id);
INSERT INTO "android_urls" VALUES(1,'http://google.com/',1,1,1,1,0);
INSERT INTO "android_urls" VALUES(4,'www.google.com/',1,1,3,1,1);

-- Create downloads table in proper (old) format.
CREATE TABLE downloads (
    id INTEGER PRIMARY KEY,
    full_path LONGVARCHAR NOT NULL,
    url LONGVARCHAR NOT NULL,
    start_time INTEGER NOT NULL,
    received_bytes INTEGER NOT NULL,
    total_bytes INTEGER NOT NULL,
    state INTEGER NOT NULL,
    end_time INTEGER NOT NULL,
    opened INTEGER NOT NULL);

COMMIT;


