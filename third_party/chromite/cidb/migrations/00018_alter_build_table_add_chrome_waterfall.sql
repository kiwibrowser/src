ALTER TABLE buildTable
  MODIFY waterfall ENUM('chromeos', 'chromiumos', 'chromiumos.tryserver',
                        'chromeos_release', 'chromeos.branch',
                        'chromeos.chrome') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (18, '00018_alter_build_table_add_chrome_waterfall.sql');
