
CREATE TABLE failureTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_stage_id INT NOT NULL,
  outer_failure_id INT,
  exception_type VARCHAR(240),
  exception_message VARCHAR(240),
  -- This should match constants.EXCEPTION_CATEGORY_ALL_CATEGORIES
  exception_category ENUM('unknown', 'build', 'test', 'infra', 'lab') NOT NULL
    DEFAULT 'unknown',
  extra_info VARCHAR(240),
  timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  FOREIGN KEY (build_stage_id)
    REFERENCES buildStageTable(id),
  FOREIGN KEY (outer_failure_id)
    REFERENCES failureTable(id)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (29, '00029_create_failure_table.sql');
