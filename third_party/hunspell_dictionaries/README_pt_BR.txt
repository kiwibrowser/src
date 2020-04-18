README_pt_BR

:: PORTUGUÊS ::
Este dicionário é baseado na versão ispell do dicionário de Português e no script "conjugue" de Ricardo Ueda Karpischek mailto:ueda@ime.usp.br ambos disponíveis em http://www.ime.usp.br/~ueda/br.ispell/ e estão cobertos, no original, pela licença versão 2 da Licença Pública Geral da Fundação para o Software Livre (FSF GPL).

Todas as modificações à lista de palavras e ao arquivo affix que permitem que o original, supra referido, funcionem com MySpell foram feitas por Augusto Tavares Rosa Marcacini mailto:amarcacini@adv.oabsp.org.br e estão cobertas pela mesma versão 2 da Licença Pública Geral da Fundação para o Software Livre (FSF GPL).



:: ENGLISH ::
This dictionary is based on a ispell version of the Portuguese dictionary and the "conjugue" script created by Ricardo Ueda Karpischek mailto:ueda@ime.usp.br both available in http://www.ime.usp.br/~ueda/br.ispell/ and thus are covered by his original Free Software Foundation (FSF)  GPL version 2 license.

All modifications to the affix file and wordlist to make it work with MySpell were done by Augusto Tavares Rosa Marcacini mailto:amarcacini@adv.oabsp.org.br and covered by the same GPL version 2 license.


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
PARA INSTALAR O CORRETOR BRASILEIRO NO OPENOFFICE:
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Copie os arquivos pt_BR.dic e pt_BR.aff para o diretório <OpenOffice.org>/user/wordbook/, onde <OpenOffice.org> é o diretório em que o programa foi instalado.

No mesmo diretório, localize o arquivo dictionary.lst. Abra-o com um editor de textos e acrescente a seguinte linha ao final:

DICT pt BR pt_BR

É necessário reiniciar o OpenOffice, inclusive o início rápido da versão para Windows que fica na barra de tarefas. Após, é necessário ativar a correção em português brasileiro. Ver em Ferramentas->Opções->Configuração da Língua->Linguística->Editar.

Também será necessário configurar a língua do seu texto, na mesma janela de formatação dos caracteres, em Formatar->Caracteres. É possível alterar o padrão (até que seja distribuída uma versão do OpenOffice em português brasileiro) em Ferramentas->Opções->Configuração da Língua->Línguas, para que todos os novos documentos se iniciem automaticamente em português brasileiro.

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
HISTÓRICO
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

*** Versão 06.08.2002 ***

Segunda versão. Contém 25165 palavras no arquivo pt_BR.dic. Várias palavras estavam duplicadas na primeira versão, por isso o número diminuiu...

Já são 2960 verbos conjugados, dos 4100 presentes no dicionário. Em relação à versão anterior, foram acrescentados quase dois mil verbos regulares da primeira conjugação. Estou no momento fazendo a parte mais fácil, é claro! :-) 

Corrigido bug no arquivo pt_BR.aff: uma letra trocada, que era "tolerada" na versão 1.0.0 do OpenOffice, causa erro fatal na versão 1.0.1, provocando o fechamento do editor de textos.

*** Versão 03.07.2002 ***

Primeira versão. Contém 25210 palavras no arquivo pt_BR.dic.

Este dicionário foi elaborado a partir da lista de palavras da versão brasileira do ispell e do conjugador de verbos conjugue, ambos de Ricardo Ueda Karpischek mailto:ueda@ime.usp.br. Palavras novas foram acrescentadas, muitas delas do jargão jurídico.

O arquivo affix (pt_br.aff) foi adaptado para funcionar com o MySpell (OpenOffice.org), criando-se várias extensões novas.

A relação de palavras deve cobrir boa parte do vocabulário mais usual. Certamente há muito o que ser acrescentado. Há cerca de 4100 verbos no infinitivo, dos quais apenas 1009 estão conjugados. 

A grande maioria dos verbos não conjugados deve ser regular, de modo que basta acrescentar "/R" após a palavra (no arquivo pt_BR.dic) para que todas as variantes sejam acrescidas. Em todo caso, é necessário verificar um a um quais verbos são regulares (tomar cuidado com acentuação e cedilhas, que fazem com que verbos regulares tenham que ser tratados de modo próprio) e quais são irregulares.

No caso dos verbos irregulares, alguns talvez se encaixem nos padrões já definidos no arquivo affix; para outros, será necessário criar novas extensões. Na próxima versão devem ser incluídas novas extensões para mais alguns padrões de verbos irregulares.

