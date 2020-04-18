CREATE VIEW failureView as
  SELECT f.*, bs.name as stage_name, bs.board, bs.status as stage_status,
         b.id as build_id, b.build_config, b.status as build_status,
         b.final as build_final, b.full_version, b.chrome_version,
         b.sdk_version, b.milestone_version, b.master_build_id
  FROM failureTable f JOIN buildStageTable bs on f.build_stage_id = bs.id
                      JOIN buildTable b on bs.build_id = b.id;

GRANT SHOW VIEW ON cidb.* to bot, readonly, annotator;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (35, '00035_create_failure_view.sql')
