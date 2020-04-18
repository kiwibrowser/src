README.pt_PT

:: PORTUGUÊS ::
Este dicionário é baseado na versão ispell do dicionário de Português criado por José João de Almeida e Ulisses Pinto mailto:jj@di.uminho.pt e está coberta, no original, pela licença versão 2 da Licença Pública Geral da Fundação para o Software Livre (FSF GPL).

Todas as modificações à lista de palavras e ao ficheiro affix que permitem que o original, supra referido, funcionem com MySpell são da autoria de Artur Correia mailto:artur.correia@netvisao.pt e estão cobertas pela mesma versão 2 da Licença Pública Geral da Fundação para o Software Livre (FSF GPL).



:: ENGLISH ::
This dictionary is based on a the ispell version of the Portuguese dictionary created by Jose Joao de Almeida and Ulisses Pinto mailto:jj@di.uminho.pt and thus is covered by his original Free Software Foundation (FSF)  GPL version 2 license.

All modification to the affix file and wordlist to make it work with MySpell are copyright Artur Correia mailto:artur.correia@netvisao.pt and covered by the same GPL version 2 license.


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
HISTÓRIA
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Versão 20.5.2002
Esta versão é completamente diferente da anterior.
1. Utilizámos a nova versão do ispell para portugues como base do trabalho (o ficheiro affix e o dicionário são novos)
2. Utilizou-se um script de perl para a transformação do ficheiro affix para myspell. Supostamente agora temos menos bugs.
3. Graças à preciosa colaboração de Daniel Andrade (mop12079@mail.telepac.pt), que encontrou - e resolveu - este bug, agora as palaras sugeridas incluem também palavras com caracteres portugeses (çãàâ, etc...)
4. Para além disso, aproveitam a esta versão todos os bugfixes anteriores.

Versão 16.4.2002
Bugfix - corrigido o bug (pt_PT.aff) que provocava alguns crashes em OpenOffice.org (myspell). O erro devia-se a uma contagem incorrecta de número de sugestões. Arranjado.

Versão 8.4.2002
Bugfix - corrigido o bug (pt_PT.aff) que fazia algumas palavras terminadas em ção serem reconhecidas como erro e sugerida palavra terminada em rão.