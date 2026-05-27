@echo off

echo =========================
echo BUILDING ALL LABS
echo =========================

g++ lab1/preprocessor.cpp -o lab1/preprocessor.exe
g++ lab2/lexer.cpp -o lab2/lexer.exe
g++ lab3/parser.cpp -o lab3/parser.exe
g++ lab4/semantic.cpp -o lab4/semantic.exe

if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED
    pause
    exit /b
)

echo.
echo =========================
echo RUNNING PIPELINE
echo =========================

lab1\preprocessor.exe lab1/test.cpp

if %errorlevel% neq 0 (
    echo Preprocessor failed
    pause
    exit /b
)

lab2\lexer.exe preprocessed_test.cpp

if %errorlevel% neq 0 (
    echo Lexer failed
    pause
    exit /b
)

lab3\parser.exe lexer_output_preprocessed_test.txt

if %errorlevel% neq 0 (
    echo Parser failed
    pause
    exit /b
)

lab4\semantic.exe ast_output.json

if %errorlevel% neq 0 (
    echo Semantic analyzer failed
    pause
    exit /b
)

echo.
echo =========================
echo PIPELINE FINISHED
echo =========================

pause