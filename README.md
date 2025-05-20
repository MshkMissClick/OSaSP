# OSaSP
🔍 DuplicateKiller
DuplicateKiller is a command-line utility written in C that scans a given directory (recursively), detects duplicate files based on their content (not name), and replaces the duplicates with hard links to the original files. It logs all replacements for auditing.

🛠 Features
Recursively scans directories

Computes hash for each regular file using custom hashing (based on djb2 and sdbm)

Detects duplicates based on file content

Replaces duplicates with hard links

Logs all replacements in a file duplicate_log.txt
