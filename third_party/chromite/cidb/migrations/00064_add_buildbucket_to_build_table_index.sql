ALTER TABLE buildTable
  ADD UNIQUE KEY `unique_externals` (
    `buildbot_generation`,
    `builder_name`,
    `waterfall`,
    `build_number`,
    `buildbucket_id`
  ),
  DROP INDEX buildbot_generation;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (64, '00064_add_buildbucket_to_build_table_index.sql');
