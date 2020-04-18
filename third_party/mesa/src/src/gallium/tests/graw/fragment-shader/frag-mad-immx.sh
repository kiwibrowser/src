FRAG

DCL IN[0], COLOR, LINEAR
DCL OUT[0], COLOR
DCL IMMX[0..1]  {{ 0.5, 0.4, 0.6, 1.0 },
                 { 0.5, 0.4, 0.6, 0.0 }}

MAD OUT[0], IN[0], IMMX[0], IMMX[1]

END
