.nuget\nuget pack vld.nuspec -OutputDirectory src\bin

.nuget\nuget push src\bin\VisualLeakDetector.*.nupkg %NUGETAPIKEY%
