ALTER TABLE buildTable
  MODIFY COLUMN waterfall VARCHAR(80);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (41, '00041_alter_build_table_stringify_waterfall.sql');
