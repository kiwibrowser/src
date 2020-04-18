CREATE TABLE buildRequestTable (
  id INT NOT NULL AUTO_INCREMENT,
  -- build_id of the build which sends the request
  build_id INT NOT NULL,
  -- build_config of the requested build
  request_build_config VARCHAR(80) NOT NULL,
  -- build_args of the requested build
  request_build_args VARCHAR(240) DEFAULT NULL,
  -- buildbucket_id of the requested build
  request_buildbucket_id varchar(80) DEFAULT NULL,
  -- reason of the request
  request_reason ENUM('sanity-pre-cq') NOT NULL,
  -- timestamp of the request
  timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY(id),
  FOREIGN KEY(build_id) REFERENCES buildTable(id),
  INDEX (timestamp)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (59, '00059_create_buildRequest_table.sql');