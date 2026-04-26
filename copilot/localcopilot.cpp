#include "localcopilot.h"
#include <QStringList>
#include <algorithm>

// ── LocalCopilot ─────────────────────────────────────────────────────────

LocalCopilot::LocalCopilot(QObject *parent)
    : QObject(parent)
{
    buildTemplates();
}

// ── Scoring ───────────────────────────────────────────────────────────────
// Each keyword that appears in the user's description adds 1 point.
// Partial matches (e.g. "buttons" contains "button") score too.

int LocalCopilot::score(const CodeTemplate &tpl,
                         const QStringList &words) const
{
    int s = 0;
    for (const QString &kw : tpl.keywords) {
        for (const QString &w : words) {
            if (w.contains(kw, Qt::CaseInsensitive) ||
                kw.contains(w, Qt::CaseInsensitive)) {
                ++s;
                break; // count keyword at most once
            }
        }
    }
    return s;
}

// ── Generator ────────────────────────────────────────────────────────────

void LocalCopilot::generate(const QString &desc) {
    QStringList words = desc.split(QRegExp("[\\s,;.!?]+"),
                                   QString::SkipEmptyParts);

    int  bestScore = 0;
    int  bestIdx   = -1;

    for (int i = 0; i < m_templates.size(); ++i) {
        int s = score(m_templates[i], words);
        if (s > bestScore) { bestScore = s; bestIdx = i; }
    }

    if (bestIdx >= 0 && bestScore >= 1) {
        const CodeTemplate &t = m_templates[bestIdx];
        emit codeReady(t.code, t.title, t.description);
    } else {
        // Build a help list
        QString help = "I could not match your request. Try one of these:\n\n";
        for (const auto &t : m_templates)
            help += QString("  • %1 — %2\n").arg(t.title).arg(t.description);
        help += "\nExample: \"counter form\", \"fibonacci\", \"calculator\"";
        emit noMatch(help);
    }
}

// ── Template library ─────────────────────────────────────────────────────

void LocalCopilot::buildTemplates() {

// ── 1. Hello World ────────────────────────────────────────────────────────
m_templates.append({
"hello_world", "Hello World",
"Basic Print statements — the simplest possible program",
{"hello", "world", "print", "first", "simple", "start", "begin", "output", "text"},
R"END(' Hello World
' The simplest CustomLanguage program.

Print "Hello, World!"
Print ""
Print "Welcome to CustomLanguage!"
Print "This is a basic Print statement example."

Dim name
name = "CustomLanguage"
Print "Language name: " & name
Print "Name length:   " & Len(name)
Print "Upper case:    " & UCase(name)
)END"});

// ── 2. Counter Form ───────────────────────────────────────────────────────
m_templates.append({
"counter_form", "Counter Form",
"Interactive form with Increment / Decrement / Reset buttons and a progress bar",
{"counter", "count", "increment", "decrement", "reset", "button", "form",
 "click", "add", "subtract", "plus", "minus", "number"},
R"END(' ── Counter Form ─────────────────────────────────────────
Dim lblTitle
Dim lblCounter
Dim bar
Dim btnInc
Dim btnDec
Dim btnReset
Dim counter

counter = 0

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 26)
SetProperty lblTitle, "text", "Counter Application"

lblCounter = CreateControl("Label", "lblCounter", 10, 46, 380, 44)
SetProperty lblCounter, "text", "Count: 0"

bar = CreateControl("ProgressBar", "bar", 10, 98, 380, 20)
SetProperty bar, "minimum", "0"
SetProperty bar, "maximum", "20"
SetProperty bar, "value", "0"

btnInc   = CreateControl("PushButton", "btnInc",   10,  128, 115, 32)
btnDec   = CreateControl("PushButton", "btnDec",   140, 128, 115, 32)
btnReset = CreateControl("PushButton", "btnReset", 275, 128, 115, 32)
SetProperty btnInc,   "text", "+ Increment"
SetProperty btnDec,   "text", "- Decrement"
SetProperty btnReset, "text", "Reset"

ShowForm "Counter", 410, 200

Print "Form closed. Final count: " & counter

Sub btnInc_Click()
    counter = counter + 1
    Call UpdateDisplay()
End Sub

Sub btnDec_Click()
    counter = counter - 1
    Call UpdateDisplay()
End Sub

Sub btnReset_Click()
    counter = 0
    Call UpdateDisplay()
End Sub

Sub UpdateDisplay()
    SetProperty lblCounter, "text", "Count: " & counter
    Dim barVal
    barVal = counter
    If barVal < 0 Then
        barVal = 0
    End If
    If barVal > 20 Then
        barVal = 20
    End If
    SetProperty bar, "value", CStr(barVal)
    Print "Counter = " & counter
End Sub
)END"});

// ── 3. Calculator Form ────────────────────────────────────────────────────
m_templates.append({
"calculator_form", "Calculator Form",
"Four-function calculator form (Add, Subtract, Multiply, Divide)",
{"calculator", "calc", "add", "subtract", "multiply", "divide",
 "arithmetic", "math", "plus", "minus", "times", "division", "form",
 "two", "numbers", "result", "compute"},
R"END(' ── Calculator Form ──────────────────────────────────────
Dim lblA
Dim txtA
Dim lblB
Dim txtB
Dim lblResult
Dim btnAdd
Dim btnSub
Dim btnMul
Dim btnDiv

Dim lblTitle
lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "Four-Function Calculator"

lblA = CreateControl("Label", "lblA", 10, 44, 80, 22)
SetProperty lblA, "text", "Number A:"
txtA = CreateControl("LineEdit", "txtA", 100, 42, 290, 26)
SetProperty txtA, "placeholderText", "Enter first number"

lblB = CreateControl("Label", "lblB", 10, 78, 80, 22)
SetProperty lblB, "text", "Number B:"
txtB = CreateControl("LineEdit", "txtB", 100, 76, 290, 26)
SetProperty txtB, "placeholderText", "Enter second number"

lblResult = CreateControl("Label", "lblResult", 10, 114, 380, 36)
SetProperty lblResult, "text", "Result: —"

btnAdd = CreateControl("PushButton", "btnAdd",  10,  160, 85, 30)
btnSub = CreateControl("PushButton", "btnSub",  105, 160, 85, 30)
btnMul = CreateControl("PushButton", "btnMul",  200, 160, 85, 30)
btnDiv = CreateControl("PushButton", "btnDiv",  295, 160, 85, 30)
SetProperty btnAdd, "text", "Add  +"
SetProperty btnSub, "text", "Sub  -"
SetProperty btnMul, "text", "Mul  ×"
SetProperty btnDiv, "text", "Div  ÷"

ShowForm "Calculator", 410, 220

Sub btnAdd_Click()
    Dim a
    Dim b
    a = CDbl(GetProperty(txtA, "text"))
    b = CDbl(GetProperty(txtB, "text"))
    SetProperty lblResult, "text", CStr(a) & " + " & CStr(b) & " = " & CStr(a + b)
End Sub

Sub btnSub_Click()
    Dim a
    Dim b
    a = CDbl(GetProperty(txtA, "text"))
    b = CDbl(GetProperty(txtB, "text"))
    SetProperty lblResult, "text", CStr(a) & " - " & CStr(b) & " = " & CStr(a - b)
End Sub

Sub btnMul_Click()
    Dim a
    Dim b
    a = CDbl(GetProperty(txtA, "text"))
    b = CDbl(GetProperty(txtB, "text"))
    SetProperty lblResult, "text", CStr(a) & " × " & CStr(b) & " = " & CStr(a * b)
End Sub

Sub btnDiv_Click()
    Dim a
    Dim b
    a = CDbl(GetProperty(txtA, "text"))
    b = CDbl(GetProperty(txtB, "text"))
    If b = 0 Then
        SetProperty lblResult, "text", "Error: Division by zero!"
    Else
        SetProperty lblResult, "text", CStr(a) & " ÷ " & CStr(b) & " = " & CStr(a / b)
    End If
End Sub
)END"});

// ── 4. Temperature Converter ──────────────────────────────────────────────
m_templates.append({
"temp_converter", "Temperature Converter",
"Form that converts between Celsius, Fahrenheit, and Kelvin",
{"temperature", "celsius", "fahrenheit", "kelvin", "convert", "converter",
 "hot", "cold", "degree", "degrees", "thermal"},
R"END(' ── Temperature Converter ────────────────────────────────
Dim lblTitle
Dim txtInput
Dim lblFrom
Dim cboFrom
Dim lblTo
Dim cboTo
Dim btnConvert
Dim lblResult

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "Temperature Converter"

lblFrom = CreateControl("Label", "lblFrom", 10, 44, 60, 22)
SetProperty lblFrom, "text", "Value:"
txtInput = CreateControl("LineEdit", "txtInput", 80, 42, 160, 26)
SetProperty txtInput, "placeholderText", "e.g. 100"

cboFrom = CreateControl("ComboBox", "cboFrom", 80, 76, 160, 24)
AddItem cboFrom, "Celsius"
AddItem cboFrom, "Fahrenheit"
AddItem cboFrom, "Kelvin"

lblTo = CreateControl("Label", "lblTo", 10, 78, 60, 22)
SetProperty lblTo, "text", "From:"

cboTo = CreateControl("ComboBox", "cboTo", 80, 108, 160, 24)
AddItem cboTo, "Celsius"
AddItem cboTo, "Fahrenheit"
AddItem cboTo, "Kelvin"

Dim lblToLabel
lblToLabel = CreateControl("Label", "lblToLabel", 10, 110, 60, 22)
SetProperty lblToLabel, "text", "To:"

btnConvert = CreateControl("PushButton", "btnConvert", 10, 142, 100, 30)
SetProperty btnConvert, "text", "Convert"

lblResult = CreateControl("Label", "lblResult", 10, 182, 380, 36)
SetProperty lblResult, "text", "Result will appear here"

ShowForm "Temperature Converter", 410, 260

Sub btnConvert_Click()
    Dim val
    Dim fromUnit
    Dim toUnit
    Dim celsius
    Dim result

    val      = CDbl(GetProperty(txtInput, "text"))
    fromUnit = GetProperty(cboFrom, "text")
    toUnit   = GetProperty(cboTo,   "text")

    ' Convert input to Celsius first
    If fromUnit = "Celsius" Then
        celsius = val
    ElseIf fromUnit = "Fahrenheit" Then
        celsius = (val - 32) * 5 / 9
    ElseIf fromUnit = "Kelvin" Then
        celsius = val - 273.15
    End If

    ' Convert Celsius to target unit
    If toUnit = "Celsius" Then
        result = celsius
    ElseIf toUnit = "Fahrenheit" Then
        result = celsius * 9 / 5 + 32
    ElseIf toUnit = "Kelvin" Then
        result = celsius + 273.15
    End If

    SetProperty lblResult, "text", _
        CStr(val) & " " & fromUnit & " = " & CStr(result) & " " & toUnit
    Print val & " " & fromUnit & " = " & result & " " & toUnit
End Sub
)END"});

// ── 5. FizzBuzz ───────────────────────────────────────────────────────────
m_templates.append({
"fizzbuzz", "FizzBuzz",
"Classic FizzBuzz: print Fizz, Buzz, or FizzBuzz for 1-100",
{"fizzbuzz", "fizz", "buzz", "divisible", "modulo", "mod", "classic",
 "interview", "3", "5", "15", "hundred"},
R"END(' ── FizzBuzz ─────────────────────────────────────────────
' Print numbers 1-100.
' Multiple of 3  → Fizz
' Multiple of 5  → Buzz
' Multiple of 15 → FizzBuzz

Dim i
For i = 1 To 100
    If i Mod 15 = 0 Then
        Print "FizzBuzz"
    ElseIf i Mod 3 = 0 Then
        Print "Fizz"
    ElseIf i Mod 5 = 0 Then
        Print "Buzz"
    Else
        Print i
    End If
Next

Print ""
Print "FizzBuzz complete (1 to 100)."
)END"});

// ── 6. Fibonacci ──────────────────────────────────────────────────────────
m_templates.append({
"fibonacci", "Fibonacci Sequence",
"Print the first N Fibonacci numbers using a loop",
{"fibonacci", "fib", "sequence", "golden", "series", "spiral"},
R"END(' ── Fibonacci Sequence ───────────────────────────────────
' Print the first 20 Fibonacci numbers.

Dim n
Dim a
Dim b
Dim temp
Dim i

n = 20
a = 0
b = 1

Print "Fibonacci sequence (first " & n & " terms):"
Print ""

For i = 1 To n
    Print "F(" & i & ") = " & a
    temp = a + b
    a = b
    b = temp
Next

Print ""
Print "Done."
)END"});

// ── 7. Factorial ──────────────────────────────────────────────────────────
m_templates.append({
"factorial", "Factorial",
"Compute N! using a loop and using a recursive Function",
{"factorial", "permutation", "combination", "n!", "multiply", "product"},
R"END(' ── Factorial ────────────────────────────────────────────
' Compute factorial using both a loop and a Function.

' Loop version
Dim n
Dim result
Dim i

n = 10
result = 1
For i = 1 To n
    result = result * i
Next
Print n & "! (loop)     = " & result

' Function version
Print "12! (function) = " & FactorialFunc(12)
Print "0!  (function) = " & FactorialFunc(0)

Function FactorialFunc(x)
    If x <= 1 Then
        Return 1
    End If
    Dim r
    r = 1
    Dim j
    For j = 2 To x
        r = r * j
    Next
    Return r
End Function
)END"});

// ── 8. String Operations ──────────────────────────────────────────────────
m_templates.append({
"string_ops", "String Operations",
"Demo of Len, Left, Right, Mid, UCase, Trim, InStr, Replace, StrReverse",
{"string", "text", "length", "len", "left", "right", "mid", "upper", "lower",
 "reverse", "trim", "replace", "instr", "find", "substring", "char"},
R"END(' ── String Operations ────────────────────────────────────
Dim s
s = "  Hello, CustomLanguage World!  "

Print "Original:   [" & s & "]"
Print "Trimmed:    [" & Trim(s) & "]"
Print "Upper:       " & UCase(Trim(s))
Print "Lower:       " & LCase(Trim(s))
Print ""

Dim clean
clean = Trim(s)
Print "Length:      " & Len(clean)
Print "Left(5):     " & Left(clean, 5)
Print "Right(6):    " & Right(clean, 6)
Print "Mid(8,6):    " & Mid(clean, 8, 6)
Print ""

Print "InStr('World'): position " & InStr(clean, "World")
Print "Replace:     " & Replace(clean, "World", "Universe")
Print "StrReverse:  " & StrReverse(Left(clean, 5))
Print ""

' Check if a word is a palindrome
Dim word
word = "racecar"
If word = StrReverse(word) Then
    Print word & " is a palindrome!"
Else
    Print word & " is NOT a palindrome."
End If
)END"});

// ── 9. Math / Random ──────────────────────────────────────────────────────
m_templates.append({
"math_ops", "Math Operations",
"Demo of Abs, Sqr, Int, Rnd, Sin, Cos, Log, Exp",
{"math", "sqrt", "square", "root", "abs", "absolute", "random", "sin",
 "cos", "tan", "log", "exp", "round", "pi", "number", "rnd"},
R"END(' ── Math Operations ──────────────────────────────────────
Dim pi
pi = 3.14159265358979

Print "── Basic math ─────────────────────────────"
Print "Abs(-7.5)  = " & Abs(-7.5)
Print "Sqr(144)   = " & Sqr(144)
Print "Int(3.9)   = " & Int(3.9)
Print "Fix(-3.9)  = " & Fix(-3.9)
Print "Sgn(-5)    = " & Sgn(-5)
Print ""

Print "── Trigonometry (radians) ─────────────────"
Print "Sin(pi/2)  = " & Sin(pi / 2)
Print "Cos(0)     = " & Cos(0)
Print "Tan(pi/4)  = " & Tan(pi / 4)
Print "Atn(1)*4   = " & Atn(1) * 4
Print ""

Print "── Logarithm / Exp ───────────────────────"
Print "Log(1)     = " & Log(1)
Print "Exp(1) ≈ e = " & Exp(1)
Print "Log(Exp(5))= " & Log(Exp(5))
Print ""

Print "── Random numbers (0-99) ─────────────────"
Randomize
Dim i
For i = 1 To 6
    Print "  Rnd: " & Int(Rnd() * 100)
Next
)END"});

// ── 10. For Loop ─────────────────────────────────────────────────────────
m_templates.append({
"for_loop", "For Loop Examples",
"For/Next loops: counting up, down, by step, nested",
{"for", "loop", "iterate", "range", "from", "to", "step", "next",
 "counting", "count", "repeat", "times", "nested", "number"},
R"END(' ── For Loop Examples ────────────────────────────────────

' Count up 1 to 10
Print "Count up 1-10:"
Dim i
For i = 1 To 10
    Print "  " & i
Next

' Count down 10 to 1
Print ""
Print "Count down 10-1:"
For i = 10 To 1 Step -1
    Print "  " & i
Next

' Even numbers 2 to 20
Print ""
Print "Even numbers 2-20:"
Dim line
line = ""
For i = 2 To 20 Step 2
    line = line & i & "  "
Next
Print "  " & line

' Nested: multiplication table (1-5)
Print ""
Print "5×5 Multiplication table:"
Dim j
For i = 1 To 5
    line = ""
    For j = 1 To 5
        Dim product
        product = i * j
        If product < 10 Then
            line = line & " " & product & "  "
        Else
            line = line & product & "  "
        End If
    Next
    Print "  " & line
Next
)END"});

// ── 11. While Loop ───────────────────────────────────────────────────────
m_templates.append({
"while_loop", "While Loop Examples",
"While/Wend and Do/Loop Until examples",
{"while", "wend", "do", "loop", "until", "condition", "repeat", "while loop"},
R"END(' ── While Loop Examples ──────────────────────────────────

' While / Wend: sum until > 100
Print "While/Wend — summing until sum > 100:"
Dim total
Dim n
total = 0
n = 1
While total <= 100
    total = total + n
    n = n + 1
Wend
Print "  Sum reached " & total & " after adding " & (n - 1) & " numbers."
Print ""

' Do / Loop Until: binary countdown
Print "Do/Loop Until — halving 1024:"
Dim val
val = 1024
Do
    Print "  " & val
    val = val \ 2
Loop Until val < 1
Print ""

' Digit sum
Print "Digit sum of 987654:"
Dim num
num = 987654
Dim digitSum
digitSum = 0
While num > 0
    digitSum = digitSum + (num Mod 10)
    num = num \ 10
Wend
Print "  Digit sum = " & digitSum
)END"});

// ── 12. Function & Sub ───────────────────────────────────────────────────
m_templates.append({
"functions", "Functions and Subs",
"How to define Functions (return value) and Subs (no return)",
{"function", "sub", "subroutine", "procedure", "return", "define",
 "declare", "call", "parameter", "argument", "reusable"},
R"END(' ── Functions and Subs ───────────────────────────────────

' ── Functions return a value ──────────────────────────────
Function Add(a, b)
    Return a + b
End Function

Function Max(a, b)
    If a > b Then
        Return a
    Else
        Return b
    End If
End Function

Function IsPrime(n)
    If n < 2 Then Return False
    Dim i
    For i = 2 To Int(Sqr(n))
        If n Mod i = 0 Then Return False
    Next
    Return True
End Function

' ── Subs do work but return nothing ──────────────────────
Sub PrintDivider(char, length)
    Dim line
    line = String(length, char)
    Print line
End Sub

Sub PrintHeader(title)
    Call PrintDivider("─", 40)
    Print " " & title
    Call PrintDivider("─", 40)
End Sub

' ── Main program ─────────────────────────────────────────
Call PrintHeader("Function Examples")
Print "Add(3, 4)   = " & Add(3, 4)
Print "Max(17, 9)  = " & Max(17, 9)
Print ""
Call PrintHeader("Prime numbers 1-30")
Dim i
For i = 1 To 30
    If IsPrime(i) Then
        Print "  " & i & " is prime"
    End If
Next
)END"});

// ── 13. Grade Calculator ─────────────────────────────────────────────────
m_templates.append({
"grade_calc", "Grade Calculator",
"Form to enter up to 5 scores and show average + letter grade",
{"grade", "grades", "score", "scores", "average", "gpa", "student",
 "pass", "fail", "mark", "exam", "test", "a", "b", "c"},
R"END(' ── Grade Calculator Form ────────────────────────────────
Dim lblTitle
Dim txt1
Dim txt2
Dim txt3
Dim txt4
Dim txt5
Dim btnCalc
Dim lblAvg
Dim lblGrade

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "Grade Calculator (up to 5 scores, 0-100)"

Dim y
y = 42
txt1 = CreateControl("LineEdit", "txt1", 10, y,      90, 26) : SetProperty txt1, "placeholderText", "Score 1"
txt2 = CreateControl("LineEdit", "txt2", 110, y,     90, 26) : SetProperty txt2, "placeholderText", "Score 2"
txt3 = CreateControl("LineEdit", "txt3", 210, y,     90, 26) : SetProperty txt3, "placeholderText", "Score 3"
txt4 = CreateControl("LineEdit", "txt4", 10,  y+34,  90, 26) : SetProperty txt4, "placeholderText", "Score 4"
txt5 = CreateControl("LineEdit", "txt5", 110, y+34,  90, 26) : SetProperty txt5, "placeholderText", "Score 5"

btnCalc = CreateControl("PushButton", "btnCalc", 10, 120, 120, 30)
SetProperty btnCalc, "text", "Calculate Grade"

lblAvg   = CreateControl("Label", "lblAvg",   10, 160, 380, 28)
lblGrade = CreateControl("Label", "lblGrade", 10, 194, 380, 28)
SetProperty lblAvg,   "text", "Average: —"
SetProperty lblGrade, "text", "Grade: —"

ShowForm "Grade Calculator", 420, 260

Sub btnCalc_Click()
    Dim s1
    Dim s2
    Dim s3
    Dim s4
    Dim s5
    s1 = CDbl(GetProperty(txt1, "text"))
    s2 = CDbl(GetProperty(txt2, "text"))
    s3 = CDbl(GetProperty(txt3, "text"))
    s4 = CDbl(GetProperty(txt4, "text"))
    s5 = CDbl(GetProperty(txt5, "text"))

    Dim sum
    Dim count
    sum   = 0
    count = 0
    If s1 > 0 Then
        sum = sum + s1
        count = count + 1
    End If
    If s2 > 0 Then
        sum = sum + s2
        count = count + 1
    End If
    If s3 > 0 Then
        sum = sum + s3
        count = count + 1
    End If
    If s4 > 0 Then
        sum = sum + s4
        count = count + 1
    End If
    If s5 > 0 Then
        sum = sum + s5
        count = count + 1
    End If

    If count = 0 Then
        SetProperty lblAvg, "text", "Please enter at least one score."
        Exit Sub
    End If

    Dim avg
    avg = sum / count
    SetProperty lblAvg, "text", "Average: " & CStr(avg)

    Dim letter
    If avg >= 90 Then
        letter = "A"
    ElseIf avg >= 80 Then
        letter = "B"
    ElseIf avg >= 70 Then
        letter = "C"
    ElseIf avg >= 60 Then
        letter = "D"
    Else
        letter = "F"
    End If
    SetProperty lblGrade, "text", "Grade: " & letter
    Print "Average: " & avg & "  Grade: " & letter
End Sub
)END"});

// ── 14. Todo List Form ───────────────────────────────────────────────────
m_templates.append({
"todo_list", "To-Do List Form",
"Form with a text field to add tasks, list box, and Remove button",
{"todo", "to-do", "task", "tasks", "list", "checklist", "items", "item",
 "add", "remove", "delete", "manage"},
R"END(' ── To-Do List Form ──────────────────────────────────────
Dim txtTask
Dim lstTasks
Dim btnAdd
Dim btnRemove
Dim lblTitle
Dim lblStatus

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "To-Do List"

txtTask = CreateControl("LineEdit", "txtTask", 10, 44, 290, 28)
SetProperty txtTask, "placeholderText", "Enter a task and click Add..."

btnAdd = CreateControl("PushButton", "btnAdd", 308, 44, 82, 28)
SetProperty btnAdd, "text", "Add"

lstTasks = CreateControl("ListWidget", "lstTasks", 10, 82, 380, 200)

btnRemove = CreateControl("PushButton", "btnRemove", 10, 292, 120, 28)
SetProperty btnRemove, "text", "Remove Selected"

lblStatus = CreateControl("Label", "lblStatus", 10, 330, 380, 22)
SetProperty lblStatus, "text", "Add tasks above."

ShowForm "To-Do List", 410, 370)

Sub btnAdd_Click()
    Dim task
    task = Trim(GetProperty(txtTask, "text"))
    If task = "" Then
        SetProperty lblStatus, "text", "Please enter a task first."
        Exit Sub
    End If
    AddItem lstTasks, task
    SetProperty txtTask, "text", ""
    SetProperty lblStatus, "text", "Added: " & task
    Print "Added task: " & task
End Sub

Sub btnRemove_Click()
    SetProperty lblStatus, "text", "Select a task in the list to remove."
    Print "Remove: use the list selection."
End Sub
)END"});

// ── 15. Login Form ───────────────────────────────────────────────────────
m_templates.append({
"login_form", "Login Form",
"Simple login dialog with username, password field, and validation",
{"login", "username", "password", "user", "authenticate", "credentials",
 "sign", "in", "account", "access", "secure"},
R"END(' ── Login Form ───────────────────────────────────────────
Dim lblTitle
Dim lblUser
Dim txtUser
Dim lblPass
Dim txtPass
Dim btnLogin
Dim lblMsg

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 28)
SetProperty lblTitle, "text", "Please Log In"

lblUser = CreateControl("Label", "lblUser", 10, 52, 100, 22)
SetProperty lblUser, "text", "Username:"
txtUser = CreateControl("LineEdit", "txtUser", 120, 50, 260, 26)
SetProperty txtUser, "placeholderText", "Enter username"

lblPass = CreateControl("Label", "lblPass", 10, 88, 100, 22)
SetProperty lblPass, "text", "Password:"
txtPass = CreateControl("LineEdit", "txtPass", 120, 86, 260, 26)
SetProperty txtPass, "placeholderText", "Enter password"

btnLogin = CreateControl("PushButton", "btnLogin", 120, 124, 120, 32)
SetProperty btnLogin, "text", "Log In"

lblMsg = CreateControl("Label", "lblMsg", 10, 168, 380, 24)
SetProperty lblMsg, "text", ""

ShowForm "Login", 410, 220

Sub btnLogin_Click()
    Dim user
    Dim pass
    user = Trim(GetProperty(txtUser, "text"))
    pass = GetProperty(txtPass, "text")

    ' Simple validation (replace with real logic)
    If user = "" Or pass = "" Then
        SetProperty lblMsg, "text", "Please enter both username and password."
        Exit Sub
    End If

    If user = "admin" And pass = "1234" Then
        SetProperty lblMsg, "text", "Login successful! Welcome, " & user & "."
        Print "Logged in as: " & user
    Else
        SetProperty lblMsg, "text", "Invalid credentials. Try admin / 1234."
        Print "Login failed for: " & user
    End If
End Sub
)END"});

// ── 16. Multiplication Table ─────────────────────────────────────────────
m_templates.append({
"multiplication_table", "Multiplication Table",
"Print a 10×10 multiplication table with For loops",
{"multiplication", "table", "times", "multiply", "matrix", "grid",
 "10x10", "product", "squares"},
R"END(' ── Multiplication Table ─────────────────────────────────
' Prints a 10x10 multiplication table.

Print "    │  1   2   3   4   5   6   7   8   9  10"
Print "────┼──────────────────────────────────────────"

Dim i
Dim j
For i = 1 To 10
    Dim row
    If i < 10 Then
        row = " " & i & "  │"
    Else
        row = i & "  │"
    End If
    For j = 1 To 10
        Dim p
        p = i * j
        If p < 10 Then
            row = row & "   " & p
        ElseIf p < 100 Then
            row = row & "  " & p
        Else
            row = row & " " & p
        End If
    Next
    Print row
Next
Print ""
Print "Done."
)END"});

// ── 17. Progress Bar Form ────────────────────────────────────────────────
m_templates.append({
"progress_form", "Progress Bar Form",
"Animated progress bar with Start/Stop/Reset buttons",
{"progress", "bar", "loading", "percentage", "percent", "status",
 "animate", "fill", "indicator"},
R"END(' ── Progress Bar Form ────────────────────────────────────
Dim lblTitle
Dim bar
Dim lblPct
Dim btnStart
Dim btnReset

lblTitle = CreateControl("Label",       "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "Progress Bar Demo"

bar = CreateControl("ProgressBar", "bar", 10, 44, 380, 28)
SetProperty bar, "minimum", "0"
SetProperty bar, "maximum", "100"
SetProperty bar, "value",   "0"

lblPct = CreateControl("Label", "lblPct", 10, 82, 380, 24)
SetProperty lblPct, "text", "0%"

btnStart = CreateControl("PushButton", "btnStart", 10,  116, 115, 30)
btnReset = CreateControl("PushButton", "btnReset", 275, 116, 115, 30)
SetProperty btnStart, "text", "Fill to 100%"
SetProperty btnReset, "text", "Reset"

ShowForm "Progress Demo", 410, 180

Sub btnStart_Click()
    Dim i
    For i = 0 To 100 Step 5
        SetProperty bar,    "value", CStr(i)
        SetProperty lblPct, "text",  CStr(i) & "%"
    Next
    SetProperty lblPct, "text", "Complete!"
    Print "Progress complete."
End Sub

Sub btnReset_Click()
    SetProperty bar,    "value", "0"
    SetProperty lblPct, "text",  "0%"
End Sub
)END"});

// ── 18. Color Picker Form ────────────────────────────────────────────────
m_templates.append({
"color_picker", "Color Picker Form",
"Form with a ComboBox of colors and a status label showing the selection",
{"color", "colour", "picker", "palette", "combobox", "combo", "dropdown",
 "select", "rgb", "theme", "hue"},
R"END(' ── Color Picker Form ────────────────────────────────────
Dim lblTitle
Dim lblColor
Dim cboColor
Dim lblResult
Dim btnApply

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "Color Picker"

lblColor = CreateControl("Label", "lblColor", 10, 48, 100, 22)
SetProperty lblColor, "text", "Pick a color:"

cboColor = CreateControl("ComboBox", "cboColor", 120, 46, 200, 26)
AddItem cboColor, "Red"
AddItem cboColor, "Green"
AddItem cboColor, "Blue"
AddItem cboColor, "Yellow"
AddItem cboColor, "Orange"
AddItem cboColor, "Purple"
AddItem cboColor, "White"
AddItem cboColor, "Black"

btnApply = CreateControl("PushButton", "btnApply", 10, 84, 100, 30)
SetProperty btnApply, "text", "Apply"

lblResult = CreateControl("Label", "lblResult", 10, 124, 380, 30)
SetProperty lblResult, "text", "No color selected yet."

ShowForm "Color Picker", 410, 190

Sub btnApply_Click()
    Dim chosen
    chosen = GetProperty(cboColor, "text")
    SetProperty lblResult, "text", "You chose: " & chosen
    Print "Color selected: " & chosen
End Sub

Sub cboColor_Changed()
    Dim chosen
    chosen = GetProperty(cboColor, "text")
    SetProperty lblResult, "text", "Selected: " & chosen
End Sub
)END"});

// ── 19. BMI Calculator ───────────────────────────────────────────────────
m_templates.append({
"bmi_calculator", "BMI Calculator Form",
"Body Mass Index calculator with weight, height inputs and category label",
{"bmi", "body", "mass", "index", "weight", "height", "overweight",
 "underweight", "obese", "health", "kg", "cm"},
R"END(' ── BMI Calculator Form ──────────────────────────────────
Dim lblTitle
Dim lblWeight
Dim txtWeight
Dim lblHeight
Dim txtHeight
Dim btnCalc
Dim lblBmi
Dim lblCategory

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "BMI Calculator"

lblWeight = CreateControl("Label", "lblWeight", 10, 48, 120, 22)
SetProperty lblWeight, "text", "Weight (kg):"
txtWeight = CreateControl("LineEdit", "txtWeight", 140, 46, 150, 26)
SetProperty txtWeight, "placeholderText", "e.g. 70"

lblHeight = CreateControl("Label", "lblHeight", 10, 84, 120, 22)
SetProperty lblHeight, "text", "Height (cm):"
txtHeight = CreateControl("LineEdit", "txtHeight", 140, 82, 150, 26)
SetProperty txtHeight, "placeholderText", "e.g. 175"

btnCalc = CreateControl("PushButton", "btnCalc", 10, 122, 120, 32)
SetProperty btnCalc, "text", "Calculate BMI"

lblBmi = CreateControl("Label", "lblBmi", 10, 166, 380, 28)
SetProperty lblBmi, "text", "BMI: —"

lblCategory = CreateControl("Label", "lblCategory", 10, 200, 380, 28)
SetProperty lblCategory, "text", "Category: —"

ShowForm "BMI Calculator", 410, 260

Sub btnCalc_Click()
    Dim wt
    Dim ht
    Dim bmi
    wt = CDbl(GetProperty(txtWeight, "text"))
    ht = CDbl(GetProperty(txtHeight, "text")) / 100  ' cm to m

    If wt <= 0 Or ht <= 0 Then
        SetProperty lblBmi, "text", "Please enter valid weight and height."
        Exit Sub
    End If

    bmi = wt / (ht * ht)
    SetProperty lblBmi, "text", "BMI: " & CStr(Int(bmi * 10) / 10)

    Dim cat
    If bmi < 18.5 Then
        cat = "Underweight"
    ElseIf bmi < 25 Then
        cat = "Normal weight"
    ElseIf bmi < 30 Then
        cat = "Overweight"
    Else
        cat = "Obese"
    End If
    SetProperty lblCategory, "text", "Category: " & cat
    Print "BMI = " & bmi & "  (" & cat & ")"
End Sub
)END"});

// ── 20. Number Guessing Game ─────────────────────────────────────────────
m_templates.append({
"guessing_game", "Number Guessing Game",
"Form game: guess a random number 1-100 with high/low hints",
{"guess", "guessing", "game", "random", "number", "hint", "higher",
 "lower", "play", "secret", "try", "attempt"},
R"END(' ── Number Guessing Game ─────────────────────────────────
Dim lblTitle
Dim txtGuess
Dim btnGuess
Dim btnNew
Dim lblHint
Dim lblAttempts

Dim secret
Dim attempts

Randomize
secret   = Int(Rnd() * 100) + 1
attempts = 0

lblTitle = CreateControl("Label", "lblTitle", 10, 10, 380, 24)
SetProperty lblTitle, "text", "Guess the number (1 - 100)"

txtGuess = CreateControl("LineEdit", "txtGuess", 10, 44, 200, 28)
SetProperty txtGuess, "placeholderText", "Enter your guess..."

btnGuess = CreateControl("PushButton", "btnGuess", 220, 44, 80, 28)
SetProperty btnGuess, "text", "Guess!"

btnNew = CreateControl("PushButton", "btnNew", 308, 44, 82, 28)
SetProperty btnNew, "text", "New Game"

lblHint = CreateControl("Label", "lblHint", 10, 84, 380, 30)
SetProperty lblHint, "text", "Make your first guess!"

lblAttempts = CreateControl("Label", "lblAttempts", 10, 120, 380, 24)
SetProperty lblAttempts, "text", "Attempts: 0"

ShowForm "Guessing Game", 410, 180

Sub btnGuess_Click()
    Dim g
    g = CInt(GetProperty(txtGuess, "text"))
    attempts = attempts + 1
    SetProperty lblAttempts, "text", "Attempts: " & attempts
    SetProperty txtGuess, "text", ""

    If g = secret Then
        SetProperty lblHint, "text", _
            "Correct! You got it in " & attempts & " attempts!"
        Print "WIN in " & attempts & " attempts! Secret was " & secret
    ElseIf g < secret Then
        SetProperty lblHint, "text", CStr(g) & " is too LOW. Try higher!"
    Else
        SetProperty lblHint, "text", CStr(g) & " is too HIGH. Try lower!"
    End If
End Sub

Sub btnNew_Click()
    Randomize
    secret   = Int(Rnd() * 100) + 1
    attempts = 0
    SetProperty lblHint,     "text", "New game! Make your first guess."
    SetProperty lblAttempts, "text", "Attempts: 0"
    Print "New game started. Good luck!"
End Sub
)END"});

} // buildTemplates
