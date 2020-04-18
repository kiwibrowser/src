-- The deadline column records the latest expected finish_time for this build.
ALTER TABLE buildTable
  ADD COLUMN deadline TIMESTAMP DEFAULT 0;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (32, '00032_alter_build_table_add_deadline.sql');
