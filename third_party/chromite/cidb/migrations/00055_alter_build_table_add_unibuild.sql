ALTER TABLE buildTable
  ADD COLUMN unibuild BOOLEAN NOT NULL DEFAULT false;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (55, '00055_alter_build_table_add_unibuild.sql');
