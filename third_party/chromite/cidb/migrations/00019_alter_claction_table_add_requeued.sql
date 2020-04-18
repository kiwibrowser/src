ALTER TABLE clActionTable
  MODIFY action ENUM('picked_up',
                     'submitted',
                     'kicked_out',
                     'submit_failed',
                     'verified',
                     'pre_cq_inflight',
                     'pre_cq_passed',
                     'pre_cq_failed',
                     'pre_cq_launching',
                     'pre_cq_waiting',
                     'pre_cq_ready_to_submit',
                     'requeued')
    NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (19, '00019_alter_claction_table_add_requeued.sql');
