USE cidb;

FLUSH PRIVILEGES;
CREATE USER annotator
IDENTIFIED BY PASSWORD '*54C06F8BEE226565D06E0AE8151E3C7381155E9B';

GRANT SELECT on cidb.* to annotator;
GRANT SELECT, UPDATE, INSERT on cidb.annotationsTable to annotator;

FLUSH PRIVILEGES;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (27, '00027_create_annotator_user.sql');
