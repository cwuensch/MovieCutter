RecStripper TAP
===============

Das TAP verarbeitet alle *.rec-Dateien im Quell-Ordner /DataFiles/RecStrip.
Die verkleinerten Aufnahmen werden im Zielverzeichnis /DataFiles/RecStrip_out abgelegt.
Die Original-Aufnahmen werden hierbei nur gelesen und NICHT verändert. Sie können auf Wunsch anschließend gelöscht werden.

Die Verarbeitung umfasst aktuell folgende Schritte:
  * Entfernung von Filler-NALUs (nur HD)
  * Entfernung von Zero-byte-Padding
  * Entfernung von Null-Paketen (falls vorhanden)
  * Entfernung der EPG-Spur [optional]

Bei der Verarbeitung werden auch die zugehörigen inf-, nav- und cut-Dateien angepasst.
Die verkleinerten Aufnahmen sollten ohne jede Einschränkung am Topf abspielbar (und auch spulbar) sein.

Es werden sowohl europäische, als auch australische Aufnahmen unterstützt (aktuell aber nur die Erweiterung .rec).

Hinweise:
---------
Zur korrekten Verarbeitung müssen (derzeit noch) Informationen aus der inf-Datei ausgelesen werden, und zwar (a) die Video-PID und (b) der Stream-Typ (SD oder HD).
Sollte keine zugehörige inf-Datei vorhanden sein, kann die resultierende Aufnahme Fehler aufweisen.

Installation:
-------------
Die Dateien 'RecStripper.tap' und 'RecStrip' müssen ins Verzeichnis /ProgramFiles auf dem Topf kopiert werden.
Optional die Sprachdatei 'RecStripper.lng' ins Verzeichnis /ProgramFiles/Settings/RecStripper kopieren.


RecStrip unter Windows
======================
Zur Ausführung des Windows-Programms RecStrip.exe ist das Microsoft Visual C++ 2010 SP1 Redistributable Package (32 bit) erforderlich.
Falls nicht installiert, bitte von hier downloaden:
https://www.microsoft.com/de-DE/download/details.aspx?id=8328
