CREATE TABLE annotationsTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT NOT NULL,
  last_updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  last_annotator VARCHAR(80),
  -- The following ENUM values should match constants.FAILURE_CATEGORY_ALL_CATEGORIES
  failure_category ENUM(
      'bad_cl', 'bug_in_tot', 'merge_conflict', 'tree_closed',
      'scheduled_abort', 'cl_not_ready', 'bad_chrome',
      'infra_failure', 'test_flake', 'gerrit_failure', 'gs_failure',
      'lab_failure', 'bad_binary_package', 'build_flake', 'mystery'
  ) DEFAULT 'mystery',
  failure_message VARCHAR(1024),
  blame_url VARCHAR(512),
  notes VARCHAR(1024),

  PRIMARY KEY (id),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id),
  INDEX (build_id),
  INDEX (last_updated)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (26, '00026_create_annotations_table.sql');
