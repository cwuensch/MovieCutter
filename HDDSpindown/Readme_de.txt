HDDSpindown
===========

HDDSpindown erm�glicht es, den Standby-Timer der internen Festplatte auf einen frei w�hlbaren Wert festzusetzen (empfehlenswert sind ca. 10-20 min).
Wenn �ber die Dauer des festgesetzten Zeitraums kein Zugriff auf die Festplatte erfolgt, dann legt diese sich brav schlafen, und wacht von selbst wieder auf, wenn sie das n�chste Mal ben�tigt wird.
Der Timer muss nach jedem Neustart des Topfs neu gesetzt werden. Das TAP muss NICHT im Hintergrund laufen.

(Zu beachten ist na�rlich, dass sich die Festplatte nur abschalten kann, wenn wirklich KEINE Zugriffe auf sie erfolgen!
Insbesondere wenn eine Aufnahme / Wiedergabe oder Timeshift l�uft, geht das nat�rlich nicht. Auch die Funktion, die EPG-Daten auf Festplatte abzulegen, sollte deaktiviert sein. Ebenso k�nnen zahlreiche TAPs das Abschalten der Festplatte verhindern.
Mit einer Minimal-Konfiguration aus TMSRemote, TMSTelnetd, RemoteSwitch_TMS und MovieCutter klappt es aber problemlos - und einem ruhigen Fernsehabend steht nichts mehr im Wege :-) )