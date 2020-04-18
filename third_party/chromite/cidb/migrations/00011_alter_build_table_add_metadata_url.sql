ALTER TABLE buildTable
  ADD COLUMN metadata_url varchar(240);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (11, '00011_alter_build_table_add_metadata_url.sql');
