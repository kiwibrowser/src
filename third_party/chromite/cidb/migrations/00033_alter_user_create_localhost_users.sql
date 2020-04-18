-- Create @localhost versions of existing users.
CREATE USER readonly@localhost
IDENTIFIED BY  PASSWORD '*12D90798C9984D0EFDDBF991FB2A92D7EEFC2E53';
CREATE USER bot@localhost
IDENTIFIED BY PASSWORD '*D17E032E3C8F0215AE62E2733FB66463F0746DAA';
CREATE USER annotator@localhost
IDENTIFIED BY PASSWORD '*54C06F8BEE226565D06E0AE8151E3C7381155E9B';

-- And grant them the same permissions as their existing counterparts.
GRANT SELECT on cidb.* to readonly@localhost;
GRANT SELECT, UPDATE, INSERT on cidb.* to bot@localhost;
GRANT SELECT on cidb.* to annotator@localhost;
GRANT SELECT, UPDATE, INSERT on cidb.annotationsTable to annotator@localhost;

FLUSH PRIVILEGES;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (33, '00033_alter_user_create_localhost_users.sql');
