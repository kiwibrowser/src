-- The failureView consists of:
-- all failureTable columns, by original name.
-- all buildStageTable columns except build_id, and its own id (which come
-- from f.*), with non-colliding names
-- all buildTable columns, with non-colliding names
ALTER VIEW failureView AS
  SELECT f.*,
    bs.name AS stage_name, bs.board, bs.status AS stage_status,
    bs.last_updated AS stage_last_updated, bs.start_time AS stage_start_time,
    bs.finish_time AS stage_finish_time, bs.final AS stage_final,
    b.id AS build_id, b.last_updated AS build_last_updated, b.master_build_id,
    b.buildbot_generation, b.builder_name, b.waterfall, b.build_number,
    b.build_config, b.bot_hostname, b.start_time AS build_start_time,
    b.finish_time AS build_finish_time, b.status AS build_status, b.build_type,
    b.chrome_version, b.milestone_version, b.platform_version, b.full_version,
    b.sdk_version, b.toolchain_url, b.final AS build_final, b.metadata_url,
    b.summary, b.deadline
  FROM failureTable f JOIN buildStageTable bs on f.build_stage_id = bs.id
                      JOIN buildTable b on bs.build_id = b.id;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (36, '00036_alter_failure_view.sql')
