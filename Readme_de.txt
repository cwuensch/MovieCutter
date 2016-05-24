Beschreibung
============
Der MovieCutter dient zum komfortablen Schneiden von aufgenommenen Filmen. Es können ein oder mehrere Schnittbereiche festgelegt werden, die dann entweder aus der Aufnahme herausgelöscht oder in separate Dateien abgespeichert werden.
Im Gegensatz zur integrierten Schnittfunktion des Receivers passt das TAP hierbei die .inf- und .nav-Dateien zu den geschnittenen Aufnahmen korrekt an, sodass diese weiterhin spulbar sind.

Installation
============
Am einfachsten erfolgt die Installation über TAPtoDate. Dabei werden sämtliche benötigten Dateien aufgespielt.

Alternative:
Zur manuellen Installation genügt es (im Normalfall), die Datei "MovieCutter.tap" per USB oder FTP ins Verzeichnis "/ProgramFiles" auf dem Topf zu kopieren.

Zusätzlich können folgende (optionale) Features installiert werden:
- Für Strippen und Segmente kopieren:	Das Programm "RecStrip" v0.8 (oder höher) ins Verzeichnis "/ProgramFiles" kopieren. (empfohlen!)
- Für Dateisystemüberprüfung (!):	Das Programm "jfs_fsck" (speziell modifizierte Version 1.1.15-TF) ins Verzeichnis "/ProgramFiles" kopieren. (dringend empfohlen!)
- Bereinigung verwaister .cut-Files:	Die Datei "DeleteCuts.sh" ins Verzeichnis "/ProgramFiles/Settings/MovieCutter" kopieren.
- Multilanguage-Unterstützung:		Die Sprach-Datei "MovieCutter.lng" ins Verzeichnis "/ProgramFiles/Settings/MovieCutter" kopieren.
- Unterstützung weiterer Geräte:	Sollte Ihr Gerät nicht von Haus aus unterstützt werden, bitte die "FirmwareTMS.dat" von FireBird aufspielen.

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

Alternativen:
- Es besteht auch die Möglichkeit, während der normalen Wiedergabe einer Datei (bei deaktiviertem MovieCutter) mit der grünen Taste Bookmarks anzulegen. Hierzu kann auch ein beliebiges Sprung- oder Werbungs-Such-TAP verwendet werden. Die so angelegten Bookmarks können anschließend in den MovieCutter importiert werden.
- Wenn eine Aufnahme an bloß einer Stelle in zwei Teilstücke geteilt werden soll, brauchen keine Schnittmarker angelegt werden.

Anwendungsbeispiele
===================
* Eine Aufnahme teilen:
	Spulen Sie die Aufnahme zu der Stelle, an der sie geteilt werden soll. Drücken Sie 'Menü', und wählen Sie im Schnittmenü "Aufnahme hier teilen".
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
- Info:		Deaktiviert den MovieCutter und zeigt die EPG-Informationen an.
- Subt:		Blendet das Fenster für die Anzeige der Segment-Texte (Captions) ein. (ab V. 3.6)
- Teletext:	Dient bei aktivierter Segment-Text-Anzeige zum Ändern der Caption des aktiven Segments. (ab V. 3.6)


Aktionen im Schnittmenü
=======================
Falls ein oder mehrere Segmente mit der blauen Tasten selektiert wurden (dunkelblau umrahmt), beziehen sich die Speichern/Löschen-Aktionen auf dieses Segment/diese Segmente. Anderenfalls beziehen sich die Funktionen auf das aktive Segment (blau hinterlegt).

* "Markierte Segmente speichern":	Das aktive Segment bzw. die selektierten Segmente werden aus der Original-Aufnahme entfernt und in jeweils einer eigenen Datei gespeichert. Die neu erzeugten Dateien bekommen den Namen der Original-Aufnahme, ergänzt um den Zusatz "(Cut-1)", "(Cut-2)", usw.
* "Markierte Segmente löschen":		Das aktive Segment bzw. die selektierten Segmente werden aus der Aufnahme herausgelöscht. Diese Teile sind unwiderruflich verloren!
* "Markierte Segmente kopieren":	Das aktive Segment bzw. die selektierten Segmente werden (gemeinsam) in eine neue Aufnahme kopiert. Die Original-Aufnahme wird hierbei nicht verändert.
* "Aufnahme hier teilen":		Die Aufnahme wird an der aktuellen Abspielposition in zwei Teilstücke zerteilt. Sämtliche Bookmarks und Segment-Marker bleiben erhalten.
* "Vor-/Nachlauf markieren":		Markiert das erste und letzte von 3 Segmenten.
* "(Un)gerade Segmente markieren":	Markiert alle Segmente mit gerader bzw. alle mit ungerader Nummer. (Die Zählung beginnt bei 1.)
* "Importiere Bookmarks":		Die für die Aufnahme angelegten Bookmarks werden importiert und als Segmentmarker verwendet.
* "Aufnahme strippen":			Bereinigt die Aufnahme von überflüssigen Fülldaten (Filler-NALUs, Zero-Byte-Stuffing und EPG-Spur). Ein Backup der Original-Aufnahme bleibt erhalten.
* "Dateisystem prüfen":			Die Integrität des Dateisystems der internen Festplatte wird überprüft. Insbesondere werden Dateien, die beim Schneiden beschädigt wurden, repariert. (-> verhindert Aufnahmenfresser)
* "MovieCutter beenden":		Beendet das TAP vollständig. Um es wieder zu verwenden, muss es über die TAP-Übersicht neu gestartet werden.

Nicht alle Aktionen stehen jederzeit zur Verfügung. Sollte ein Eintrag fehlen, versuchen Sie zwischen dem Bookmark- bzw. Segment-Modus zu wechseln.


Optionen in der MovieCutter.ini
===============================
[seit V. 3.3]
  - CutFileMode:		Speicherung der Schnittmarker: [0] in cut- und inf-Datei, [1] nur cut-Datei, [2] nur inf-Datei.
  - DeleteCutFiles:		Löschen verwaister .cut-Files: [0] nie, [1] nur in /DataFiles, [2] rekursiv in Unterverzeichnissen.
  - SpecialEndMode:		Umgekehrte Endbehandlung: [1] nur wenn notwendig, [2] immer, [0] deaktivieren.
  - DisableSpecialEnd	(nicht mehr unterstützt)
  - RCUMode:		(neu)	Fernbedienungs-Modus: [0] auto, [1] SRP-2401, [2] SRP-2410, [3] CRP-2401, [4] TF5000 (=CRP), [5] VolKeys nicht belegen.
[seit V. 3.1b]
  - MaxNavDiscrepancy:		Maximal zulässige Abweichung der nav-Datei (in Millisekunden), [0] Meldung niemals anzeigen.
[seit V. 3.1]
  - CheckFSAfterCut:	(neu)	Dateisystemprüfung: [1] automatisch (nur wenn nötig), [2] immer prüfen, [3] beim Beenden prüfen, [0] niemals prüfen (nicht empfohlen!).
  - DoiCheckTest:	(neu)	Inodes überprüfen: [1] gesammelt am Ende (ro), [2] gesammelt mit Fix, [3] einzeln zwischen Schnitten (ro), [4] einzeln mit Fix, [0] kein Test.
[seit V. 3.0]
  - DoiCheckTest:	(alt!)	Inodes überprüfen: [1] Testen aber nicht fixen, [2] Test und Fix, [0] kein Test.
  - InodeMonitoring:		Überwachung beschädigter Inodes. [0,1]
  - RCUMode:		(alt!)	Fernbedienungs-Modus: [0] SRP-2401, [1] SRP-2410, [2] CRP-2401, [3] TF5000 (=CRP), [4] VolKeys nicht belegen.
[seit V. 2.1]
  - DefaultOSDMode:		OSD-Modus: [0] Hintergrund, [1] Vollständiges OSD, [2] ohne Segmentliste, [3] Minimal-Modus.
  - DefaultMinuteJump:		Startwert für den Minutensprung-Modus [1-99], [0] zum Deaktivieren.
  - AskBeforeEdit:		Vor irreversiblen Schnittoperationen nachfragen. [0,1]
  - Overscan_X:			Abstand des OSD vom linken/rechten Bildschirmrand [0-100] (empfohlen: 30-60)
  - Overscan_Y:			Abstand des OSD vom oberen/unteren Bildschirmrand [0-100] (Standard: 25)
  - SegmentList_X:		Anzeigeposition der Segmentliste von links [0-556]
  - SegmentList_Y:		Anzeigeposition der Segmentliste von oben [0-246]
  - DisableSleepKey:		Deaktiviert die MC-Beendigung durch Drücken der Sleep-Taste. [0,1]
  - DisableSpecialEnd:	(alt!)	Deaktivieren der umgekehrten Endbehandlung. (Debugging!) [0,1]
[seit V. 2.0i]
  - SaveCutBak			Beim Schneiden ein Backup des CutFiles (.cut.bak) anlegen. [0,1]
  - ShowRebootMessage		Beim Laden einer zu frischen Aufnahme zum Neustart auffordern. [0,1]
  - CheckFSAfterCut:	(alt!)	Dateisystemprüfung: [0] automatisch (nur wenn nötig), [1] immer prüfen, [2] niemals prüfen (nicht empfohlen!).
[seit V. 2.0g]
  - DirectSegmentsCut		Aktion im Schnittmenü: [0] gerade/ungerade Segmente auswählen, [1] gerade/ungerade Segmente löschen.
[seit V. 1.x]
  - AutoOSDPolicy:		MovieCutter beim Abspielen einer Aufnahme automatisch aktivieren. [0,1]
