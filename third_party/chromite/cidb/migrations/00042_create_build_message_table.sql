CREATE TABLE buildMessageTable (
  id INT NOT NULL AUTO_INCREMENT,
  build_id INT NOT NULL,
  message_type VARCHAR(240),
  message_subtype VARCHAR(240),
  message_value VARCHAR(480),
  timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY(id),
  FOREIGN KEY (build_id)
    REFERENCES buildTable(id)
);


INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (42, '00042_create_build_message_table.sql');
