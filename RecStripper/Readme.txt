RecStripper TAP
===============

Das TAP verarbeitet alle *.rec-Dateien im Quell-Ordner /DataFiles/RecStrip.
Die verkleinerten Aufnahmen werden im Zielverzeichnis /DataFiles/RecStrip_out abgelegt.
Die Original-Aufnahmen werden hierbei nur gelesen und NICHT ver�ndert. Sie k�nnen auf Wunsch anschlie�end gel�scht werden.

Die Verarbeitung umfasst aktuell folgende Schritte:
  * Entfernung von Filler-NALUs (nur HD)
  * Entfernung von Zero-byte-Padding
  * Entfernung von Null-Paketen (falls vorhanden)
  * Entfernung der EPG-Spur [optional]

Bei der Verarbeitung werden auch die zugeh�rigen inf-, nav- und cut-Dateien angepasst.
Die verkleinerten Aufnahmen sollten ohne jede Einschr�nkung am Topf abspielbar (und auch spulbar) sein.

Es werden sowohl europ�ische, als auch australische Aufnahmen unterst�tzt (aktuell aber nur die Erweiterung .rec).

Hinweise:
---------
Zur korrekten Verarbeitung m�ssen (derzeit noch) Informationen aus der inf-Datei ausgelesen werden, und zwar (a) die Video-PID und (b) der Stream-Typ (SD oder HD).
Sollte keine zugeh�rige inf-Datei vorhanden sein, kann die resultierende Aufnahme Fehler aufweisen.

Installation:
-------------
Die Dateien 'RecStripper.tap' und 'RecStrip' m�ssen ins Verzeichnis /ProgramFiles auf dem Topf kopiert werden.
Optional die Sprachdatei 'RecStripper.lng' ins Verzeichnis /ProgramFiles/Settings/RecStripper kopieren.


RecStrip unter Windows
======================
Zur Ausf�hrung des Windows-Programms RecStrip.exe ist das Microsoft Visual C++ 2010 SP1 Redistributable Package (32 bit) erforderlich.
Falls nicht installiert, bitte von hier downloaden:
https://www.microsoft.com/de-DE/download/details.aspx?id=8328
