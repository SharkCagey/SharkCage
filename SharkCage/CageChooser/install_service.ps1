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
    Write-Host $sourceDir;
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

        $public_doc_folder = Join-Path -Path ([Environment]::GetEnvironmentVariable("Public")) -ChildPath "Documents\SharkCage";
        Copy-Item -Path "$sourceDir\special_configs\*" -Destination $public_doc_folder -Force;

        New-Service  -Name $serviceName -BinaryPathName (Join-Path $copyPath "CageService.exe") | out-null;
        Start-Service -Name $serviceName | out-null;

        # install dir
        $registry_path = "HKLM:\SOFTWARE\";
        $registry_key = "SharkCage";
        if (-not (Test-Path (Join-Path -Path $registry_path -ChildPath $registry_key)))
        {
            New-Item -Path $registry_path -Name $registry_key;
        }
        
        if (Get-ItemProperty -Path (Join-Path -Path $registry_path -ChildPath $registry_key) -Name "InstallDir")
        {
            Set-ItemProperty -Path (Join-Path -Path $registry_path -ChildPath $registry_key) -Name "InstallDir" -Value $copyPath;
        }
        else
        {
            New-ItemProperty -Path (Join-Path -Path $registry_path -ChildPath $registry_key) -Name "InstallDir" -Value $copyPath -PropertyType "String" | out-null;
        }

        # SharkConfigurator location dir
        $registry_subkey = "Configs";
        $registry_key_combined = (Join-Path -Path $registry_path -ChildPath $registry_key);
        if (-not (Test-Path (Join-Path -Path $registry_key_combined -ChildPath $registry_subkey)))
        {
            New-Item -Path $registry_key_combined -Name $registry_subkey;
        }

        $config_path = Join-Path -Path $public_doc_folder -ChildPath "CageConfigurator.sconfig";

        $regex = '(?:\w*"application_path":.*)';
        $new_text = '"application_path": "' + (Join-Path -Path $copyPath -ChildPath "CageConfigurator.exe") + '",';
        $new_text = $new_text.Replace('\', '\\');
        (Get-Content $config_path) -replace $regex, $new_text | Set-Content $config_path;

        if (Get-ItemProperty -Path (Join-Path -Path $registry_key_combined -ChildPath $registry_subkey) -Name "CageConfigurator")
        {
            Set-ItemProperty -Path (Join-Path -Path $registry_key_combined -ChildPath $registry_subkey) -Name "CageConfigurator" -Value $config_path;
        }
        else
        {
            New-ItemProperty -Path (Join-Path -Path $registry_key_combined -ChildPath $registry_subkey) -Name "CageConfigurator" -Value $config_path -PropertyType "String" | out-null;
        }
    
        Write-Host "Service successfully created!" -ForegroundColor darkgreen;
    }
    Catch
    {
        Write-Host "Service could not be created:" -ForegroundColor red;
        Write-Host $_.Exception.Message;
        Read-Host -Prompt "Press any key to exit...";
        exit;
    }

    exit;
}
