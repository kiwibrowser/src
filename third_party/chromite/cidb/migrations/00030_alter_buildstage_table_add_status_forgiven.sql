ALTER TABLE buildStageTable
  MODIFY COLUMN status ENUM('fail', 'pass', 'inflight', 'missing', 'aborted',
                            'planned', 'skipped', 'forgiven');

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (30, '00030_alter_buildstage_table_add_status_forgiven.sql');
