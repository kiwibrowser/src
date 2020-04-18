CREATE TABLE boardPerBuildTable (
  build_id INT NOT NULL,
  board VARCHAR(80) NOT NULL,
  main_firmware_version VARCHAR(80),
  ec_firmware_version VARCHAR(80),
  PRIMARY KEY (build_id, board),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id),
  INDEX (build_id)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (6, '00006_create_boardperbuild_table.sql');
