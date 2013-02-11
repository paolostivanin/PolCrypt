PolCrypt
========

This software let you to:
* Encrypt and decrypt files using AES256-CBC;
* Calculate (and optionally store) the hash of your files (supported hash are SHA-1, RMD160, MD2, MD4, MD5, TIGER, TIGER1, TIGER2, SHA224, SHA256, SHA384, SHA512, CRC32, WHIRLPOOL);

This software is developed by Paolo Stivanin (a.k.a Polslinux)

Security
--------
* Confidentiality is given by AES256-CBC;
* Integrity is given by the MAC calculation (MAC = HMAC+SHA512);
* The input key is derived using PBKDF2 with 150'000 iterations and SHA512 as hash algo;
* High security because your key is temporarly stored into a secure memory that will be destroyed when the program will quit and also the input file will be overwritten and renamed prior its removing (secure file deletion);


Mockup
------
![Image Alt](https://raw.github.com/polslinux/PolCrypt/master/docs/polcrypt.png)

RoadMap
-------
**The code is in PREALPHA stage, there isn't any ETA for the first alpha**
??/??/2013 - v1.0-alpha
* ADDED: GUI (GTK+ v3);
* ADDED: Encrypt/Decrypt with AES256-CBC + MAC (Encrypt-then-MAC, HMAC+SHA512);

Requirements
------------
* GCC or Clang	: required version of Clang **>= 3.1**
* Gcrypt	: required version **>=1.5.0**
* GTK+		: required version **>=3.6.0**
* Glib
* OpenSSL

How it works
------------
...


How to use
----------
...



How to install
--------------
...


Extra options
-------------
...


Notes
-----
...