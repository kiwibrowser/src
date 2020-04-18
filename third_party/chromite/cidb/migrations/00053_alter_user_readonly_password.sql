SET PASSWORD FOR readonly = '*B5FD9E9B380D4AD30AE9E52C41DB645437F47013';

SET PASSWORD for readonly@localhost = '*B5FD9E9B380D4AD30AE9E52C41DB645437F47013';

INSERT INTO schemaVersionTable (schemaVersion, scriptName) VALUES
  (53, '00053_alter_user_readonly_password.sql');
