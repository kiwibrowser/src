ALTER TABLE clActionTable
  MODIFY COLUMN build_id int(11) NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (63, '00063_alter_claction_table_allow_build_id_null.sql');
