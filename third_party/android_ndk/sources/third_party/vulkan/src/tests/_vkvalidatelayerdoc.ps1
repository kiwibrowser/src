# Powershell script for running the layer validation details doc validator
# To run this test:
#    From a Windows powershell:
#    cd C:\src\Vulkan-LoaderAndValidationLayers\build\tests
#    .\vkvalidatelayerdoc.ps1 [-Debug]

if ($args[0] -eq "-Debug") {
    $dPath = "Debug"
} else {
    $dPath = "Release"
}

write-host -background black -foreground green "[  RUN     ] " -nonewline
write-host "vkvalidatelayerdoc.ps1: Validate layer documentation"

# Run doc validation from project root dir
push-location ..\..

# Validate that layer documentation matches source contents
python vk_layer_documentation_generate.py --validate

# Report result based on exit code
if (!$LASTEXITCODE) {
    write-host -background black -foreground green "[  PASSED  ] " -nonewline;
    $exitstatus = 0
} else {
    echo 'Validation of vk_validation_layer_details.md failed'
    write-host -background black -foreground red "[  FAILED  ] "  -nonewline;
    echo '1 FAILED TEST'
    $exitstatus = 1
}

pop-location
exit $exitstatus
