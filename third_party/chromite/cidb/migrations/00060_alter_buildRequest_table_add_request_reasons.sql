ALTER TABLE buildRequestTable
  MODIFY request_reason ENUM('sanity-pre-cq',
                             'important_cq_slave',
                             'exprimental_cq_slave') NOT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (60, '00060_alter_buildRequest_table_add_request_reasons.sql');
