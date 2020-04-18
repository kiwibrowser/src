Commented test data sample
--------------------------
All of the attributes are REQUIRED unless otherwise noted.

<font_test_data (table information under here)
   path="Tuffy.ttf" (path to the font file)
   post_name="Tuffy" (postscript name of the font; OPTIONAL)
   sha1="59a4bf055e1fd30d56ecc89760861fa0655252bb" (sha1 of the font file)
   >
  <cmap_table (cmap information under here)
     num_cmaps="3" (number of cmaps in the original file)
     >
    <cmap (mappings under here)
       byte_length="288" (length of the original cmap)
       encoding_id="3" (encoding id of the cmap)
       format="4" (format of the cmap)
       language="0" (language of the cmap)
       num_chars="195" (original cmap)
       platform_id="0" (platform id for the cmap)
       >
      <map (mapping from char to gid)
	 char="0x020ac" (hex value of the code point being mapped)
	 gid="196" (glyph id corresponding to the code point)
	 name="Euro" (name of the unicode character; OPTIONAL)
	 />
    </cmap>
  </cmap_table>
</font_test_data>
