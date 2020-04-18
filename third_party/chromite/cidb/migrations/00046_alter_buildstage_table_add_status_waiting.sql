ALTER TABLE buildStageTable
  MODIFY COLUMN status ENUM('fail', 'pass', 'inflight', 'missing', 'aborted',
                            'planned', 'skipped', 'forgiven','waiting');

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (46, '00046_alter_buildstage_table_add_status_waiting.sql');
