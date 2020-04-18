ALTER TABLE buildTable
  ADD INDEX start_time(start_time);


INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (50, '00050_alter_build_table_add_start_time_index.sql');
