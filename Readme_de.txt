Beschreibung
============
Der MovieCutter dient zum komfortablen Schneiden von aufgenommenen Filmen. Es können ein oder mehrere Schnittbereiche festgelegt werden, die dann entweder aus der Aufnahme herausgelöscht oder in separate Dateien abgespeichert werden.
Im Gegensatz zur integrierten Schnittfunktion des Receivers passt das TAP hierbei die .inf- und .nav-Dateien zu den geschnittenen Aufnahmen korrekt an, sodass diese weiterhin spulbar sind.

Installation
============
Am einfachsten erfolgt die Installation über TAPtoDate.
WICHTIG!!! Zum Ausführen des MovieCutters müssen die Pakete "SmartEPG FontPack" und "FirmwareTMS.dat" von Firebird installiert sein. Bei Installation über TAPtoDate werden diese automatisch mit installiert.

Alternative:
- Zur manuellen Installation muss "MovieCutter.tap" ins Verzeichnis "/ProgramFiles" kopiert werden und "MovieCutter.lng" und "MovieCutter.ini" ins Verzeichnis "/ProgramFiles/Settings/MovieCutter".
- Zusätzlich müssen die Schriftarten "Calibri_10.ufnt", "Calibri_12.ufnt", "Calibri_14.ufnt" und "Courier_New_13.ufnt" im Verzeichnis "/ProgramFiles/Settings/Fonts" liegen.
- Und es werden unter "/ProgramFiles" die Dateien "FirmwareTMS.dat" und "jfs_fsck" benötigt.

Starten / Beenden
=================
Nach dem Starten des TAPs läuft dieses dezent im Hintergrund, bis eine Aufnahme abgespielt wird.
* Aktivieren des MovieCutters: Drücken Sie (während der Wiedergabe einer Aufnahme) die Schnitt-Taste [>|<] oder die WEISSE Taste, um das OSD einzublenden.
* Ausblenden des MovieCutters: Drücken Sie die Taste EXIT einmal, um das OSD wieder auszublenden. Das TAP läuft dann weiter im Hintergrund und reagiert auf alle Funktionstasten.
* Deaktivieren des MovieCutters: Drücken Sie die Taste EXIT noch einmal, um den MovieCutter zu deaktivieren. Das TAP läuft dann weiter im Hintergrund, arbeitet aber nicht mehr auf Eingaben.
* Um das TAP komplett zu beenden, wählen Sie im Schnittmenü die Option "MovieCutter beenden" oder drücken Sie einmal die SLEEP-Taste.

Alternativen:
- Wenn in der Datei MovieCutter.ini die Option "AutoOSDPolicy=1" gesetzt wird, dann wird der MovieCutter beim Abspielen einer Aufnahme stets automatisch aktiviert.
- Die Aktivierung erfolgt in diesem Fall in dem unter "DefaultOSDMode" eingestellten Modus.
- Das Ein- und Ausblenden des OSD ist zudem auch über den TMS-Commander möglich.

Bedienung
=========
* Drücken Sie die GRÜNE Taste, um einen neuen Schnittmarker an der aktuellen Abspielposition anzulegen. Dadurch wird die Aufnahme in Abschnitte (Segmente) unterteilt.
* Durch Drücken der GELBEN Taste lässt sich der nächstgelegene Schnittmarker an die aktuelle Wiedergabeposition verschieben. Mit der ROTEN Taste wird der nächstgelegene Marker gelöscht.
* Wenn die gewünschten Schnittbereiche festgelegt sind, verwenden Sie die BLAUE Taste, um ein oder mehrere dieser Segmente auszuwählen.
* Die gewählten Segmente lassen sich dann entweder aus der Aufnahme herauslöschen (dann sind sie unwiederbringlich verloren), oder in jeweils eine separate Datei abspeichern (dabei werden sie ebenfalls aus der Original-Aufnahme entfernt).

Alternative:
- Es besteht auch die Möglichkeit, während der normalen Wiedergabe einer Datei (bei deaktiviertem MovieCutter) mit der grünen Taste Bookmarks anzulegen. Hierzu kann auch ein beliebiges Sprung- oder Werbungs-Such-TAP verwendet werden. Die so angelegten Bookmarks können anschließend in den MovieCutter importiert werden.

Anwendungsbeispiele
===================
* Vor- und Nachlauf entfernen:
	Setzen Sie am Beginn und am Ende des zu behaltenden Films je einen Marker. Wählen Sie dann im Schnittmenü die Funktion "Vor-/Nachlauf markieren" und anschließend "Ausgewählte Segmente löschen".
* Eine Aufnahme von Vor- und Nachlauf und der Werbung bereinigen:
	Setzen Sie jeweils einen Marker am Beginn und Ende des eigentlichen Films, sowie einen am Beginn und Ende jeder Werbeunterbrechung. Wählen Sie dann die Funktion "Ungerade Segmente markieren" und anschließend "Ausgewählte Segmente löschen".
* Mehrere Aufnahmen in einer Datei (ohne Werbung innerhalb der Filme):
	Setzen Sie am Beginn und Ende jedes zu behaltenden Films je einen Marker. Wählen Sie "Gerade Segmente markieren" und anschließend "Ausgewählte Segmente speichern". Jedes selektierte Segment landet in einer eigenen Datei. In der Original-Aufnahme verbleiben nur noch die unrelevanten Teile.
* Mehrere Aufnahmen in einer Datei (mit Werbeunterbrechungen im Film):
	ZUERST markieren Sie die zu behaltenden Filme und speichern diese in separate Dateien (siehe Punkt 3). DANN eliminieren Sie die Werbeblöcke innerhalb der einzelnen Filme (siehe Punkt 1).

Hinweise
========
- Beim Löschen/Speichern eines Segments wird (durch die Firmware) häufig das Ende der Original-Aufnahme korrumpiert!!
    -> Nach meinen Beobachtungen tritt das Problem nicht auf, wenn zwischen Aufnahme und Schneiden ein Neustart des Receivers durchgeführt wurde. Aus diesem Grund prüft der MovieCutter, ob seit der Aufnahme bereits ein Neustart durchgeführt wurde, und zeigt ggf. eine Warnung an.
    -> Mit neueren MovieCutter-Versionen sollte dieses Problem auch ohne Neustart nicht mehr auftreten.
- Da die .inf- und .nav-Dateien aus dem Original neu berechnet werden, sind die geschnittenen Dateien sofort spulbar.
- Auch wenn es durch das genaue Spulen den Anschein erweckt, lassen sich Aufnahmen nicht framegenau schneiden. Das liegt u.a. daran, dass intern mit Blöcken gearbeitet wird (ca. 9 kB), das Dateisystem danach aber auf Sektorgrenzen schneidet. Bei ersten Tests war diese Differenz bis zu einer halben Sekunde lang, genauere Daten werden wir aber erst durch weitere Tests bekommen.
- Wenn sich der Wiedergabepunkt in den letzten 10 Sekunden der Aufnahme befindet, wird zuerst das Spulen eingestellt und später automatisch pausiert. Dies soll verhindern, dass sich die Wiedergabe beendet.


Tasten
======
- Play, Pause, Forward, Rewind, Slow haben die normalen Funktionen
- Stop:		Stoppt die Wiedergabe. Da der MovieCutter aber nur mit laufender Wiedergabe funktioniert, wird dadurch das OSD ausgeblendet.
- Exit:		Blendet das OSD aus. Das TAP läuft weiterhin im Hintergrund und reagiert auf alle Tasten. Nochmaliges Drücken deaktiviert den MovieCutter.
- Up / Down:	Zum nächsten bzw. vorherigen Segment springen. Der Sprung erfolgt mit ca. 1 sek. Verzögerung, damit das gewünschte Segment in Ruhe ausgewählt werden kann.
- Grün:		Neuen Schnittmarker an der aktuellen Wiedergabeposition hinzufügen. Da Anfang und Ende der Datei jeweils einen nicht sichtbaren Marker enthalten, wird die Datei mit dem Setzen des ersten Markers in 2 Teile geteilt.
- Rot:		Löscht den nächstgelegenen Segmentmarker.
- Gelb:		Verschiebt den nächstgelegenen Marker an die aktuelle Position.
- Blau:		(De-)selektiert das aktive Segment.
- VF / FAV:	Aktiviert den Bookmark-Modus (ab V. 2.0). Mit den Farbtasten lassen sich nun die Bookmarks anstelle der Schnittmarker verändern. Erneutes Drücken derselben Taste kehrt in den Segment-Modus zurück.
- 1 bis 9, 0:	Aktiviert den Minutensprung-Modus und stellt die gewünschte Sprungweite ein (1 - 99 min). Die Eingabe der Minutenzahl erfolgt zweistellig. Mit der 0 wird der Minutensprung-Modus wieder beendet. (ab V. 2.0)
- Links,Rechts:	Ändern der Wiedergabe-Geschwindigkeit. Nur noch. (ab V. 2.0)
- Skip-Tasten:	- normal: direkter Segmentsprung. - im Minutensprung-Modus: Sprung um die gewählte Minutenzahl. - im Bookmark-Modus: Sprung zum nächsten bzw. vorherigen Bookmark. (ab V. 2.0)
- Vol-Up/Down:	"
- P+ / P-:	Schnelle Navigation mit sich anpassender Sprungweite (ähnlich FastSkip)
- Ok:		während der Wiedergabe: Wiedergabe wird angehalten (Pause). - während Pause oder Spulen: Play.
- Menu:		Pausiert die Wiedergabe und blendet das Schnittmenü ein.
- Weiß:		Wechselt zwischen 3 Darstellungs-Modi (vollständiges OSD, Ausblenden der Segmentliste, Minimal-Modus). (ab V. 2.1)
- Info:         Deaktiviert den MovieCutter und zeigt die EPG-Informationen an.


Aktionen im Schnittmenü
=======================
Falls ein oder mehrere Segmente mit der blauen Tasten selektiert wurden (dunkelblau umrahmt), beziehen sich die Speichern/Löschen-Aktionen auf dieses Segment/diese Segmente. Anderenfalls beziehen sich die Funktionen auf das aktive Segment (blau hinterlegt).

* "Markierte Segmente speichern":	Das aktive Segment bzw. die selektierten Segmente werden aus der Original-Aufnahme entfernt und in jeweils einer eigenen Datei gespeichert. Die neu erzeugten Dateien bekommen den Namen der Original-Aufnahme, ergänzt um den Zusatz "(Cut-1)", "(Cut-2)", usw.
* "Markierte Segmente löschen":	        Das aktive Segment bzw. die selektierten Segmente werden aus der Aufnahme herausgelöscht. Diese Teile sind unwiderruflich verloren!
* "Vor-/Nachlauf markieren":		Markiert das erste und letzte von 3 Segmenten.
* "(Un)gerade Segmente markieren":	Markiert alle Segmente mit gerader bzw. alle mit ungerader Nummer. (Die Zählung beginnt bei 1.)
* "Importiere Bookmarks":		Die für die Aufnahme angelegten Bookmarks werden importiert und als Segmentmarker verwendet.
* "Diese Datei löschen":		Die aktuelle Aufnahme wird gelöscht und das TAP beendet.
* "MovieCutter beenden":		Beendet das TAP vollständig. Um es wieder zu verwenden, muss es über die TAP-Übersicht neu gestartet werden.


Optionen in der MovieCutter.ini
===============================
- AutoOSDPolicy:		1: MovieCutter wird beim Abspielen einer Aufnahme automatisch eingeblendet. - 0: MovieCutter muss immer manuell gestartet werden.
(ab V. 2.0g unterstützt)
- DirectSegmentsCut		0: Aktion zum Auswählen der gerade/ungerade Segmente. - 1: Aktion zum direkten Löschen der geraden/ungeraden Segmente.
(ab V. 2.0i unterstützt)
- SaveCutBak			1: Beim Schneiden wird ein Backup des CutFiles angelegt. - 0: Keine .cut.bak Dateien.
- ShowRebootMessage		1: Vor dem Schnitt wird zum Neustart aufgefordert. - 0: Keine Neustart-Meldung.
- CheckFSAfterCut		0: Automatische Dateisystemprüfung (nur wenn nötig). - 1: Immer nach dem Schneiden prüfen. - 2: Niemals prüfen (nicht empfohlen!)
(ab V. 2.1 unterstützt)
- DefaultOSDMode:		0: MC arbeitet im Hintergrund. - 1: Vollständiges OSD. - 2: OSD ohne Segmentliste. - 3: OSD im Minimal-Modus.
- DefaultMinuteJump:		0: Minutensprung-Modus ist beim Starten deaktiviert. - 1-99: Voreingestellter Wert für den Minutensprung-Modus.
- AskBeforeEdit:		1: Vor dem Ausführen einer irreversiblen Schnittoperation nachfragen. - 0: Keine Rückfrage.
- Overscan_X:			Abstand des OSD vom linken/rechten Bildschirmrand. Mögliche Werte: 0-100 (empfohlen: 30-60)
- Overscan_Y:			Abstand des OSD vom oberen/unteren Bildschirmrand. Mögliche Werte: 0-100 (Standard: 25)
- SegmentList_X:		Anzeigeposition der Segmentliste (x-Koordinate der oberen linken Ecke, mögliche Werte: 0-556)
- SegmentList_Y:		Anzeigeposition der Segmentliste (y-Koordinate der oberen linken Ecke, mögliche Werte: 0-246)
- DisableSleepKey:		1: Deaktiviert die Beendigung des MovieCutters durch Drücken der Sleep-Taste.
- DisableSpecialEnd:		Debugging-Einstellung (Standard: 0).
(ab V. 3.0 unterstützt)
- DoiCheckTest:			0: Kein Inode-Test zwischen den Schnitten. - 1: Testen aber nicht fixen. - 2: Test und fix.
- InodeMonitoring:		1: Überwachung der beim Schneiden beschädigten Inodes. - 0: keine Überwachung.
- RCUMode:			0: SRP-2401 - 1: SRP-2410 - 2: CRP-2401 - 3: TF5000 (identisch mit 2) - 4: VolKeys nicht belegen
