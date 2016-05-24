Beschreibung
============
Der MovieCutter dient zum komfortablen Schneiden von aufgenommenen Filmen. Es k�nnen ein oder mehrere Schnittbereiche festgelegt werden, die dann entweder aus der Aufnahme herausgel�scht oder in separate Dateien abgespeichert werden.
Im Gegensatz zur integrierten Schnittfunktion des Receivers passt das TAP hierbei die .inf- und .nav-Dateien zu den geschnittenen Aufnahmen korrekt an, sodass diese weiterhin spulbar sind.

Installation
============
Am einfachsten erfolgt die Installation �ber TAPtoDate. Dabei werden s�mtliche ben�tigten Dateien aufgespielt.

Alternative:
Zur manuellen Installation gen�gt es (im Normalfall), die Datei "MovieCutter.tap" per USB oder FTP ins Verzeichnis "/ProgramFiles" auf dem Topf zu kopieren.

Zus�tzlich k�nnen folgende (optionale) Features installiert werden:
- F�r Strippen und Segmente kopieren:	Das Programm "RecStrip" v0.8 (oder h�her) ins Verzeichnis "/ProgramFiles" kopieren. (empfohlen!)
- F�r Dateisystem�berpr�fung (!):	Das Programm "jfs_fsck" (speziell modifizierte Version 1.1.15-TF) ins Verzeichnis "/ProgramFiles" kopieren. (dringend empfohlen!)
- Bereinigung verwaister .cut-Files:	Die Datei "DeleteCuts.sh" ins Verzeichnis "/ProgramFiles/Settings/MovieCutter" kopieren.
- Multilanguage-Unterst�tzung:		Die Sprach-Datei "MovieCutter.lng" ins Verzeichnis "/ProgramFiles/Settings/MovieCutter" kopieren.
- Unterst�tzung weiterer Ger�te:	Sollte Ihr Ger�t nicht von Haus aus unterst�tzt werden, bitte die "FirmwareTMS.dat" von FireBird aufspielen.

Starten / Beenden
=================
Nach dem Starten des TAPs l�uft dieses dezent im Hintergrund, bis eine Aufnahme abgespielt wird.
* Aktivieren des MovieCutters: Dr�cken Sie (w�hrend der Wiedergabe einer Aufnahme) die Schnitt-Taste [>|<] oder die WEISSE Taste, um das OSD einzublenden.
* Ausblenden des MovieCutters: Dr�cken Sie die Taste EXIT einmal, um das OSD wieder auszublenden. Das TAP l�uft dann weiter im Hintergrund und reagiert auf alle Funktionstasten.
* Deaktivieren des MovieCutters: Dr�cken Sie die Taste EXIT noch einmal, um den MovieCutter zu deaktivieren. Das TAP l�uft dann weiter im Hintergrund, arbeitet aber nicht mehr auf Eingaben.
* Um das TAP komplett zu beenden, w�hlen Sie im Schnittmen� die Option "MovieCutter beenden" oder dr�cken Sie einmal die SLEEP-Taste.

Alternativen:
- Wenn in der Datei MovieCutter.ini die Option "AutoOSDPolicy=1" gesetzt wird, dann wird der MovieCutter beim Abspielen einer Aufnahme stets automatisch aktiviert.
- Die Aktivierung erfolgt in diesem Fall in dem unter "DefaultOSDMode" eingestellten Modus.
- Das Ein- und Ausblenden des OSD ist zudem auch �ber den TMS-Commander m�glich.

Bedienung
=========
* Dr�cken Sie die GR�NE Taste, um einen neuen Schnittmarker an der aktuellen Abspielposition anzulegen. Dadurch wird die Aufnahme in Abschnitte (Segmente) unterteilt.
* Durch Dr�cken der GELBEN Taste l�sst sich der n�chstgelegene Schnittmarker an die aktuelle Wiedergabeposition verschieben. Mit der ROTEN Taste wird der n�chstgelegene Marker gel�scht.
* Wenn die gew�nschten Schnittbereiche festgelegt sind, verwenden Sie die BLAUE Taste, um ein oder mehrere dieser Segmente auszuw�hlen.
* Die gew�hlten Segmente lassen sich dann entweder aus der Aufnahme herausl�schen (dann sind sie unwiederbringlich verloren), oder in jeweils eine separate Datei abspeichern (dabei werden sie ebenfalls aus der Original-Aufnahme entfernt).

Alternativen:
- Es besteht auch die M�glichkeit, w�hrend der normalen Wiedergabe einer Datei (bei deaktiviertem MovieCutter) mit der gr�nen Taste Bookmarks anzulegen. Hierzu kann auch ein beliebiges Sprung- oder Werbungs-Such-TAP verwendet werden. Die so angelegten Bookmarks k�nnen anschlie�end in den MovieCutter importiert werden.
- Wenn eine Aufnahme an blo� einer Stelle in zwei Teilst�cke geteilt werden soll, brauchen keine Schnittmarker angelegt werden.

Anwendungsbeispiele
===================
* Eine Aufnahme teilen:
	Spulen Sie die Aufnahme zu der Stelle, an der sie geteilt werden soll. Dr�cken Sie 'Men�', und w�hlen Sie im Schnittmen� "Aufnahme hier teilen".
* Vor- und Nachlauf entfernen:
	Setzen Sie am Beginn und am Ende des zu behaltenden Films je einen Marker. W�hlen Sie dann im Schnittmen� die Funktion "Vor-/Nachlauf markieren" und anschlie�end "Ausgew�hlte Segmente l�schen".
* Eine Aufnahme von Vor- und Nachlauf und der Werbung bereinigen:
	Setzen Sie jeweils einen Marker am Beginn und Ende des eigentlichen Films, sowie einen am Beginn und Ende jeder Werbeunterbrechung. W�hlen Sie dann die Funktion "Ungerade Segmente markieren" und anschlie�end "Ausgew�hlte Segmente l�schen".
* Mehrere Aufnahmen in einer Datei (ohne Werbung innerhalb der Filme):
	Setzen Sie am Beginn und Ende jedes zu behaltenden Films je einen Marker. W�hlen Sie "Gerade Segmente markieren" und anschlie�end "Ausgew�hlte Segmente speichern". Jedes selektierte Segment landet in einer eigenen Datei. In der Original-Aufnahme verbleiben nur noch die unrelevanten Teile.
* Mehrere Aufnahmen in einer Datei (mit Werbeunterbrechungen im Film):
	ZUERST markieren Sie die zu behaltenden Filme und speichern diese in separate Dateien (siehe Punkt 3). DANN eliminieren Sie die Werbebl�cke innerhalb der einzelnen Filme (siehe Punkt 1).

Hinweise
========
- Beim L�schen/Speichern eines Segments wird (durch die Firmware) h�ufig das Ende der Original-Aufnahme korrumpiert!!
    -> Nach meinen Beobachtungen tritt das Problem nicht auf, wenn zwischen Aufnahme und Schneiden ein Neustart des Receivers durchgef�hrt wurde. Aus diesem Grund pr�ft der MovieCutter, ob seit der Aufnahme bereits ein Neustart durchgef�hrt wurde, und zeigt ggf. eine Warnung an.
    -> Mit neueren MovieCutter-Versionen sollte dieses Problem auch ohne Neustart nicht mehr auftreten.
- Da die .inf- und .nav-Dateien aus dem Original neu berechnet werden, sind die geschnittenen Dateien sofort spulbar.
- Auch wenn es durch das genaue Spulen den Anschein erweckt, lassen sich Aufnahmen nicht framegenau schneiden. Das liegt u.a. daran, dass intern mit Bl�cken gearbeitet wird (ca. 9 kB), das Dateisystem danach aber auf Sektorgrenzen schneidet. Bei ersten Tests war diese Differenz bis zu einer halben Sekunde lang, genauere Daten werden wir aber erst durch weitere Tests bekommen.
- Wenn sich der Wiedergabepunkt in den letzten 10 Sekunden der Aufnahme befindet, wird zuerst das Spulen eingestellt und sp�ter automatisch pausiert. Dies soll verhindern, dass sich die Wiedergabe beendet.


Tasten
======
- Play, Pause, Forward, Rewind, Slow haben die normalen Funktionen
- Stop:		Stoppt die Wiedergabe. Da der MovieCutter aber nur mit laufender Wiedergabe funktioniert, wird dadurch das OSD ausgeblendet.
- Exit:		Blendet das OSD aus. Das TAP l�uft weiterhin im Hintergrund und reagiert auf alle Tasten. Nochmaliges Dr�cken deaktiviert den MovieCutter.
- Up / Down:	Zum n�chsten bzw. vorherigen Segment springen. Der Sprung erfolgt mit ca. 1 sek. Verz�gerung, damit das gew�nschte Segment in Ruhe ausgew�hlt werden kann.
- Gr�n:		Neuen Schnittmarker an der aktuellen Wiedergabeposition hinzuf�gen. Da Anfang und Ende der Datei jeweils einen nicht sichtbaren Marker enthalten, wird die Datei mit dem Setzen des ersten Markers in 2 Teile geteilt.
- Rot:		L�scht den n�chstgelegenen Segmentmarker.
- Gelb:		Verschiebt den n�chstgelegenen Marker an die aktuelle Position.
- Blau:		(De-)selektiert das aktive Segment.
- VF / FAV:	Aktiviert den Bookmark-Modus (ab V. 2.0). Mit den Farbtasten lassen sich nun die Bookmarks anstelle der Schnittmarker ver�ndern. Erneutes Dr�cken derselben Taste kehrt in den Segment-Modus zur�ck.
- 1 bis 9, 0:	Aktiviert den Minutensprung-Modus und stellt die gew�nschte Sprungweite ein (1 - 99 min). Die Eingabe der Minutenzahl erfolgt zweistellig. Mit der 0 wird der Minutensprung-Modus wieder beendet. (ab V. 2.0)
- Links,Rechts:	�ndern der Wiedergabe-Geschwindigkeit. Nur noch. (ab V. 2.0)
- Skip-Tasten:	- normal: direkter Segmentsprung. - im Minutensprung-Modus: Sprung um die gew�hlte Minutenzahl. - im Bookmark-Modus: Sprung zum n�chsten bzw. vorherigen Bookmark. (ab V. 2.0)
- Vol-Up/Down:	"
- P+ / P-:	Schnelle Navigation mit sich anpassender Sprungweite (�hnlich FastSkip)
- Ok:		w�hrend der Wiedergabe: Wiedergabe wird angehalten (Pause). - w�hrend Pause oder Spulen: Play.
- Menu:		Pausiert die Wiedergabe und blendet das Schnittmen� ein.
- Wei�:		Wechselt zwischen 3 Darstellungs-Modi (vollst�ndiges OSD, Ausblenden der Segmentliste, Minimal-Modus). (ab V. 2.1)
- Info:		Deaktiviert den MovieCutter und zeigt die EPG-Informationen an.
- Subt:		Blendet das Fenster f�r die Anzeige der Segment-Texte (Captions) ein. (ab V. 3.6)
- Teletext:	Dient bei aktivierter Segment-Text-Anzeige zum �ndern der Caption des aktiven Segments. (ab V. 3.6)


Aktionen im Schnittmen�
=======================
Falls ein oder mehrere Segmente mit der blauen Tasten selektiert wurden (dunkelblau umrahmt), beziehen sich die Speichern/L�schen-Aktionen auf dieses Segment/diese Segmente. Anderenfalls beziehen sich die Funktionen auf das aktive Segment (blau hinterlegt).

* "Markierte Segmente speichern":	Das aktive Segment bzw. die selektierten Segmente werden aus der Original-Aufnahme entfernt und in jeweils einer eigenen Datei gespeichert. Die neu erzeugten Dateien bekommen den Namen der Original-Aufnahme, erg�nzt um den Zusatz "(Cut-1)", "(Cut-2)", usw.
* "Markierte Segmente l�schen":		Das aktive Segment bzw. die selektierten Segmente werden aus der Aufnahme herausgel�scht. Diese Teile sind unwiderruflich verloren!
* "Markierte Segmente kopieren":	Das aktive Segment bzw. die selektierten Segmente werden (gemeinsam) in eine neue Aufnahme kopiert. Die Original-Aufnahme wird hierbei nicht ver�ndert.
* "Aufnahme hier teilen":		Die Aufnahme wird an der aktuellen Abspielposition in zwei Teilst�cke zerteilt. S�mtliche Bookmarks und Segment-Marker bleiben erhalten.
* "Vor-/Nachlauf markieren":		Markiert das erste und letzte von 3 Segmenten.
* "(Un)gerade Segmente markieren":	Markiert alle Segmente mit gerader bzw. alle mit ungerader Nummer. (Die Z�hlung beginnt bei 1.)
* "Importiere Bookmarks":		Die f�r die Aufnahme angelegten Bookmarks werden importiert und als Segmentmarker verwendet.
* "Aufnahme strippen":			Bereinigt die Aufnahme von �berfl�ssigen F�lldaten (Filler-NALUs, Zero-Byte-Stuffing und EPG-Spur). Ein Backup der Original-Aufnahme bleibt erhalten.
* "Dateisystem pr�fen":			Die Integrit�t des Dateisystems der internen Festplatte wird �berpr�ft. Insbesondere werden Dateien, die beim Schneiden besch�digt wurden, repariert. (-> verhindert Aufnahmenfresser)
* "MovieCutter beenden":		Beendet das TAP vollst�ndig. Um es wieder zu verwenden, muss es �ber die TAP-�bersicht neu gestartet werden.

Nicht alle Aktionen stehen jederzeit zur Verf�gung. Sollte ein Eintrag fehlen, versuchen Sie zwischen dem Bookmark- bzw. Segment-Modus zu wechseln.


Optionen in der MovieCutter.ini
===============================
[seit V. 3.3]
  - CutFileMode:		Speicherung der Schnittmarker: [0] in cut- und inf-Datei, [1] nur cut-Datei, [2] nur inf-Datei.
  - DeleteCutFiles:		L�schen verwaister .cut-Files: [0] nie, [1] nur in /DataFiles, [2] rekursiv in Unterverzeichnissen.
  - SpecialEndMode:		Umgekehrte Endbehandlung: [1] nur wenn notwendig, [2] immer, [0] deaktivieren.
  - DisableSpecialEnd	(nicht mehr unterst�tzt)
  - RCUMode:		(neu)	Fernbedienungs-Modus: [0] auto, [1] SRP-2401, [2] SRP-2410, [3] CRP-2401, [4] TF5000 (=CRP), [5] VolKeys nicht belegen.
[seit V. 3.1b]
  - MaxNavDiscrepancy:		Maximal zul�ssige Abweichung der nav-Datei (in Millisekunden), [0] Meldung niemals anzeigen.
[seit V. 3.1]
  - CheckFSAfterCut:	(neu)	Dateisystempr�fung: [1] automatisch (nur wenn n�tig), [2] immer pr�fen, [3] beim Beenden pr�fen, [0] niemals pr�fen (nicht empfohlen!).
  - DoiCheckTest:	(neu)	Inodes �berpr�fen: [1] gesammelt am Ende (ro), [2] gesammelt mit Fix, [3] einzeln zwischen Schnitten (ro), [4] einzeln mit Fix, [0] kein Test.
[seit V. 3.0]
  - DoiCheckTest:	(alt!)	Inodes �berpr�fen: [1] Testen aber nicht fixen, [2] Test und Fix, [0] kein Test.
  - InodeMonitoring:		�berwachung besch�digter Inodes. [0,1]
  - RCUMode:		(alt!)	Fernbedienungs-Modus: [0] SRP-2401, [1] SRP-2410, [2] CRP-2401, [3] TF5000 (=CRP), [4] VolKeys nicht belegen.
[seit V. 2.1]
  - DefaultOSDMode:		OSD-Modus: [0] Hintergrund, [1] Vollst�ndiges OSD, [2] ohne Segmentliste, [3] Minimal-Modus.
  - DefaultMinuteJump:		Startwert f�r den Minutensprung-Modus [1-99], [0] zum Deaktivieren.
  - AskBeforeEdit:		Vor irreversiblen Schnittoperationen nachfragen. [0,1]
  - Overscan_X:			Abstand des OSD vom linken/rechten Bildschirmrand [0-100] (empfohlen: 30-60)
  - Overscan_Y:			Abstand des OSD vom oberen/unteren Bildschirmrand [0-100] (Standard: 25)
  - SegmentList_X:		Anzeigeposition der Segmentliste von links [0-556]
  - SegmentList_Y:		Anzeigeposition der Segmentliste von oben [0-246]
  - DisableSleepKey:		Deaktiviert die MC-Beendigung durch Dr�cken der Sleep-Taste. [0,1]
  - DisableSpecialEnd:	(alt!)	Deaktivieren der umgekehrten Endbehandlung. (Debugging!) [0,1]
[seit V. 2.0i]
  - SaveCutBak			Beim Schneiden ein Backup des CutFiles (.cut.bak) anlegen. [0,1]
  - ShowRebootMessage		Beim Laden einer zu frischen Aufnahme zum Neustart auffordern. [0,1]
  - CheckFSAfterCut:	(alt!)	Dateisystempr�fung: [0] automatisch (nur wenn n�tig), [1] immer pr�fen, [2] niemals pr�fen (nicht empfohlen!).
[seit V. 2.0g]
  - DirectSegmentsCut		Aktion im Schnittmen�: [0] gerade/ungerade Segmente ausw�hlen, [1] gerade/ungerade Segmente l�schen.
[seit V. 1.x]
  - AutoOSDPolicy:		MovieCutter beim Abspielen einer Aufnahme automatisch aktivieren. [0,1]
