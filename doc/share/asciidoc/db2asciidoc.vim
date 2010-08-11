:%s/\s*<para>//
:%s/<\/para>//
:%s/    <title>\([^<]*\)<\/title>/===== \1/
:%s/   <title>\([^<]*\)<\/title>/==== \1/
:%s/  <title>\([^<]*\)<\/title>/=== \1/
:%s/ <title>\([^<]*\)<\/title>/== \1/
:%s/<\/title>//
:%s/    <title>\(.*\)/===== \1/
:%s/   <title>\(.*\)/==== \1/
:%s/  <title>\(.*\)/=== \1/
:%s/ <title>\(.*\)/== \1/
:%s/\s*<indexterm>/indexterm:/
:g/\s*<\/indexterm>/d
:g/\s*<\/section>/d
:g/\s*<\/chapter>/d
:%s/<literal>/`/
:%s/<\/literal>/`/
:%s/<literal>/`/g
:%s/<\/literal>/`/g
:%s/\s*<section id=\"\(\S*\)\">/[[\1]]/
:%s/\s*<chapter id=\"\(\S*\)\">/[[\1]]/
:%s/<primary>\([^<]*\)<\/primary>/[\1]/
:%s/<quote>/""/g
:%s/<\/quote>/""/g
:%s/<emphasis>/_/g
:%s/<\/emphasis>/_/g
:%s/<command>/**/g
:%s/<\/command>/**/g
:%s/<option>/%/g
:%s/<\/option>/%/g
:%s/<replaceable>/?/g
:%s/<\/replaceable>/?/g
:%s/\s*<programlisting>/[source]----/
:%s/<\/programlisting>/----/
:%s/\s*<term>\([^<]*\)<\/term>/\1::/

