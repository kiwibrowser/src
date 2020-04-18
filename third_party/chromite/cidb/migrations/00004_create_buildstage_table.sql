CREATE TABLE buildStageTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT NOT NULL,
  name VARCHAR(80) NOT NULL,
  board VARCHAR(80),
  status ENUM('fail', 'pass') NOT NULL,
  log_url VARCHAR(240),
  duration_seconds INT NOT NULL,
  summary BLOB,
  PRIMARY KEY (id),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id),
  INDEX (build_id)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (4, '00004_create_buildstage_table.sql');
