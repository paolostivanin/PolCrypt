PolCrypt
========

This software let you to:
* Encrypt and decrypt files using AES, Twofish, Serpent or Camellia (all with a 256bit key) in CBC mode;
* Compute the hash of a given file (supported hashes are MD5, GOST, Stribog-512, SHA-1, SHA-256, SHA-512 and Whirlpool);

This software is developed by Paolo Stivanin (a.k.a Polslinux)


Security
--------
* Confidentiality is given by AES/Twofish/Serpent/Camellia;
* Integrity is given by the MAC calculation (MAC = HMAC+SHA512);
* The input key is derived using PBKDF2 with 150'000 iterations and using SHA512 as hash algo;
* High security because your key is temporarly stored into a secure memory that will be destroyed when the program exit;
* The input file will be overwritten prior its removing (secure file deletion);


Latest release
--------------
The latest (v2.1.0-dev) release can be found in the '-dev' branch, just clone it :)


Requirements
------------
* GCC or Clang	: required version of Clang **>= 3.1**, of GCC **>= 4.4.0**;
* Gcrypt	: required version **>=1.6.0**;
* GTK+		: required version **>=3.4.0**;
* Glib		: for CLI **>=2.32.0**, for GUI **>=2.36.0**;


How to use (CLI)
---------------
`./polcrypt-cli --encrypt <path-to-input-file> --algo <aes,twofish,serpent,camellia>` to encrypt a file<br>
`./polcrypt-cli --decrypt <path-to-input-file>` to decrypt a file<br>
`./polcrypt-cli --hash <path-to-input-file> --algo <md5|sha1|sha256|sha512|gostr|stribog|whirlpool|all>` to compute one or more file hash<br>


How to compile
--------------
* Clone the repo<br>
`git clone https://github.com/polslinux/PolCrypt.git`<br>
* move inside the directory you've just downloaded:<br>
`cd /path/to/PolCrypt`<br>
* run:<br>
`make all` to build the cli and gui version<br>
`make cli` to build only the cli version<br>
`make gui` to build only the gui version<br>
`make install` to install the software<br>
After you have successfully compiled the software, you will find the `polcrypt-{cli,gui}` binary into the project root directory.


Extra options
-------------
`./polcrypt-cli --version` to see the current software version<br>
`./polcrypt-cli --help` to display a small help<br>


Notes
-----
This software is licensed under the GNU General Public License version 3 and above.
