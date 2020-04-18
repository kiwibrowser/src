ALTER TABLE buildTable
  ADD UNIQUE INDEX buildbucket_id_index(buildbucket_id);

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (51, '00051_alter_build_table_add_buildbucket_id_index.sql');
