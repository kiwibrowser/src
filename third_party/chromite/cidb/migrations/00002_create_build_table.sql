CREATE TABLE buildTable (
  id INT NOT NULL AUTO_INCREMENT,
  master_build_id INT,
  buildbot_generation INT NOT NULL,
  builder_name VARCHAR(80) NOT NULL,
  waterfall ENUM('chromeos', 'chromiumos', 'chromiumos.tryserver') NOT NULL,
  build_number INT NOT NULL,
  build_config VARCHAR(80) NOT NULL,
  bot_hostname VARCHAR(80) NOT NULL,
  -- Specifying a DEFAULT value without an ON UPDATE clause allows
  -- UPDATE queries to other columns that do not automatically update
  -- start_time to CURRENT_TIMESTAMP
  start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  finish_time TIMESTAMP,
  -- The following ENUM values should match
  -- manifest_version.BuilderStatus.All_STATUSES
  status ENUM('fail', 'pass', 'inflight', 'missing', 'aborted')
    DEFAULT 'inflight' NOT NULL,
  status_pickle BLOB,
  build_type VARCHAR(80),
  chrome_version VARCHAR(80),
  milestone_version VARCHAR(80),
  platform_version VARCHAR(80),
  full_version VARCHAR(80),
  sdk_version VARCHAR(80),
  toolchain_url VARCHAR(240),
  metadata_json BLOB,
  final BOOL NOT NULL DEFAULT false,
  PRIMARY KEY (id),
  FOREIGN KEY (master_build_id)
    REFERENCES buildTable(id),
  UNIQUE INDEX (buildbot_generation, builder_name, waterfall, build_number),
  INDEX (master_build_id)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (2, '00002_create_build_table.sql');
