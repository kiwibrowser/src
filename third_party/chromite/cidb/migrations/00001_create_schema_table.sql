CREATE TABLE schemaVersionTable (
  schemaVersion INT NOT NULL,
  scriptName VARCHAR(80),
  timestamp TIMESTAMP,
  PRIMARY KEY (schemaVersion)
);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (1, '00001_create_schema_table.sql');
