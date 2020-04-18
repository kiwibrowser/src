CREATE INDEX build_config_index on buildTable (build_config);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (8, '00008_create_build_config_index.sql');
