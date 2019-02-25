# Anagram finder for Helmes programming challenge

Application, that finds all anagrams for provided word from a provided dictionary as fast as possible. Returns comma-separated string where first value is runtime in microseconds and following values are found anagrams.
```
./anagram-finder [fullPathToDictionaryFile] [wordToFindAnagramsFor]
```

Example usage:
```bash
$ ./anagram-finder /path/to/dict.txt lapi
8371,alpi,laip,lapi,pali,pila
```


## Files in repo
* bin/anagram-finder - executable program
* anagram-finder.cpp - source code for the program
* readme.md - this readme file


## Notes
* dictionary file must be ISO-8859-13 encoded and have Windows line endings ("\r\n", CRLF), this is the format lemmad.txt dictionary file inside http://www.eki.ee/tarkvara/wordlist/lemmad.zip has
* search word itself is also included in the results in case it is found in the dictionary (i.e. it will not be filtered out from the results)
