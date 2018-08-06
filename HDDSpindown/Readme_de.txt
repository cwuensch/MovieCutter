HDDSpindown
===========

HDDSpindown ermöglicht es, den Standby-Timer der internen Festplatte auf einen frei wählbaren Wert festzusetzen (empfehlenswert sind ca. 10-20 min).
Wenn über die Dauer des festgesetzten Zeitraums kein Zugriff auf die Festplatte erfolgt, dann legt diese sich brav schlafen, und wacht von selbst wieder auf, wenn sie das nächste Mal benötigt wird.
Der Timer muss nach jedem Neustart des Topfs neu gesetzt werden. Das TAP muss NICHT im Hintergrund laufen.

(Zu beachten ist naürlich, dass sich die Festplatte nur abschalten kann, wenn wirklich KEINE Zugriffe auf sie erfolgen!
Insbesondere wenn eine Aufnahme / Wiedergabe oder Timeshift läuft, geht das natürlich nicht. Auch die Funktion, die EPG-Daten auf Festplatte abzulegen, sollte deaktiviert sein. Ebenso können zahlreiche TAPs das Abschalten der Festplatte verhindern.
Mit einer Minimal-Konfiguration aus TMSRemote, TMSTelnetd, RemoteSwitch_TMS und MovieCutter klappt es aber problemlos - und einem ruhigen Fernsehabend steht nichts mehr im Wege :-) )