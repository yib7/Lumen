# Security Policy

Lumen is an offline command-line renderer. It reads a local scene file and writes
an image file. It opens no network connections and handles no credentials.

The realistic risk surface is the scene-file parser reading untrusted input. If you
find a crash or memory-safety issue (for example a malformed scene that causes a
buffer overrun), please report it.

## Reporting

Open a private security advisory on the repository (Security tab, "Report a
vulnerability"), or open an issue if the problem is not sensitive. Include the
scene file or input that triggers it and the command you ran.
