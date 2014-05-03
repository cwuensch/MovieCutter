Description
===========
MovieCutter provides a convenient solution to edit your recorded movies. A recorded file can be partitioned in one or more segments, which can afterwars be either deleted or saved into separate files.
In contrast to the integrated cutting tool of the receiver, this TAP automatically recalculates the .inf- and .nav files to match the edited recodings, which makes them instantly seekable.

Installation
============
The easiest way to install the TAP is using TAPtoDate.
IMPORTANT!!! To run the MovieCutters you must have installed the packets "SmartEPG FontPack" and "FirmwareTMS.dat" by Firebird. They can easily be obtained via TAPtoDate.

Alternative:
- To install the TAP manually, just copy "MovieCutter.tap" into the folder "/ProgramFiles" and "MovieCutter.lng" and "MovieCutter.ini" into "/ProgramFiles/Settings/MovieCutter".

Starting / Stopping
===================
After starting the TAPs it runs discreetly in the background until a recording is played back.
* Enable the MovieCutter: Press (during playback of a recording) the PLAY button [>] so that the progressbar is displayed. Then press the cut button [>|<] to show the OSD.
* Disable the MovieCutter: Press the EXIT button to hide the OSD again. The TAP then continues to run in the background until the next use.
* To stop the TAP completely, select "Exit MovieCutter" in the action menu, or stop it using the TAP Overview.

Alternatives:
- If the option "AutoOSDPolicy=1" is set in the file MovieCutter.ini, then the MovieCutter is always displayed automatically, when a playback starts.
- Activating/Deactivating the MovieCutter OSD is also possible via the TMS Commander.

Usage
=====
* Press the GREEN button to create a new section marker at the current playback position. This divides the recording into sections (segments).
* Pressing the YELLOW button moves the closest section marker to the current position. With the RED button the nearest marker is deleted.
* If all the desired cutting areas are defined, use the BLUE button to select one or more of these segments.
* The selected segments can then either be deleted from the recording (then they are irretrievably lost), or stored into separate files (which also removes them from the original recording).

Alternative:
- You can also create bookmarks during normal playback of a file (with deactivated MovieCutter) using the green button. This may include any jump or advertising search TAP be used. The thus created bookmarks can then be imported into the MovieCutter.

Use Case Examples
=================
* Remove padding:
	Set a marker at the beginning and at the end of the movie that you want to keep. Then, in the action menu chose "Select padding" function and then select "Delete selected segments".
* Clean a recording from padding and advertising:
	Set a marker each at the beginning and at the end of the actual movie, and one at the beginning and end of each commercial break. Then chose "Select odd segments" and then select "Delete selected segments".
* Multiple recordings in one file (without advertising within the films):
	Set a marker at the beginning and end of each movie, that you want to keep. Chose "Select even segments" and then "Save selected segments". Each selected segment ends up in a separate file. In the original recording only remain the non-relevant parts.
* Multiple recordings in one file (with commercial breaks within the films):
	FIRST, mark every movie to be kept and save them in separate files (see point 3). THEN eliminate the advertising blocks within each of those files (see point 1).

Notes
=====
- When cutting a file the firmware may sometimes corrupt the ending of the original file. Observation showed that rebooting the receiver between recording and cutting might fix this problem.
- Since the .inf and .nav files are recalculated from the original, the edited files are immediately seekable.
- Even if it gives the impression through the exact seeking functions, it is not possible to cut frame-accurately. This is due to the fact that the receiver internally works with blocks (about 9 kB), but the filesystem then cuts on sector boundaries. In the first tests, this difference was up to half a second, but we will only get more accurate data by further testing.
- If the playback point is within the last 10 seconds of the recording, the fast forward or backward function is automatically disabled and later the playback is paused. This is to prevent that the playback exits.


Keys
====
- Play, Pause, Forward, Rewind, Slow have the normal functions.
- Stop:		Stops the playback. Since the MovieCutter only works with playback, the OSD is hidden.
- Exit:		Hides the OSD. The TAP will continue to run in the background.
- Up / Down:	Go to the next or previous segment. The jump is executed with an at about 1 sec. delay to allow you to select another segment.
- Green:	Add a new segment marker at the current playback position. Since the beginning and end of the file already contain an invisible marker, the first segment marker divides the file in two parts.
- Red:		Deletes the closest segment marker.
- Yellow:	Moves the closest segment marker to the current position.
- Blue:		(De-)selects the active segment.
- White:	Enables the Bookmark mode (V. 2.0). The colored buttons can now be used to change the bookmarks instead of segment markers. Pressing the white button returns to the segment mode.
- 1 to 9, 0:	Enables the minute jump mode and changed the jump distance (1 - 99 min). You can enter the number of minutes in two digits. With the 0 minute jump mode is disabled. (V. 2.0)
- Left, Right:	Change the playback speed. Only. (V. 2.0.)
- Skip-Keys:	- normal: direct segment jump. - in minute jump mode: Jump the selected number of minutes forwards or backwards. - in the Bookmark mode: Jump to the next or previous bookmark. (V. 2.0)
- Ok:		during playback: Pause playback. - During pause or seeking: Play.
- Menu:		Pauses the playback and opens the action menu.
- Info:		Switch between 3 display modes (full OSD , hiding the segment list, minimal mode). (V. 2.1)


Actions in the action menu
==========================
If one or more segments were selected with the blue button (dark blue frame), the Save/Delete actions refer to this segment/these segments. Otherwise, the functions are related to the active segment (in blue).

* "Save selected segments":	The active segment or selected segments are removed from the original recording and stored into a separate file each. The newly created files get the name of the original recording, with addition of "(Cut-1)", "(Cut-2)", etc.
* "Delete selected segments":	The active segment or selected segments are canceled out from the recording. These parts are lost forever!
* "Select padding":		Mark the first and last of three segments.
* "Select even/odd segments":	Selects all segments with even resp. odd number. (counting starts at 1)
* "Import Bookmarks":		The Bookmarks from the recording are imported and used as new segment markers.
* "Delete this file":		The current recording is deleted and TAP exits.
* "Exit MovieCutter":		Exits the TAP completely. To use it again, it must be restarted from the TAP Overview.


Options in the MovieCutter.ini
==============================
- AutoOSDPolicy:		1: Movie Cutter is automatically displayed when a playback starts. - 0: Movie Cutter always has to be activated manually.
(supported in V. 2.0g and higher)
- DirectSegmentsCut:		0: Action selects the even/odd segments. - 1: Action deletes the even/odd segments instantly.
(supported in V. 2.0i and higher)
- SaveCutBak			1: Creates a backup of the cut file. - 0: Create no .cut.bak files.
- ShowRebootMessage		1: Reminds the user to reboot the system before doing a cut. - 0: No reboot warning.
- CheckFSAfterCut		0: No automatic file system check after performing a cut.  - 1: Automatic check active.
(shall be supported in V. 2.1 and higher)
- DefaultOSDMode:		0: Show full OSD. - 1: Hide the segment list. - 2: Display OSD in minimal mode.
- MinuteJump:			0: Minute jump mode is disabled at startup. - 1-99: Default value for the minute jump mode.
- AskBeforeEdit:		1: Ask before performing an irreversible cutting operation . - 0: Do not ask.
- OverScanMode: 		1: No important messages are placed to the screen margins (overscan area). - 0: Use the complete screen.
- SegmentList_X:		Display position of the segment list (x-coordinate, possible values: 0-719)
- SegmentList_Y:		Display position of the segment list (y-coordinate, possible values: 0-575)