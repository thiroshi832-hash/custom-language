' ============================================================
' form_demo.bas  –  Interactive UI demo for CustomLanguage IDE
' Author : Hiroshi Tanaka
'
' How to run:
'   1. Open this file in the IDE  (File > Open)
'   2. Press Build & Run  (Ctrl+F5)
'   3. A form window appears — click the buttons to interact
'   4. Close the form window to end the program
' ============================================================

' ── Global widget IDs ────────────────────────────────────────────────────
' CreateControl returns an integer ID used by SetProperty / AddItem.
' Declared at module level so event-handler Subs can read them.

Dim lblTitle
Dim lblCounter
Dim bar
Dim btnInc
Dim btnDec
Dim btnReset
Dim lblLog
Dim lstLog
Dim cboColor
Dim chkDouble
Dim lblStatus

' ── State ─────────────────────────────────────────────────────────────────
Dim counter
counter = 0

' ── Create all controls ────────────────────────────────────────────────────

' Title label
lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 28)
SetProperty lblTitle, "text", "CustomLanguage  —  Interactive Counter Demo"

' Counter display
lblCounter = CreateControl("Label", "lblCounter", 10, 50, 380, 40)
SetProperty lblCounter, "text", "Count: 0"

' Progress bar  (0 – 20)
bar = CreateControl("ProgressBar", "bar", 10, 100, 380, 22)
SetProperty bar, "minimum", "0"
SetProperty bar, "maximum", "20"
SetProperty bar, "value",   "0"

' Buttons
btnInc   = CreateControl("PushButton", "btnInc",   10,  134, 115, 32)
btnDec   = CreateControl("PushButton", "btnDec",   140, 134, 115, 32)
btnReset = CreateControl("PushButton", "btnReset", 275, 134, 115, 32)
SetProperty btnInc,   "text", "+ Increment"
SetProperty btnDec,   "text", "- Decrement"
SetProperty btnReset, "text", "Reset"

' "Double step" checkbox
chkDouble = CreateControl("CheckBox", "chkDouble", 10, 176, 200, 24)
SetProperty chkDouble, "text", "Double step (add/sub 2)"

' Color picker combo
lblLog = CreateControl("Label", "lblLog", 10, 210, 120, 22)
SetProperty lblLog, "text", "Pick a colour:"

cboColor = CreateControl("ComboBox", "cboColor", 140, 208, 150, 24)
AddItem cboColor, "Red"
AddItem cboColor, "Green"
AddItem cboColor, "Blue"
AddItem cboColor, "Yellow"
AddItem cboColor, "Purple"

' Event log list
Dim lblLogTitle
lblLogTitle = CreateControl("Label", "lblLogTitle", 10, 242, 100, 22)
SetProperty lblLogTitle, "text", "Event log:"

lstLog = CreateControl("ListWidget", "lstLog", 10, 266, 380, 120)

' Status label at the bottom
lblStatus = CreateControl("Label", "lblStatus", 10, 396, 380, 22)
SetProperty lblStatus, "text", "Ready. Click a button to start."

' ── Show the form — blocks until closed ──────────────────────────────────
ShowForm "Counter Demo", 410, 435

' Execution resumes here after the form is closed.
Print "Form closed. Final counter value: " & counter

' ── Event handlers ────────────────────────────────────────────────────────

Sub btnInc_Click()
    Dim stepVal
    stepVal = 1
    If GetProperty(chkDouble, "checked") = "1" Then
        stepVal = 2
    End If

    counter = counter + stepVal
    Call UpdateDisplay("Incremented by " & stepVal)
End Sub

Sub btnDec_Click()
    Dim stepVal
    stepVal = 1
    If GetProperty(chkDouble, "checked") = "1" Then
        stepVal = 2
    End If

    counter = counter - stepVal
    Call UpdateDisplay("Decremented by " & stepVal)
End Sub

Sub btnReset_Click()
    counter = 0
    Call UpdateDisplay("Reset to 0")
End Sub

Sub cboColor_Changed()
    Dim colorName
    colorName = GetProperty(cboColor, "text")
    SetProperty lblStatus, "text", "Color changed to: " & colorName
End Sub

' ── Helper sub ────────────────────────────────────────────────────────────

Sub UpdateDisplay(msg)
    ' Update counter label
    SetProperty lblCounter, "text", "Count: " & counter

    ' Clamp progress bar to 0-20
    Dim barVal
    barVal = counter
    If barVal < 0 Then
        barVal = 0
    End If
    If barVal > 20 Then
        barVal = 20
    End If
    SetProperty bar, "value", CStr(barVal)

    ' Status line
    SetProperty lblStatus, "text", msg & "  |  counter = " & counter

    ' Append to event log
    AddItem lstLog, msg & "  →  " & counter

    ' Also echo to the IDE console
    Print msg & "  →  counter = " & counter
End Sub
