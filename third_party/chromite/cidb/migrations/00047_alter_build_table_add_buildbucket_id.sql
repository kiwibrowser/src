ALTER TABLE buildTable
  ADD COLUMN buildbucket_id varchar(80) DEFAULT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (47, '00047_alter_build_table_add_buildbucket_id.sql');
