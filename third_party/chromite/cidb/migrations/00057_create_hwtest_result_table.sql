CREATE TABLE hwTestResultTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT NOT NULL,
  test_name VARCHAR(240) NOT NULL,
  status ENUM('fail', 'pass', 'abort', 'other') NOT NULL,
  PRIMARY KEY (id),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id),
  INDEX (build_id)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (57, '00057_create_hwtest_result_table.sql');
