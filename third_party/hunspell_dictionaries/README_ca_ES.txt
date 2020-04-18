Diccionari MySpell 0.1
----------------------

L'objectiu d'aquest projecte és crear un corrector ortogràfic multiplataforma de lliure distribució que es pugui fer servir en tots els programes que s'estan traduint al català. L'idea es crear un lèxic i unes regles ortogràfiques i realitzar versions de les mateixes per iSpell, MySpell, AsPell. S'han tingut molt en compte les diferents variants dialectals, especialment el valencià i el balear.

L'única versió d'un corrector en català d'aquesta mena que coneixem és l'Ispell d'en Ignasi Labastida que podeu trobar aquí. Durant molt anys ha estat la peça en català de referència en aquesta àrea, des de Softcatalà hem decidit començar de nou en quelcom més sofisticat i elaborat. La feina de l'Ignasi ens ha servit per afinar i constrastar la nostra feina.

Que s'ha fet
------------

Bé, s'ha creat un lèxic i les corresponents regles perquè el diccionari pugui declinar les paraules correctament. Actualment el diccionari funciona bé però hi ha encara problemes que s'han de corregir. Si vols conèixer amb més detall el seu estat baixa-te'l.

Coses pedents
-------------

Provar el corrector per detectar-ne els errors. Els primers que poden inaugurar la llista:

- Falten plurals i femenins (p.e ell/ella/ells/elles). S'haurien de provar  sobretot els pronoms.
- Hi ha paraules no admeten la preposició de (p.e d'on). Crec que són  sobretot adverbis i pronoms.
- Es considera incorrecte "És" (en majúscules). .
- Si s'accepta una proposta amb un apòstrof, OpenOffice no el canvia per un  apòstrof tipogràfic. Sospit que això no es pot arreglar des de MySpell, però s'hauria d'investigar més
- Els suggeriments no tenen gaire qualitat, ja que no es comença pels  errors més habituals, ni es proposen paraules a més d'una letra de  distància. Crec que tenc un pla que pot marxar per arreglar les dues coses alhora.

Llicència
---------

Catalan wordlist for MySpell version 0.1
Copyright (C) 2002 Joan Moratinos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


===================
NOTAS de ACTIVACION
===================
Hay una problema en la manera de activar la diccionario de Catalan.
Aqui, una copia del comunicación respecto al problema:

 Hola Richard:
 Esta solución que me has dado ha funcionado ya que ahora corrige en
 catalán. Es una solución temporal, pero lo importante es que se pueda 
 utilizar el diccionario.
 Juan José

 Richard Holt escribió:

 ...
 >>del todo lo podría solucionar. Te agradezco el interés. Verás aquí en

 >>Barcelona nadie querrá usar el OpenOffice a no ser que sea capaz de 
 >>corregir el catalán y me interesa como alternativa al Word en el
 >>colegio donde trabajo.
 >>
 >>
 >
 >Hola Juan,
 >
 >Yo se que recibiste la respuesta de Kevin y que ya Tomas, quien esta
 >manteniendo la diccionario, estan buscando una solucion. 
 >
 >Aqui una traduccion en mi espanglish (no se como esta su ingles pero
 >quizás sera util):
 >
 >Escoje una idioma que no esta usando, asignarlo en dictionary.lst:
 >
 >DICT es CO ca_ES
 >
 >para prestar lo de Colombia pero se ve más cerca de Catalan, más o
 >menos. 
 >
 >Este quiere decir que ahora, Catalan van a parecer como Español
 >(Colombia), y tienes que fijar la idioma en que trabajas a Colombia,
 >pero segun lo funciona. No es muy elegante pero pronto consigas una
 >verdadero. 
 >
 >"The only workaround is to assign the Catalan dictionary to some
liitle used  locale and set your language appropriately to actually
spellcheck  something."

saludos,
Richard.
http://es.openoffice.org
Table of contents
=================
LICENSES-en.txt
LLICENCIES-ca.txt
README.chromium

LICENSES-en.txt
===============
LICENSES :
Spelling dictionary: LGPL, GPL
LLICENCIES-ca.txt
=================
LLICÃˆNCIES:
Diccionari ortogrÃ fic : LGPL, GPL

README.chromium
===============
Name: A Calalan spellchecking dictionary
URL: http://www.softcatala.org/wiki/Rebost:Corrector_ortogr%C3%A0fic_de_catal%C3%A0_%28general%29_per_a_l%27OpenOffice.org
Version: 2.3.0
LICENSE File: LICENSES-en.txt
Security Critical: no
Description:
This folder contains a partial copy of the Catalan dictionary for hunspell.

