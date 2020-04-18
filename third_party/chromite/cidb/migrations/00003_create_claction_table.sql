CREATE TABLE clActionTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT NOT NULL,
  change_number INT NOT NULL,
  patch_number INT NOT NULL,
  change_source ENUM('internal', 'external') NOT NULL,
  -- The following ENUM values should match constants.py:CL_ACTIONS
  action ENUM('picked_up', 'submitted', 'kicked_out', 'submit_failed') NOT NULL,
  reason VARCHAR(80),
  timestamp TIMESTAMP NOT NULL,
  PRIMARY KEY (id),
  FOREIGN KEY(build_id)
    REFERENCES buildTable(id),
  INDEX (change_number, change_source),
  INDEX (change_number, patch_number, change_source)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (3, '00003_create_claction_table.sql');
