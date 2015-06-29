archivefs
=========

This is a project that I experimented when I took the OS course. It allows you to map any archive file(.zip,.rar,.7z ...) to a file system, so that you can open the file in other applications without manually decompressing it. And since it's a true file system, you can browse or search like any other file systems under Windows.

The idea is quite interesting, I may continue the project when I get time. What I plan to do next is to mirror a whole file system, and when it encounters an archive file, the FS automatically decompress it and fake it as a directory. This can be helpful in some scenarios. For example, I am using foobar2000, with this file system, it can search into any archive files for music.

The project is based on 
  Dokan: dokan-dev.net and 
  lib7zip: https://code.google.com/p/lib7zip/
