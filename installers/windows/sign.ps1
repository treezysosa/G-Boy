param([Parameter(Mandatory=$true)][string]$CertificateThumbprint,[Parameter(Mandatory=$true)][string[]]$Files)
$ErrorActionPreference='Stop'
foreach ($file in $Files) {
  & signtool sign /sha1 $CertificateThumbprint /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 $file
  if ($LASTEXITCODE -ne 0) { throw "Signing failed: $file" }
  & signtool verify /pa /v $file
  if ($LASTEXITCODE -ne 0) { throw "Signature validation failed: $file" }
}
