Option Explicit

Dim fso, file
Dim NrFiles, i, strText

Set fso = CreateObject("Scripting.FileSystemObject")

NrFiles = WScript.Arguments.Count

If NrFiles > 0 Then
  For i = 0 To (NrFiles - 1)
    Set file = fso.OpenTextFile(WScript.Arguments(i), 1)
    strText = file.ReadAll
    file.Close

    strText = Replace(strText, "byte ", "static byte ")
    strText = Replace(strText, "TYPE_GrData ", "static TYPE_GrData ")

    Set file = fso.OpenTextFile(WScript.Arguments(i), 2)
    file.Write strText
    file.Close
  Next
  WScript.Echo "Fertig."
Else
  WScript.Echo "Bitte zu konvertierende Dateien übergeben."
End If
