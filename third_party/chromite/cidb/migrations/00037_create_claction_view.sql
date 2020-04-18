CREATE VIEW clActionView as
  SELECT c.*,
    b.last_updated, b.master_build_id, b.buildbot_generation, b.builder_name,
    b.waterfall, b.build_number, b.build_config, b.bot_hostname, b.start_time,
    b.finish_time, b.status, b.build_type, b.chrome_version,
    b.milestone_version, b.platform_version, b.full_version, b.sdk_version,
    b.toolchain_url, b.final, b.metadata_url, b.summary, b.deadline
 FROM clActionTable c JOIN buildTable b on c.build_id = b.id;

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (37, '00037_create_claction_view.sql')
