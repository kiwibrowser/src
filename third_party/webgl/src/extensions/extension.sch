<?xml version="1.0" encoding="UTF-8" ?>
<schema xmlns="http://purl.oclc.org/dsdl/schematron">
  <title>WebGL Extension Validity Schematron Schema</title>
  <!--<ns prefix="" uri="" />-->

  <pattern name="Stages" id="stage-patt">
    <rule context="/rejected">
      <assert test="self::node()[starts-with(@href,'rejected/')]"
              >A rejected's @href belongs in the 'rejected' directory.</assert>
      <assert test="self::node()[@href=concat('rejected/',name,'/')]"
              >A rejected extension should have a URL matching its name and ending in '/'.</assert>
    </rule>

    <rule context="/proposal">
      <assert test="self::node()[starts-with(@href,'proposals/')]"
              >A proposal's @href belongs in the 'proposals' directory.</assert>
      <assert test="self::node()[@href=concat('proposals/',name,'/')]"
              >A proposal should have a URL matching its name and ending in '/'.</assert>
    </rule>

    <rule context="/ratified | /extension | /draft">
      <assert test="self::node()[@href=concat(name,'/')]"
              >An extension should have a URL matching its name and ending in '/'.</assert>
    </rule>
  </pattern>

  <pattern name="Extensions" id="extension-patt">
    <rule context="/*">
      <assert test="self::node()[@href=concat($path,'/')]"
              >An extension should be stored under its @href path.</assert>
    </rule>
    <rule context="idl">
      <assert test="self::node()[@xml:space='preserve']"
              >IDL blocks should always preserve internal space.</assert>
    </rule>
  </pattern>
</schema>
