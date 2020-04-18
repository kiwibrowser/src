ALTER TABLE buildTable
  MODIFY waterfall ENUM('chromeos', 'chromiumos', 'chromiumos.tryserver',
                        'chromeos_release', 'chromeos.branch',
                        'chromeos.chrome', 'chromiumos.chromium') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (38, '00038_alter_build_table_add_chromium_waterfall.sql');
