HDDSpindown
===========

HDDSpindown makes it possible to set the standby timer of the internal hard disk to a freely selectable value (approx. 10-20 min are recommended).
If the hard disk is not accessed for the duration of the fixed period, it goes to sleep and wakes up the next time it is needed.
The timer must be reset each time the Toppy is restarted. The TAP does NOT have to run in the background.

(Please note, of course, that the hard disk can only switch off if there are really NO accesses to it!
Especially when a recording / playback or timeshift is running, this is of course not possible. The function to store the EPG data on the hard disk should also be deactivated. Many TAPs can also prevent the hard disk from switching off.
But with a minimal configuration of TMSRemote, TMSTelnetd, RemoteSwitch_TMS and MovieCutter it works fine - and nothing stands in the way of a quiet TV evening :-) )

If the TAP is placed in AutoStart (i.e. started within 60 seconds after booting), it reads only the HDDSpindown.ini and sets the standby timer for the drive entered there to the entered target value. After that, the TAP stops immediately.
If the TAP is started more than 60 seconds after booting, it shows its menu and allows the standby timer to be configured. Click on "Apply" to save the values and save them the next time you run the program with AutoStart.
