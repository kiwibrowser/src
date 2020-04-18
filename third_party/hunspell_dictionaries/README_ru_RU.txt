Source
======
http://extensions.services.openoffice.org/project/dict_ru_RU

Table of contents
=================
LICENSE
README
README.koi

LICENSE
=======
* Copyright (c) 1997-2008, Alexander I. Lebedev

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Modified versions must be clearly marked as such.
* The name of Alexander I. Lebedev may not be used to endorse or promote
  products derived from this software without specific prior written
  permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

README
======

	New Russian dictionaries for ispell (version 0.99g5)

Author:
	Alexander Lebedev (http://scon155.phys.msu.su/eng/lebedev.html)
Primary-site:
	ftp://scon155.phys.msu.su/pub/russian/ispell/rus-ispell.tar.gz

This package contains a few lists of Russian words and an affix file to
be used with a well-known ispell spelling checker.  It may be especially
useful for UNIX users.  However, as the package supports five different
character sets used for Russian, it may be of interest for DOS, OS/2
and Windows users too.

The word lists are based on the list collected about ten years ago by
Neal Dalton.  After removing mistaken words and adding many new rules
and words, the dictionaries contain now over 139,000 basic words and
produce over 1,367,000 related words (compared to 52,000 words in Neal's
version and 952,000 words in the dictionary by K. Knizhnik).

This package seems to be the only one that supports the right spelling
of words with the Russian letter 'yo' (other dictionaries simply
replace the letter 'yo' by 'ye').

Further detailed documentation on the package is in Russian (koi8-r
character set) in README.koi file.


README.koi
==========

		   Словарь русского языка для ispell

			    Версия - 0.99g5
      Автор: Александр Лебедев (http://scon155.phys.msu.su/~swan/)

Предлагаемый орфографический словарь русского языка представляет интерес
прежде всего для пользователей системы UNIX, в которой набор средств для
проверки правописания весьма ограничен.  Однако словарь может быть
полезен и для пользователей, работающих в операционных системах Windows,
DOS и OS/2, поскольку словарь поддерживает пять различных кодировок русских
букв (см. ниже).  Встроенные в ispell функции позволяют считать его удобным
средством для работы с текстами, подготовленными в редакторе TeX, а также
с файлами в формате HTML. Вариант словаря с набором слов и affix-файлом,
подготовленными для программы MySpell, используется для проверки орфографии
в системе OpenOffice.  Кроме этого, словарь может использоваться в
локальных и глобальных поисковых системах, строящихся на основе программ,
подобных HTDig и ASPseek.

Исторически предлагаемый словарь возник из небольшого словаря русского
языка для ispell, составленного Нилом Далтоном (Neal Dalton) в 1992 г.
на основе текстов, которые автор нашел в Интернет.  Словарь Далтона был
очень небольшим (всего 52 тысячи словоформ), содержал большое число
ошибок (более 8% слов) и имел слабо развитый файл преобразования
окончаний слов (affix-файл).

На первом этапе (работа была начата в конце 1997 года) в affix-файл
словаря были добавлены отсутствовавшие в нем правила образования форм
существительных, прилагательных, причастий, наречий, изменены правила
формирования окончаний глаголов, так что affix-файл можно считать созданным
заново.  Основным подходом, положенным в основу настоящего словаря, было
использование нормализованной формы слова и правил словоизменения,
отвечающих грамматике русского языка, а не традиционное использование
программы munchlist для создания словарей для ispell.  По этой причине
этот словарь одновременно содержит и важную информацию о морфологии слов,
которая необходима для современных русскоязычных поисковых систем.  При
вычитке словаря Далтона конкретные слова были приведены к нормализованной
форме и из словаря было исключено большинство ошибочных слов.  На этом
этапе работы мной использовался орфографический словарь русского языка,
выпущенный Институтом русского языка АН СССР в 1991 г.  Одновременно в
словарь было добавлено большое число слов, взятых из технических и
литературных текстов.

На втором этапе все имеющиеся в словаре слова были перепроверены с
помощью электронного орфографического словаря "Корректор" (120 тысяч
слов), а обнаруженные расхождения выверены по орфографическому словарю
русского языка, справочнику Зализняка ("Грамматический словарь русского
языка: словоизменение", 100 тысяч слов) и "Сводному словарю современной
русской лексики" (170 тысяч слов).

Все новые слова, добавляемые в словарь после этого, проходят проверку
с помощью указанных выше орфографических словарей (в специальных
областях -- энциклопедий) и с помощью нового издания "Русского
орфографического словаря" под редакцией Лопатина (160 тысяч слов).
Слова, отсутствующие в этих изданиях, добавляются если они действительно
широко используются и в их написании нет никаких сомнений.  В настоящей
версии объем словаря составляет более 139 тысяч базовых слов, а полное
число образуемых из них словоформ превышает 1.367 миллиона (против 52 тысяч
в словаре Нила Далтона и 952 тысяч слов в имеющимся в свободном доступе
в Интернет словаре Константина Книжника, в котором, как оказалось при
выборочной проверке, содержится 6% ошибочных слов).

Отличительной чертой данного словаря от всех известных автору других
орфографических словарей является то, что начиная с версии 0.99c0 в него
включена полноценная поддержка буквы ё (другие словари просто заменяют
букву ё на букву е).  Это может быть очень полезно при подготовке литературы
для детей младшего возраста, изданий для иностранцев, орфографических
справочников -- там, где правила издания требуют использования буквы ё.

Словарь base.koi, а также шесть дополнительных словарей (abbrev.koi,
computer.koi, for_name.koi, geography.koi, rare.koi, science.koi) и файл
russian.aff.koi поставляются в кодировке koi8-r.  Для преобразования
текстов в другие поддерживаемые кодировки -- cp866 (alt), iso-8859-5,
cp1251 (win) и maccyrillic -- в комплект включен скрипт-перекодировщик
trans, написанный Владимиром Воловичем.

Внимание!  В словарь rare.koi выделены некоторые редкие слова, написание
которых мало отличается от широко распространенных слов (например, пара
слов шоссе--шассе) и включение которых в основной словарь может приводить
к пропуску ошибок.  По умолчанию эти слова не включаются в словарь.  Если
эти слова вам действительно могут понадобиться (например, при создании
поисковой системы), уберите значок # в конце строки dict = ... в
Makefile.

Словарь постоянно совершенствуется, дополняется и корректируется.
Последнюю версию словаря можно найти на сервере:

    ftp://scon155.phys.msu.su/pub/russian/ispell/rus-ispell.tar.gz

Файлы-заготовки для пользователей Windows и DOS в кодировках cp866 и cp1251
можно найти на сервере:

    ftp://scon155.phys.msu.su/pub/russian/ispell/msdos/ricp866.zip
    ftp://scon155.phys.msu.su/pub/russian/ispell/msdos/ricp1251.zip

На основе этого словаря Сергей Виницкий создал словарь для проверки
правописания в текстах, набранных в дореформенной русской орфографии.
Предложенное им расширение кодировки koi8 и сами словари можно найти по
адресу:

    http://oldrus-ispell.sourceforge.net/koi8-extended.html

Вариант словаря, подготовленный для работы с программой MySpell, можно
найти по адресу:

    ftp://scon155.phys.msu.su/pub/russian/ispell/myspell/rus-myspell.tar.gz


УСТАНОВКА:

  A. Для пользователей, работающих в UNIX

Для пользователей UNIX для работы с орфографическим словарем русского
языка понадобится пакет программ ispell (последняя версия имеет номер
3.3.02).  Эти программы обычно включаются в дистрибутивы UNIX и скорее
всего уже имеются на вашем компьютере.  При желании пакет можно взять
по адресу
    http://fmg-www.cs.ucla.edu/geoff/ispell.html
или найти на любом крупном ftp-сервере.  При самостоятельной компиляции
ispell надо скопировать файл local.h.samp в local.h, закомментировать в
нем NO8BIT (#undef NO8BIT) и установить правильные пути для BINDIR,
LIBDIR и других рабочих директорий.

Чтобы построить рабочие файлы для ispell, вам необходим GNU make и
программы buildhash (одна из программ, поставляемых вместе с ispell),
sed, а также sort, tr и uniq (стандартные текстовые утилиты UNIX), пути
к которым указаны в переменной $PATH.

  1) Поместите файл rus-ispell.tar.gz в рабочей директории и
     разархивируйте его с помощью одной из команд:
	gzip -dc rus-ispell.tar.gz | tar -xvf -
	tar -xzvf rus-ispell.tar.gz
  2) Отредактируйте Makefile и убедитесь, что LIB указывает на место,
	где ispell будет искать свои рабочие файлы
  3) Сделайте make для кодировки, используемой в вашей системе:
     koi8-r (koi), iso-8859-5 (iso), cp866 (alt), cp1251 (win) или
     maccyrillic (mac).  Для этого выберите один из следующих вариантов:
	make koi (или просто make)
	make iso
	make alt
	make win
	make mac
     Создаваемый при этом hash-файл не будет поддерживать букву ё (как
     требует того практика большинства изданий на русском языке).  Если
     же по какой-то причине вам нужна поддержка буквы ё (см. выше), для
     включения поддержки буквы ё необходимо набрать:
	make koi YO=1 (или просто make YO=1)
	make iso YO=1
	make alt YO=1
	make win YO=1
	make mac YO=1
  4) Установите словари в рабочей директории ispell (здесь вам могут
     понадобиться права доступа root):
	make install
  5) Теперь можно стереть ненужные файлы:
	make clean

  B. Для пользователей, работающих в Windows

Пользователи, работающие в Windows, могут использовать проверку русской
орфографии со словарями ispell, работая в программе TxtEdit (автор --
Лузиус Шнайдер).  Саму программу можно найти по адресу
    http://www.luziusschneider.com/TxtEditHome.htm
и дополнить ее программой ispell, адаптированной для win32, которую можно
взять с того же сайта по адресу:
    http://www.luziusschneider.com/Speller/ISpCzLoRu.exe
Установка этих программ не вызывает проблем.

Недостатком программы является использование устаревшей (примерно пятилетней
давности) версии настоящего словаря.  Для обновления этой версии пользователи
Windows могут взять готовые affix-файл и списки слов в кодировке cp1251 в
вариантах с поддержкой и без поддержки буквы ё, которые можно найти по адресу:

    ftp:://scon155.phys.msu.su/pub/russian/ispell/msdos/ricp866.zip
    ftp://scon155.phys.msu.su/pub/russian/ispell/msdos/ricp1251.zip

Для обновления словарей необходимо просто распаковать полученные файлы в
директории Program Files\Common Files\ISpell, запустить программу buildhash
(buildhash.exe russian.dic russian.aff russian.hash), а затем переместить
файлы russian.aff, russianyo.aff, russian.hash и russianyo.hash в
поддиректорию Russian, где ispell хранит рабочие файлы.


ИСПОЛЬЗОВАНИЕ:

	ispell -d russian имя_файла

----------------------------------------------------------------
  Со всеми замечаниями, пожеланиями прошу обращаться к автору,
  Александру Лебедеву (swan@scon155.phys.msu.su)

