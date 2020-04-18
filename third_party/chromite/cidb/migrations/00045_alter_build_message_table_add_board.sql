ALTER TABLE buildMessageTable
  ADD COLUMN board VARCHAR(240) DEFAULT NULL;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (45, '00045_alter_build_message_table_add_board.sql');

