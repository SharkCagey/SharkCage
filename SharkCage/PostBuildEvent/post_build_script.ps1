param(
    [switch] $DontStartNewContext
);

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator) -or -not $DontStartNewContext)
{
    Start-Process powershell.exe -Verb runAs -ArgumentList ("-ExecutionPolicy", "Unrestricted", "& '" + $myinvocation.mycommand.definition + "' -DontStartNewContext");
    exit;
}
else
{
    $sourceDir = Split-Path -Path $PSScriptRoot -Parent;
    $serviceName = "shark-cage-service";
    $defaultCopyPath = "C:\sharkcage\";

    Write-Host "Greetings, this is your friendly neighbourhood shark cage post-build helper!";
    Write-Host "";
    Write-Host "Where do you want to copy the SharkCage binaries? (Default is '$defaultCopyPath', press Enter if you want to keep this)";
    
    $copyPath = Read-Host -Prompt "path";
    
    if ($copyPath.length -eq 0)
    {
        $copyPath = $defaultCopyPath;
    }
    if ($copyPath[-1] -ne '\') { $copyPath += '\'; }
    
    
    Write-Host "Chosen path: $copyPath" -ForegroundColor DarkGreen;
    
    ($CageService = Get-Process CageService -ErrorAction SilentlyContinue) | out-null;
    if ($CageService)
    {
        $CageService | Stop-Process -Force | out-null;
    }
    
    Stop-Service -Name $serviceName -ErrorAction SilentlyContinue | out-null;
    ($service = Get-WmiObject win32_service | Where {$_.name -eq "shark-cage-service"}) | out-null;
    if ($service)
    {
        $service.delete() | out-null;
    }
    
    if (-not (Test-Path -path $copyPath)) 
    {
        New-Item $copyPath -Type Directory | out-null;
    }
    
    Try
    {
        Copy-Item -Path "$sourceDir\post_build_output\*" -Destination $copyPath -Force;
    
        New-Service  -Name $serviceName -BinaryPathName (Join-Path $copyPath "CageService.exe") | out-null;
        Start-Service -Name $serviceName | out-null;
    
        Write-Host "Service successfully created!" -ForegroundColor darkgreen;
    }
    Catch
    {
        Write-Host "Service could not be created:" -ForegroundColor red;
        Write-Host $_.Exception.Message;
        Read-Host -Prompt "Press any key to exit...";
        exit;
    }
    
    $startCmd = Read-Host -Prompt "Would you like to open the 'StarterCmd' program now? [y | yes]";
    if ($startCmd -eq "y" -or $startCmd -eq "yes")
    {
        Start-Process "$copyPath\StarterCmd.exe";
    }
    
    exit;
}