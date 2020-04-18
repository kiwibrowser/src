ALTER TABLE clActionTable
  ADD COLUMN buildbucket_id varchar(80) DEFAULT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (48, '00048_alter_claction_table_add_buildbucket_id.sql');
