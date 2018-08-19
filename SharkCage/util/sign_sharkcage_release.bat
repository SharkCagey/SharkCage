echo off

echo Use this batch script to sign the 'SharkCageApplication'.
echo Make sure: 
echo 1. You started the script with the 'Developer Command Prompt for VS 2017' 
echo 2. You provided the path to your solution folder e.g.: C:\HTWG_shark_cage\SharkCage
echo 3. You have the Yubikey with the certificate (including the private key) available.

set path_to_solution=%1

echo Now build the complete solution, when it is done press enter.
pause >nul

signtool sign /d "CageChooser" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\CageChooser\obj\Release\CageChooser.exe
signtool sign /d "CageConfigurator" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\CageConfigurator\obj\Release\CageConfigurator.exe
signtool sign /d "CageService" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\CageService\Release\CageService.exe
signtool sign /d "CageManager" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\CageManager\Release\CageManager.exe
signtool sign /d "CageManager" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\CageManager\Release\CageManager.exe

echo Now build the 'SharkCageInstaller' project, when it is done press enter
pause >nul

signtool sign /d "SharkCage Installer" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\SharkCageInstaller\Release\setup.exe
signtool sign /d "SharkCage Installer" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 ADBE74BD39789DD111815DE59C60D715143E4620 %path_to_solution%\SharkCageInstaller\Release\SharkCage.msi

echo finished.