-- Add buildbucket_id column to failureView.
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
    b.summary, b.deadline, b.important, b.buildbucket_id
  FROM failureTable f JOIN buildStageTable bs on f.build_stage_id = bs.id
                      JOIN buildTable b on bs.build_id = b.id;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (52, '00052_alter_failureView_add_buildbucket_id.sql')
