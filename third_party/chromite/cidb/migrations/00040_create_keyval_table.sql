CREATE TABLE keyvalTable (
  k VARCHAR(240) NOT NULL,
  v VARCHAR(240),
  PRIMARY KEY(k)
);


INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (40, '00040_create_keyval_table.sql');
