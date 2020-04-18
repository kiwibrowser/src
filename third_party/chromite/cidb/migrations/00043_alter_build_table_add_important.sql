ALTER TABLE buildTable
  ADD COLUMN important BOOLEAN DEFAULT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (43, '00043_alter_build_table_add_important.sql');
