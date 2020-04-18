ispell-lt
=========

You are looking at the dictionaries and affix files for spellchecking
of Lithuanian texts.

The latest version of the affix tables and dictionaries can be found
at ftp://ftp.akl.lt/ispell-lt/ .  The mailing list of the project is
available at https://lists.akl.lt/mailman/listinfo/ispell-lt .  A
browsable web interface to the project CVS repository is available at
http://sraige.mif.vu.lt/cvs/ispell-lt/

The software is available under the provisions of a BSD-style license.
The full text of the license is available in the COPYING file.

The project has been sponsored by the Information Society Development
Committee of the Government of Republic of Lithuania.

Albertas Agejevas <alga@akl.lt>
31-December-2003


Add this line to "dictionary.lst" when OOo nor Quickstarter are NOT executing:
DICT lt LT lt_LT



		Lietuviðkø MySpell afiksø lenteliø diegimo instrukcijos

		Ramûnas Lukaðevièius (Ramunas.Lukasevicius@mail.lt)
		Albertas Agejevas (alga@akl.lt)

	1. Áþanga
	2. Lenteliø ádiegimas á Mozillà
	3. Lenteliø ádiegimas á OpenOffice



	1. Áþanga

Ðis failas skirtas apraðymams kaip vienoje ar kitoje operacinëje
sistemoje tam tikroms programoms ádiegti afiksø lenteles kad tikrintø
lietuviø kalbos raðybà.

Þemiau pateikti apraðymai yra skirti ðiek tiek paþengusiems
vartotojams.  Ateityje bûtø galima sukurti koká skriptà, palengvinantá
ádiegimà.

	2. Lenteliø ádiegimas á Mozilla

Standartiniame Mozillos ádiegime iki versijos 1.5 nëra ádiegiamas
raðybos tikrinimo komponentas, todël reikia já papildomai atsisiøsti
ir ásidiegti ið interneto adresu
http://spellchecker.mozdev.org/installation.html Ádiegus ðá paketà,
raðant laiðkà, ar kuriant puslapá, meniu turi atsirasti mygtukas
"spell".  Mozilla 1.5 ir vëlesnioms versijoms atskirai raðybos
tikrinimo paketo diegti nereikia, pakanka tik ádiegti þodynà ir
lenteles.

Diegiant afiksø lenteles uþtenka nukopijuoti lt_LT.dic ir lt_LT.aff
bylas á $mozilla/components/myspell katalogà.  Èia $mozilla yra
Mozilla ádiegimo katalogas, pvz. Windows platformoje 
"C:\Program Files\mozilla.org\Mozilla", o Linux platformoje
/usr/lib/mozilla.

	3. Lenteliø ádiegimas á OpenOffice

Þemiau pateiktos instrukcijos, kaip ádiegti þodynà á OpenOffice.org
versijas 641C, 1.0 ir vëlesnius:

1) prieð pradedant ádiegimà, reikia uþdaryti visus OpenOffice langus,
   netgi Quickstarter.

2) nukopijuojame lt_LT.dic, lt_LT.aff, ir dictionary.lst á
   $OpenOffice/user/wordbook/ Èia $OpenOffice yra OpenOffice.org
   ádiegimo katalogas, pvz. Windows platformoje "C:\Program
   Files\OpenOffice.org.1.1.0\", o Linux platformoje
   ~/.openoffice/1.1.0/.

3) Dabar paleidþiame OpenOffice ir vykdom tokias komandas:
   Tools->Options->LanguageSettings->WritingAids
   Paspaudþiame Edit (prie Available language modules), pasirenkame
   lietuviø kalbà ir paþymime "OpenOffice MySpell spell checker".

4) Dabar pasirenkame Tools->Options->LanguageSettings->Languages ir
   pasirenkame Lietuviðkà lokalæ, bei nustatome lietuviø kalbà kaip
   nutylimàjà dokumentams lotyniðkomis raidëmis (ties "Western").

Þodynas ádiegtas ir uþregistruotas á OpenOffice.

Apie ádiegimà galima pasiskaityti ir adresu
http://whiteboard.openoffice.org/lingucomponent/download_dictionary.html#installspell


