-- We want to add an auto-updating last_updated column. In MySQL prior to 5.6,
-- only one column timestamp column in the table may reference CURRENT_TIMESTAMP,
-- and for it to work as expected it must be the lowest-numbered column of type
-- TIMESTAMP. Thus, this alter table simultaneously changes the type of start_time
-- to no longer reference CURRENT TIME, adds the last_updated column as the table's
-- second column, and creates an index on the new column.
ALTER TABLE buildTable
  MODIFY start_time TIMESTAMP DEFAULT 0,
  ADD COLUMN last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP AFTER id,
  ADD INDEX last_updated_index(last_updated);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (9, '00009_alter_build_table_add_last_updated.sql');
