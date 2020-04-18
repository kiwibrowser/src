-- The summary field contains an overall human readable summary of the build.
-- The master builders summarize failures from all their slaves.
-- slaves summarize only their own failure.
ALTER TABLE buildTable
  ADD COLUMN summary varchar(1024) DEFAULT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (25, '00025_alter_build_table_add_summary.sql');
