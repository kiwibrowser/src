ALTER TABLE clActionTable
  MODIFY action ENUM('picked_up', 'submitted', 'kicked_out', 'submit_failed',
                     'verified') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (10, '00010_alter_claction_table_add_verified.sql');
