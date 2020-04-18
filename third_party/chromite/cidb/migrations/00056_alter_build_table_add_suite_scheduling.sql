ALTER TABLE buildTable
  ADD COLUMN suite_scheduling BOOLEAN NOT NULL DEFAULT false;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (56, '00056_alter_build_table_add_suite_scheduling.sql');
