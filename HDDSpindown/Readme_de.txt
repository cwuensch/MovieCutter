HDDSpindown
===========

HDDSpindown erm�glicht es, den Standby-Timer der internen Festplatte auf einen frei w�hlbaren Wert festzusetzen (empfehlenswert sind ca. 10-20 min).
Wenn �ber die Dauer des festgesetzten Zeitraums kein Zugriff auf die Festplatte erfolgt, dann legt diese sich brav schlafen, und wacht von selbst wieder auf, wenn sie das n�chste Mal ben�tigt wird.
Der Timer muss nach jedem Neustart des Topfs neu gesetzt werden. Das TAP muss NICHT im Hintergrund laufen.

(Zu beachten ist nat�rlich, dass sich die Festplatte nur abschalten kann, wenn wirklich KEINE Zugriffe auf sie erfolgen!
Insbesondere wenn eine Aufnahme / Wiedergabe oder Timeshift l�uft, geht das nat�rlich nicht. Auch die Funktion, die EPG-Daten auf Festplatte abzulegen, sollte deaktiviert sein. Ebenso k�nnen zahlreiche TAPs das Abschalten der Festplatte verhindern.
Mit einer Minimal-Konfiguration aus TMSRemote, TMSTelnetd, RemoteSwitch_TMS und MovieCutter klappt es aber problemlos - und einem ruhigen Fernsehabend steht nichts mehr im Wege :-) )

Wird das TAP in den AutoStart gelegt (d.h. innerhalb von 60 sek nach dem Booten gestartet), dann liest es nur die HDDSpindown.ini ein und setzt den Standby-Timer f�r das dort eingetragene Laufwerk auf den eingetragenen Zielwert. Danach beendet sich das TAP sofort.
Wird das TAP mehr als 60 sek nach dem Booten gestartet, so zeigt es sein Men� und erm�glicht die Konfiguration des Standby-Timers. Mit "�bernehmen" werden die Werte gespeichert und beim n�chsten Ausf�hren per AutoStart �bernommen.
