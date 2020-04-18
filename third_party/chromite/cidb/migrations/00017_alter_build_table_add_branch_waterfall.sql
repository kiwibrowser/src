ALTER TABLE buildTable
  MODIFY waterfall ENUM('chromeos', 'chromiumos', 'chromiumos.tryserver',
                        'chromeos_release', 'chromeos.branch') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (17, '00017_alter_build_table_add_branch_waterfall.sql');
