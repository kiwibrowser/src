ALTER TABLE buildRequestTable
  MODIFY request_reason ENUM('sanity-pre-cq',
                             'important_cq_slave',
                             'experimental_cq_slave') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (61, '00061_alter_buildRequest_table_fix_request_reasons.sql');
