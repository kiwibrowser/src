ALTER TABLE buildTable
  MODIFY waterfall ENUM('chromeos', 'chromiumos', 'chromiumos.tryserver',
                        'chromeos_release') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (14, '00014_alter_build_table_add_release_waterfall.sql');
