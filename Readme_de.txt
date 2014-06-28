Beschreibung
============
Der MovieCutter dient zum komfortablen Schneiden von aufgenommenen Filmen. Es k�nnen ein oder mehrere Schnittbereiche festgelegt werden, die dann entweder aus der Aufnahme herausgel�scht oder in separate Dateien abgespeichert werden.
Im Gegensatz zur integrierten Schnittfunktion des Receivers passt das TAP hierbei die .inf- und .nav-Dateien zu den geschnittenen Aufnahmen korrekt an, sodass diese weiterhin spulbar sind.

Installation
============
Am einfachsten erfolgt die Installation �ber TAPtoDate.
WICHTIG!!! Zum Ausf�hren des MovieCutters m�ssen die Pakete "SmartEPG FontPack" und "FirmwareTMS.dat" von Firebird installiert sein. Bei Installation �ber TAPtoDate werden diese automatisch mit installiert.

Alternative:
- Zur manuellen Installation muss "MovieCutter.tap" ins Verzeichnis "/ProgramFiles" kopiert werden und "MovieCutter.lng" und "MovieCutter.ini" ins Verzeichnis "/ProgramFiles/Settings/MovieCutter".
- Zus�tzlich m�ssen die Schriftarten "Calibri_10.ufnt", "Calibri_12.ufnt", "Calibri_14.ufnt" und "Courier_New_13.ufnt" im Verzeichnis "/ProgramFiles/Settings/Fonts" liegen.
- Und es werden unter "/ProgramFiles" die Dateien "FirmwareTMS.dat" und "jfs_fsck" ben�tigt.

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

Alternative:
- Es besteht auch die M�glichkeit, w�hrend der normalen Wiedergabe einer Datei (bei deaktiviertem MovieCutter) mit der gr�nen Taste Bookmarks anzulegen. Hierzu kann auch ein beliebiges Sprung- oder Werbungs-Such-TAP verwendet werden. Die so angelegten Bookmarks k�nnen anschlie�end in den MovieCutter importiert werden.

Anwendungsbeispiele
===================
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
- Info:         Deaktiviert den MovieCutter und zeigt die EPG-Informationen an.


Aktionen im Schnittmen�
=======================
Falls ein oder mehrere Segmente mit der blauen Tasten selektiert wurden (dunkelblau umrahmt), beziehen sich die Speichern/L�schen-Aktionen auf dieses Segment/diese Segmente. Anderenfalls beziehen sich die Funktionen auf das aktive Segment (blau hinterlegt).

* "Markierte Segmente speichern":	Das aktive Segment bzw. die selektierten Segmente werden aus der Original-Aufnahme entfernt und in jeweils einer eigenen Datei gespeichert. Die neu erzeugten Dateien bekommen den Namen der Original-Aufnahme, erg�nzt um den Zusatz "(Cut-1)", "(Cut-2)", usw.
* "Markierte Segmente l�schen":	        Das aktive Segment bzw. die selektierten Segmente werden aus der Aufnahme herausgel�scht. Diese Teile sind unwiderruflich verloren!
* "Vor-/Nachlauf markieren":		Markiert das erste und letzte von 3 Segmenten.
* "(Un)gerade Segmente markieren":	Markiert alle Segmente mit gerader bzw. alle mit ungerader Nummer. (Die Z�hlung beginnt bei 1.)
* "Importiere Bookmarks":		Die f�r die Aufnahme angelegten Bookmarks werden importiert und als Segmentmarker verwendet.
* "Diese Datei l�schen":		Die aktuelle Aufnahme wird gel�scht und das TAP beendet.
* "MovieCutter beenden":		Beendet das TAP vollst�ndig. Um es wieder zu verwenden, muss es �ber die TAP-�bersicht neu gestartet werden.


Optionen in der MovieCutter.ini
===============================
- AutoOSDPolicy:		1: MovieCutter wird beim Abspielen einer Aufnahme automatisch eingeblendet. - 0: MovieCutter muss immer manuell gestartet werden.
(ab V. 2.0g unterst�tzt)
- DirectSegmentsCut		0: Aktion zum Ausw�hlen der gerade/ungerade Segmente. - 1: Aktion zum direkten L�schen der geraden/ungeraden Segmente.
(ab V. 2.0i unterst�tzt)
- SaveCutBak			1: Beim Schneiden wird ein Backup des CutFiles angelegt. - 0: Keine .cut.bak Dateien.
- ShowRebootMessage		1: Vor dem Schnitt wird zum Neustart aufgefordert. - 0: Keine Neustart-Meldung.
- CheckFSAfterCut		0: Automatische Dateisystempr�fung (nur wenn n�tig). - 1: Immer nach dem Schneiden pr�fen. - 2: Niemals pr�fen (nicht empfohlen!)
(ab V. 2.1 unterst�tzt)
- DefaultOSDMode:		0: MC arbeitet im Hintergrund. - 1: Vollst�ndiges OSD. - 2: OSD ohne Segmentliste. - 3: OSD im Minimal-Modus.
- DefaultMinuteJump:		0: Minutensprung-Modus ist beim Starten deaktiviert. - 1-99: Voreingestellter Wert f�r den Minutensprung-Modus.
- AskBeforeEdit:		1: Vor dem Ausf�hren einer irreversiblen Schnittoperation nachfragen. - 0: Keine R�ckfrage.
- Overscan_X:			Abstand des OSD vom linken/rechten Bildschirmrand. M�gliche Werte: 0-100 (empfohlen: 30-60)
- Overscan_Y:			Abstand des OSD vom oberen/unteren Bildschirmrand. M�gliche Werte: 0-100 (Standard: 25)
- SegmentList_X:		Anzeigeposition der Segmentliste (x-Koordinate der oberen linken Ecke, m�gliche Werte: 0-556)
- SegmentList_Y:		Anzeigeposition der Segmentliste (y-Koordinate der oberen linken Ecke, m�gliche Werte: 0-246)
- DisableSleepKey:		1: Deaktiviert die Beendigung des MovieCutters durch Dr�cken der Sleep-Taste.
- DisableSpecialEnd:		Debugging-Einstellung (Standard: 0).
(ab V. 3.0 unterst�tzt)
- DoiCheckTest:			0: Kein Inode-Test zwischen den Schnitten. - 1: Testen aber nicht fixen. - 2: Test und fix.
- InodeMonitoring:		1: �berwachung der beim Schneiden besch�digten Inodes. - 0: keine �berwachung.
- RCUMode:			0: SRP-2401 - 1: SRP-2410 - 2: CRP-2401 - 3: TF5000 (identisch mit 2) - 4: VolKeys nicht belegen
