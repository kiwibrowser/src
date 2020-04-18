DROP TABLE buildStageTable;

CREATE TABLE buildStageTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT NOT NULL,
  name VARCHAR(80) NOT NULL,
  board VARCHAR(80),
  -- This should match constants.BUILDER_ALL_STATUSES
  status ENUM('fail', 'pass', 'inflight', 'missing', 'aborted', 'planned',
              'skipped'),
  last_updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  start_time TIMESTAMP DEFAULT 0,
  finish_time TIMESTAMP DEFAULT 0,
  final BOOL NOT NULL DEFAULT false,

  PRIMARY KEY (id),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id),
  INDEX (build_id),
  INDEX (last_updated)
);


INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (28, '00028_remake_buildstage_table.sql');
