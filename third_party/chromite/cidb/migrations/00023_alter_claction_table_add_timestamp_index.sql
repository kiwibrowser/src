ALTER TABLE clActionTable
  ADD INDEX timestamp_index(timestamp);


INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (23, '00023_alter_claction_table_add_timestamp_index.sql');
