ALTER TABLE childConfigPerBuildTable
  ADD COLUMN last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  ADD COLUMN status ENUM('fail','pass','inflight','missing','aborted')
    NOT NULL DEFAULT 'inflight',
  ADD COLUMN final BOOL NOT NULL DEFAULT false,
  ADD INDEX last_updated_index(last_updated);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (16, '00016_alter_childconfigperbuild_table_add_status.sql');
