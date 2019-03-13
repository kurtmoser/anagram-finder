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

Compile flags (for refactored version):
```bash
$ gcc -O3 -o anagram-refactored anagram-refactored.cpp -lpthread -msse4.2
```


## Files in repo
* bin/anagram-finder - executable program
* bin/anagram-refactored - executable for post-challenge refactored program
* anagram-finder.cpp - source code for the program
* anagram-refactored.cpp - source code for post-challenge refactored program
* readme.md - this readme file


## Notes
* dictionary file must be ISO-8859-13 encoded and have Windows line endings ("\r\n", CRLF), this is the format lemmad.txt dictionary file inside http://www.eki.ee/tarkvara/wordlist/lemmad.zip has
* search word itself is also included in the results in case it is found in the dictionary (i.e. it will not be filtered out from the results)

---

# Post-challenge notes

So there were some important missing pieces that stopped my initial solution from reaching optimal performance. There are likely more ways to improve even the refactored code but for now here are couple of things I learned from coming back to the code after the challenge.

## Multithreading

The most obvious performance gain comes from searching for anagrams in multiple threads in parallel. I was aware of multithreading, but having never used it beforehand myself I wasn't even considering it at the time. Implementing it gives obvious big performance boost in this kind of program over doing everything in single thread.\
An interesting detail to note about working in threads in parallel is that I started to experience performance loss on a machine with 8 processors when using all the threads instead of using less (for example 4) threads. After struggling to find the reason for that I finally accidentally stumbled on the fact that even simplest of mallocing in threads caused this issue and for best performance it was important to malloc all the necessary memory beforehand in the main thread.

## SSE/SIMD

Performance can also be improved by using SSE/SIMD instructions to search for line breaks in dictionary file. Line break searching is the actual bottleneck for this anagram finding program and it is important to get it as fast as possible. My initial solution used simple strchr(), there are better ways like casting char array to unsigned long and using bit operations to search for line break in 8 bytes at once, but interestingly enough there are specific low-level operations for doing such operations fast (and I was completely unaware of their existance) - SSE/SIMD. SSE/SIMD allows to perform "S"ingle "I"nstruction over "M"ultiple "D"ata at once and for our case it means that we can search any character(s) (like line break indicating '\r', '\n', etc) in 16 bytes in parallel.

## mmap()

My initial solution used opening file with fopen() and reading its contents into memory. I experimented also with mmap() (had no previous knowledge about it) but didn't see any big difference in performance. The reason for that was that I actually misimplemented mmap a bit - with mmap there is no need to read/duplicate anything into local variable, we can avoid unnecessary mallocing and copying of data. With mmap we can get direct pointer to the first byte in (dictionary) file and start operating with it immediately.
