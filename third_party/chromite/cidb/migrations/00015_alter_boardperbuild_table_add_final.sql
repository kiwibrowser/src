ALTER TABLE boardPerBuildTable
  ADD COLUMN last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  ADD COLUMN final BOOL NOT NULL DEFAULT false,
  ADD INDEX last_updated_index(last_updated);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (15, '00015_alter_boardperbuild_table_add_final.sql');
