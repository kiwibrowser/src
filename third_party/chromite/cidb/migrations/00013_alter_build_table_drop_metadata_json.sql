ALTER TABLE buildTable
  DROP COLUMN metadata_json;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (13, '00013_alter_build_table_drop_metadata_json.sql');
