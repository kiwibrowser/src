USE mysql;

-- Delete all non-root users and all their privileges
DELETE from user where User!='root' and User!='';
DELETE from db where User!='root' and User!='';
DELETE from tables_priv where User!='root' and User!='';
DELETE from columns_priv where User!='root' and User!='';
DELETE from procs_priv where User!='root' and User!='';
DELETE from proxies_priv where User!='root' and User!='';
FLUSH PRIVILEGES;

USE cidb;

-- Create users
CREATE USER readonly
IDENTIFIED BY  PASSWORD '*12D90798C9984D0EFDDBF991FB2A92D7EEFC2E53';

CREATE USER bot
IDENTIFIED BY PASSWORD '*D17E032E3C8F0215AE62E2733FB66463F0746DAA';

-- Give users correct privileges
GRANT SELECT on cidb.* to readonly;

GRANT SELECT, UPDATE, INSERT on cidb.* to bot;

FLUSH PRIVILEGES;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (5, '00005_create_users.sql');
