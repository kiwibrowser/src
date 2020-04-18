ALTER TABLE annotationsTable
  ADD COLUMN deleted BOOL NOT NULL DEFAULT false;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (39, '00039_alter_annotations_table_add_deleted.sql');
