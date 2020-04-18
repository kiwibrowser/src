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
                     'requeued',
                     'screened_for_pre_cq',
                     'validation_pending_pre_cq',
                     'irrelevant_to_slave',
                     'trybot_launching',
                     'speculative',
                     'forgiven',
                     'pre_cq_fully_verified',
                     'pre_cq_reset',
                     'trybot_cancelled',
                     'relevant_to_slave',
                     'exonerated')
    NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (62, '00062_alter_claction_table_add_exonerated.sql');
