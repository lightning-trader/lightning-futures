# Convert all source files to UTF-8 with BOM
$files = Get-ChildItem -Path '..\src' -Recurse -Include '*.cpp','*.h','*.hpp'
foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw -Encoding Default
    $utf8Bom = New-Object System.Text.UTF8Encoding $true
    [System.IO.File]::WriteAllText($file.FullName, $content, $utf8Bom)
    Write-Host "Converted: $($file.FullName)"
}

# Convert test files
$testFiles = Get-ChildItem -Path '.' -Include '*.cpp','*.h','*.hpp'
foreach ($file in $testFiles) {
    $content = Get-Content $file.FullName -Raw -Encoding Default
    $utf8Bom = New-Object System.Text.UTF8Encoding $true
    [System.IO.File]::WriteAllText($file.FullName, $content, $utf8Bom)
    Write-Host "Converted: $($file.FullName)"
}

Write-Host "All files converted to UTF-8 with BOM successfully!"