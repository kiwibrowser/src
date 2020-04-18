CREATE TABLE childConfigPerBuildTable (
  build_id INT NOT NULL,
  child_config VARCHAR(80) NOT NULL,
  PRIMARY KEY (build_id, child_config),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id),
  INDEX (build_id)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (7, '00007_create_childconfigperbuild_table.sql');
