' ============================================================
' demo.bas  –  Feature showcase for the CustomLanguage IDE
' Author : Hiroshi Tanaka
' ============================================================

' ── 1. Variables & basic arithmetic ─────────────────────────────────────
Print "=== 1. Variables & Arithmetic ==="

Dim a
Dim b
Dim result

a = 42
b = 13

result = a + b
Print "42 + 13 = " & result

result = a - b
Print "42 - 13 = " & result

result = a * b
Print "42 * 13 = " & result

result = a / b
Print "42 / 13 = " & result

result = a \ b
Print "42 \ 13 (int div) = " & result

result = a Mod b
Print "42 Mod 13 = " & result

result = 2 ^ 10
Print "2 ^ 10 = " & result

' ── 2. String operations ─────────────────────────────────────────────────
Print ""
Print "=== 2. String Operations ==="

Dim s
s = "  Hello, CustomLanguage!  "
Print "Original   : [" & s & "]"
Print "Trimmed    : [" & Trim(s) & "]"
Print "UCase      : " & UCase(Trim(s))
Print "LCase      : " & LCase(Trim(s))
Print "Len        : " & Len(Trim(s))
Print "Left(5)    : " & Left(Trim(s), 5)
Print "Right(14)  : " & Right(Trim(s), 14)
Print "Mid(8,6)   : " & Mid(Trim(s), 8, 6)
Print "InStr(Hello): " & InStr(s, "Hello")
Print "Replace    : " & Replace(Trim(s), "CustomLanguage", "World")
Print "StrReverse : " & StrReverse("abcde")

' ── 3. If / ElseIf / Else ───────────────────────────────────────────────
Print ""
Print "=== 3. If / ElseIf / Else ==="

Dim score
score = 78

If score >= 90 Then
    Print "Grade: A"
ElseIf score >= 80 Then
    Print "Grade: B"
ElseIf score >= 70 Then
    Print "Grade: C  (score=" & score & ")"
ElseIf score >= 60 Then
    Print "Grade: D"
Else
    Print "Grade: F"
End If

' ── 4. For / Next with Step ──────────────────────────────────────────────
Print ""
Print "=== 4. For / Next ==="

Dim i
Dim sum
sum = 0
For i = 1 To 10
    sum = sum + i
Next
Print "Sum 1..10 = " & sum

Print "Even numbers 2-20:"
Dim line
line = ""
For i = 2 To 20 Step 2
    line = line & i & " "
Next
Print line

Print "Countdown 5..1:"
line = ""
For i = 5 To 1 Step -1
    line = line & i & " "
Next
Print line

' ── 5. While / Wend ──────────────────────────────────────────────────────
Print ""
Print "=== 5. While / Wend ==="

Dim n
n = 1
Dim output
output = ""
While n <= 32
    output = output & n & " "
    n = n * 2
Wend
Print "Powers of 2 up to 32: " & output

' ── 6. Do / Loop (Until & While variants) ────────────────────────────────
Print ""
Print "=== 6. Do / Loop ==="

' Collatz sequence starting at 27
Dim x
x = 6
Dim steps
steps = 0
output = "6"
Do Until x = 1
    If x Mod 2 = 0 Then
        x = x \ 2
    Else
        x = x * 3 + 1
    End If
    output = output & " -> " & x
    steps = steps + 1
Loop
Print "Collatz(6): " & output
Print "Steps: " & steps

' ── 7. Functions ─────────────────────────────────────────────────────────
Print ""
Print "=== 7. Functions ==="

Function Factorial(n)
    If n <= 1 Then
        Factorial = 1
    Else
        Factorial = n * Factorial(n - 1)
    End If
End Function

Function IsPrime(n)
    If n < 2 Then
        IsPrime = False
        Exit Function
    End If
    Dim d
    For d = 2 To CInt(Sqr(n))
        If n Mod d = 0 Then
            IsPrime = False
            Exit Function
        End If
    Next
    IsPrime = True
End Function

Function Fibonacci(n)
    If n <= 1 Then
        Fibonacci = n
        Exit Function
    End If
    Dim a2
    Dim b2
    Dim tmp
    a2 = 0
    b2 = 1
    Dim k
    For k = 2 To n
        tmp = a2 + b2
        a2  = b2
        b2  = tmp
    Next
    Fibonacci = b2
End Function

Print "Factorials:"
For i = 0 To 10
    Print "  " & i & "! = " & Factorial(i)
Next

Print "Primes up to 30:"
line = ""
For i = 2 To 30
    If IsPrime(i) Then
        line = line & i & " "
    End If
Next
Print "  " & line

Print "Fibonacci sequence (F0..F12):"
line = ""
For i = 0 To 12
    line = line & Fibonacci(i) & " "
Next
Print "  " & line

' ── 8. Subroutines ───────────────────────────────────────────────────────
Print ""
Print "=== 8. Subroutines ==="

Sub PrintBanner(title)
    Dim bar
    bar = String(Len(title) + 4, "-")
    Print bar
    Print "| " & title & " |"
    Print bar
End Sub

Sub PrintTable(rows, cols)
    Dim r
    Dim c
    For r = 1 To rows
        line = ""
        For c = 1 To cols
            Dim cell
            cell = r * c
            If cell < 10 Then
                line = line & " " & cell & " "
            Else
                line = line & cell & " "
            End If
        Next
        Print line
    Next
End Sub

Call PrintBanner("Multiplication Table 5x5")
Call PrintTable(5, 5)

' ── 9. Math built-ins ────────────────────────────────────────────────────
Print ""
Print "=== 9. Math Built-ins ==="

Print "Abs(-7.5)    = " & Abs(-7.5)
Print "Int(3.9)     = " & Int(3.9)
Print "Int(-3.1)    = " & Int(-3.1)
Print "Fix(-3.9)    = " & Fix(-3.9)
Print "Sqr(144)     = " & Sqr(144)
Print "2^10         = " & (2 ^ 10)
Print "Log(1)       = " & Log(1)
Print "Exp(1)       = " & Exp(1)
Print "Sin(0)       = " & Sin(0)
Print "Cos(0)       = " & Cos(0)

' Approximate Pi via Leibniz series
Dim pi
Dim sign
pi   = 0
sign = 1
For i = 0 To 9999
    pi   = pi + sign / (2 * i + 1)
    sign = sign * -1
Next
pi = pi * 4
Print "Pi (10000 terms) = " & pi

' ── 10. Type conversion & checks ─────────────────────────────────────────
Print ""
Print "=== 10. Type Conversion & Checks ==="

Print "CInt(3.7)    = " & CInt(3.7)
Print "CDbl(""2.5"") = " & CDbl("2.5")
Print "CStr(100)    = " & CStr(100)
Print "CBool(1)     = " & CBool(1)
Print "CBool(0)     = " & CBool(0)
Print "IsNumeric(""42"") = " & IsNumeric("42")
Print "IsNumeric(""abc"")= " & IsNumeric("abc")
Print "TypeName(42)    = " & TypeName(42)
Print "TypeName(3.14)  = " & TypeName(3.14)
Print "TypeName(""hi"") = " & TypeName("hi")
Print "TypeName(True)  = " & TypeName(True)

' ── 11. Nested loops + Exit For ─────────────────────────────────────────
Print ""
Print "=== 11. Nested Loops + Exit ==="

' Find all Pythagorean triples with hypotenuse <= 20
Dim aa
Dim bb
Dim cc
Print "Pythagorean triples (c <= 20):"
For aa = 1 To 20
    For bb = aa To 20
        For cc = bb To 20
            If aa*aa + bb*bb = cc*cc Then
                Print "  " & aa & "^2 + " & bb & "^2 = " & cc & "^2"
            End If
        Next
    Next
Next

' ── 12. String formatting helpers ────────────────────────────────────────
Print ""
Print "=== 12. String Helpers ==="

Function PadLeft(s, width)
    Dim p
    p = CStr(s)
    Do While Len(p) < width
        p = " " & p
    Loop
    PadLeft = p
End Function

Function PadRight(s, width)
    Dim p
    p = CStr(s)
    Do While Len(p) < width
        p = p & " "
    Loop
    PadRight = p
End Function

Print PadRight("Name",    10) & PadLeft("Score", 6)
Print String(16, "-")
Print PadRight("Alice",   10) & PadLeft(98,    6)
Print PadRight("Bob",     10) & PadLeft(74,    6)
Print PadRight("Charlie", 10) & PadLeft(85,    6)
Print PadRight("Diana",   10) & PadLeft(91,    6)

Print ""
Print "=== Done ==="
