HDDSpindown
===========

HDDSpindown ermöglicht es, den Standby-Timer der internen Festplatte auf einen frei wählbaren Wert festzusetzen (empfehlenswert sind ca. 10-20 min).
Wenn über die Dauer des festgesetzten Zeitraums kein Zugriff auf die Festplatte erfolgt, dann legt diese sich brav schlafen, und wacht von selbst wieder auf, wenn sie das nächste Mal benötigt wird.
Der Timer muss nach jedem Neustart des Topfs neu gesetzt werden. Das TAP muss NICHT im Hintergrund laufen.

(Zu beachten ist natürlich, dass sich die Festplatte nur abschalten kann, wenn wirklich KEINE Zugriffe auf sie erfolgen!
Insbesondere wenn eine Aufnahme / Wiedergabe oder Timeshift läuft, geht das natürlich nicht. Auch die Funktion, die EPG-Daten auf Festplatte abzulegen, sollte deaktiviert sein. Ebenso können zahlreiche TAPs das Abschalten der Festplatte verhindern.
Mit einer Minimal-Konfiguration aus TMSRemote, TMSTelnetd, RemoteSwitch_TMS und MovieCutter klappt es aber problemlos - und einem ruhigen Fernsehabend steht nichts mehr im Wege :-) )

Wird das TAP in den AutoStart gelegt (d.h. innerhalb von 60 sek nach dem Booten gestartet), dann liest es nur die HDDSpindown.ini ein und setzt den Standby-Timer für das dort eingetragene Laufwerk auf den eingetragenen Zielwert. Danach beendet sich das TAP sofort.
Wird das TAP mehr als 60 sek nach dem Booten gestartet, so zeigt es sein Menü und ermöglicht die Konfiguration des Standby-Timers. Mit "Übernehmen" werden die Werte gespeichert und beim nächsten Ausführen per AutoStart übernommen.
