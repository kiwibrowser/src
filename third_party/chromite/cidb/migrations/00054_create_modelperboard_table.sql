CREATE TABLE modelPerBoardTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT,
  board VARCHAR(80) NOT NULL,
  model_name VARCHAR(80) NOT NULL,
  main_firmware_version varchar(80) DEFAULT NULL,
  ec_firmware_version varchar(80) DEFAULT NULL,
  timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY(id),
  FOREIGN KEY (build_id, board)
    REFERENCES boardPerBuildTable(build_id, board)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (54, '00054_create_modelperboard_table.sql');
